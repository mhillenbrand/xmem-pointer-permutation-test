#include <algorithm>
#include <cinttypes>
#include <iomanip>
#include <iostream>
#include <random>
#include <typeinfo>

#include "benchmark_kernels.h"
#include "common.h"

/* Testsuite for generators of randomly permutated linked lists
 * ============================================================
 *
 * Desired metrics for the permutation:
 * (1) Maximum memory coverage of pointer chain: chasing pointers will
 *     visit all elements (i.e., the pointers form a hamiltonian cycle).
 * (2) Avoid regular stride: address distance between elements should be
 *     random (what distribution, actually??).
 *
 * Test a generator for these properties
 *  1. generate permutation
 *  2. alloc and init stride histogram
 *  3. follow pointer chain until we return to the initial element
 *      (a) count visited elements
 *      (b) note each stride in the histogram.
*/

namespace xmem {
    bool g_verbose = false; /* needed by X-Mem source code */
};

class permutated_list_generator {
protected:
    int N;
    uint64_t * list;

public:
    permutated_list_generator(int N) : N(N) {
	list = new uint64_t[N];
    }

    virtual ~permutated_list_generator() {
	delete list;
    }

    int getN() const { return N; }

    virtual uint64_t * getlist() = 0;
};

class XMem_list_generator : public permutated_list_generator {

public:
    XMem_list_generator(int N) : permutated_list_generator(N) { }

    uint64_t * getlist() {

	xmem::build_random_pointer_permutation(list,
	    (void *) &list[N-1], xmem::CHUNK_64b);

	return list;
    }

};

class external_shuffle_generator : public permutated_list_generator {

public:
    external_shuffle_generator(int N) : permutated_list_generator(N) { }

    uint64_t * getlist() {

	/* represent the order in which we are going to traverse the
	 * pointer nodes */
	int * traversal_order = new int[N + 1];

	/* 0, 1, 2, ...., N-1 */
	for(int i = 0; i < N; i++)
	    traversal_order[i] = i;

	/* and back to 0 */
	traversal_order[N] = 0;

	/* at this point, traversal_order represents a hamiltonion
	 * cycle, albeit a not very random one */

	std::mt19937_64 gen(time(NULL)); //Mersenne Twister random number generator, seeded as in X-Mem
	//std::minstd_rand0 gen(time(NULL)); // slightly faster alternative

	/* now we randomize the order in which we visit the nodes,
	 * maintaining the invariants that
	 *   - we begin and end at the 0
	 *   - traversal_order represents a hamiltonian cycle over all
	 *     nodes 
	 * For that purpose, we only shuffle the elements 1, 2, ..., N-1
	 * in the middle */
	std::shuffle(traversal_order + 1, traversal_order + (N - 1), gen);

	/* std::cout << std::endl;

	for(int i = 0; i < N + 1; i++)
		std::cout << traversal_order[i] << " ";

	std::cout << std::endl; */

	/* implement the traversal order within the pointer array */
	for(int i = 0; i < N; i++) {
	    /* std::cout << traversal_order[i] << " -> ";
	    std::cout << traversal_order[i+1] << std::endl; */
	    list[ traversal_order[i] ] =
		(uint64_t) &list[ traversal_order[i+1] ];
	}

	delete traversal_order;

	return list;
    }

};

