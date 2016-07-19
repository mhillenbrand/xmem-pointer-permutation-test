#include <iostream>
#include <cinttypes>

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

/* TODO templateify this and add wrapper classes */
float testGenerator(int N, bool printHist) {

    uint64_t *array =  new uint64_t[N];

    int * strides = new int[2*N];
    int i;

    for(i = 0; i < 2 * N; i++)
	strides[i] = 0;

    std::cout << "list of " << N << " elements." << std::endl;

    xmem::build_random_pointer_permutation(array,
	    (void *) &array[N-1], xmem::CHUNK_64b);

    std::cout << "shuffled." << std::endl;

    uint64_t * p = array;

    int cyclelength = 0;
    bool * visited = new bool[N];

    for(i = 0; i < N; i++)
	visited[i] = false;

    do {
	/* std::cout << " " << (p - array) << ","; */

	visited[ p - array ] = true;

	cyclelength ++;

	int stride = (int) ( ((uint64_t *) *p) - p);

	/* Note: when coding undercaffeinated, you might need to resort
	 * to this kind of debugging:
	 std::cout << (p - array) << " -> " << ( (uint64_t *)*p - array)
		<< ": " << stride << " idx " << (N + stride) << std::endl;
	*/

	strides[ N + stride ] ++;

	p = (uint64_t *) *p;

    } while( ! visited[ p - array ] );

    std::cout << "found cycle of length " << cyclelength;
    float covered = (100.0* cyclelength) / N;
    std::cout << " (i.e., covering " << covered << "%)";
    std::cout << " on index " << (p - array) << std::endl;

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
	    std::cout << "[" << a[i] << ";" << b[i] << ")  ";

	    int dots = 40 * hist[i] / maxAmount;

	    for(int j = 0; j < dots; j++)
		std::cout << '*';

	    for(int j = 0; j < 40 - dots; j++)
		std::cout << ' ';

	    std::cout << " (" << hist[i] << ")" << std::endl;
	}
    }

    delete array;
    delete strides;
    delete visited;

    return covered;
}

int main(int argc, char ** argv) {

    testGenerator(128, true);
    testGenerator(1024, true);

    testGenerator(6<<20, true);
    /* testGenerator(20<<20, true);
    testGenerator(32<<20, true);
    testGenerator(64<<20, true); */
    /* testGenerator(128<<20, true); */
    /* testGenerator(256<<20, true); */

    float sum = 0;
    for(int i = 0; i < 100; i++)
	sum += testGenerator(4<<20, true);

    sum /= 100;
    std::cout << std::endl << "for 4 MiB test case, average coverage of ";
    std::cout << sum << "% (100 runs)" << std::endl;

}