template<class T> float testGenerator(int N, bool printHist) {

    T gen(N);
    int * strides = new int[2*N];
    int i;

    for(i = 0; i < 2 * N; i++)
	strides[i] = 0;

    std::cout << typeid(T).name() << ": list of " << N << " elements." << std::endl;

    uint64_t * list = gen.getlist();
    uint64_t * p = list;

    int cyclelength = 0;
    bool * visited = new bool[N];

    for(i = 0; i < N; i++)
	visited[i] = false;

    do {
	/* std::cout << " " << (p - list) << ","; */

	visited[ p - list ] = true;

	cyclelength ++;

	int stride = (int) ( ((uint64_t *) *p) - p);

	/* Note: when coding undercaffeinated, you might need to resort
	 * to this kind of debugging:
	 std::cout << (p - list) << " -> " << ( (uint64_t *)*p - list)
		<< ": " << stride << " idx " << (N + stride) << std::endl;
	 */

	strides[ N + stride ] ++;

	p = (uint64_t *) *p;

    } while( ! visited[ p - list ] );

    std::cout << typeid(T).name() << ": found cycle of length " << cyclelength;
    float covered = (100.0* cyclelength) / N;
    std::cout << " (i.e., covering " << covered << "%)";
    std::cout << " on index " << (p - list) << std::endl;

    std::cout << std::endl;

    if(printHist) {
	/* simple early version
	    std::cout << "Histogram of stride lengths" << std::endl;
	for(i = 0; i < 2 * N; i++) {
	    int stride = i - N;
	
	    std::cout << stride << " | " << strides[i] << std::endl;
	}
	*/

	/* tidy histogram on 20 lines */
	int hist[20], a[20], b[20];
	int maxAmount = 0;

	std::cout << "Histogram of stride lengths" << std::endl;
	for(i = 0; i < 20; i++) {
	    a[i] = (int) (-N + (2.0*N / 20) * i);
	    b[i] = (int) (-N + (2.0*N / 20) * (i+1));

	    hist[i]= 0;

	    for(int j = a[i] + N; j < b[i] + N; j++)
		hist[i] += strides[j];

	    if(hist[i] > maxAmount)
		maxAmount = hist[i];
	}

	for(i = 0; i < 20; i++) {
	    std::cout <<  "[" << std::setw(9) << a[i] << ";";
	    std::cout << std::setw(9) << b[i] << ") ";

	    int dots = 40 * hist[i] / maxAmount;

	    for(int j = 0; j < dots; j++)
		std::cout << '*';

	    for(int j = 0; j < 40 - dots; j++)
		std::cout << ' ';

	    std::cout << " (" << hist[i] << ")" << std::endl;
	}

	std::cout << std::endl;
    }

    delete strides;
    delete visited;

    return covered;
}

template<class T> void speedrun(int N) {

    T gen(N);

    uint64_t * l = gen.getlist();

    (void) l;

}

int main(int argc, char ** argv) {

    //testGenerator<external_shuffle_generator>(32<<20, true);
    //speedrun<XMem_list_generator>(32 << 20);
    //speedrun<external_shuffle_generator>(32 << 20);

    testGenerator<external_shuffle_generator>(128, true);
    testGenerator<external_shuffle_generator>(1024, true);
    testGenerator<external_shuffle_generator>(6<<20, true);
    testGenerator<external_shuffle_generator>(32<<20, true);

    testGenerator<XMem_list_generator>(128, true);
    testGenerator<XMem_list_generator>(1024, true);


    testGenerator<XMem_list_generator>(6 << 20, true);
    testGenerator<XMem_list_generator>(32 << 20, true);
    /* testGenerator(6<<20, true); */
    /* testGenerator(20<<20, true);
    testGenerator(32<<20, true);
    testGenerator(64<<20, true); */
    /* testGenerator(128<<20, true); */
    /* testGenerator(256<<20, true); */

    float sum = 0;
    for(int i = 0; i < 100; i++)
	sum += testGenerator<XMem_list_generator>(2 << 20, false);

    sum /= 100;

    float sum2 = 0;
    for(int i = 0; i < 100; i++)
	sum2 += testGenerator<external_shuffle_generator>(2 << 20, false);

    sum2 /= 100;

    std::cout << std::endl << "X-Mem: for 2 MiB test case, average coverage of ";
    std::cout << sum << "% (100 runs)" << std::endl;

    std::cout << std::endl << "external shuffle: for 2 MiB test case, average coverage of ";
    std::cout << sum2 << "% (100 runs)" << std::endl;
}

