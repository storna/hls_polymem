#include <stdio.h>
#include <stdlib.h>
#include <bitset>
#include "hls_polymem.h"

#define DIM 16
#define log2_p 1 // log2(p)
#define log2_q 2 // log2(q)
#define p (1 << log2_p) // number of rows of the matrix in which physical memory modules (linear registers) are organized in the POLYMEM
#define q (1 << log2_q) // number of columns of the matrix in which physical memory modules (linear registers) are organized in the POLYMEM
#define N_LINEAR_REGISTERS (p * q)

typedef int DATA_T;


void print_matrix(DATA_T a[DIM][DIM]){

	int i, j;

    for(i = 0; i < DIM; i++){
        for(j = 0; j < DIM; j++){
        	std::cout << a[i][j] << "\t";
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}


void print_block(DATA_T block[N_LINEAR_REGISTERS]){

	int i;

    for(i = 0; i < N_LINEAR_REGISTERS; i++){
        std::cout << block[i] << "\t";
    }
    std::cout << "\n";
}


int main() {
	int i, j;
    DATA_T block[N_LINEAR_REGISTERS];

	DATA_T a[DIM][DIM];
    for(i = 0; i < DIM; i++)
        for(j = 0; j < DIM; j++)
        	a[i][j] = i * DIM + j;//((DATA_T) rand() / (RAND_MAX / 100));

    printf("\n============= Random Matrix =============\r\n\n");
    print_matrix(a);


    printf("\n============= POLYMEM_SCHEME_ReO =============\r\n\n");
	// TEST SCHEME
    hls::polymem<DATA_T, log2_p, log2_q, DIM, DIM, POLYMEM_SCHEME_ReO> a_ReO;
    for(i = 0; i < DIM; i++)
        for(j = 0; j < DIM; j++)
        	a_ReO.write(a[i][j], i, j);

    a_ReO.print_registers();
    a_ReO.print();

    printf("\n---> SCHEME_ReO - ACCESS_TYPE_Re (i=3, j=5): \r\n");
    a_ReO.read_block(3, 5, block, POLYMEM_ACCESS_TYPE_Re);
    print_block(block);




    printf("\n============= POLYMEM_SCHEME_ReRo =============\r\n\n");
	// TEST SCHEME
    hls::polymem<DATA_T, log2_p, log2_q, DIM, DIM, POLYMEM_SCHEME_ReRo> a_ReRo;
    for(i = 0; i < DIM; i++)
        for(j = 0; j < DIM; j++)
        	a_ReRo.write(a[i][j], i, j);

    a_ReRo.print_registers();
    a_ReRo.print();

    printf("\n---> SCHEME_ReRo - ACCESS_TYPE_Re (i=3, j=5): \r\n");
    a_ReRo.read_block(3, 5, block, POLYMEM_ACCESS_TYPE_Re);
    print_block(block);

    printf("\n---> SCHEME_ReRo - ACCESS_TYPE_Ro (i=3, j=5): \r\n");
    a_ReRo.read_block(3, 5, block, POLYMEM_ACCESS_TYPE_Ro);
    print_block(block);

    printf("\n---> SCHEME_ReRo - ACCESS_TYPE_MD (i=1, j=1): \r\n");
    a_ReRo.read_block(1, 1, block, POLYMEM_ACCESS_TYPE_MD);
    print_block(block);

    printf("\n---> SCHEME_ReRo - ACCESS_TYPE_SD (i=1, j=14): \r\n");
    a_ReRo.read_block(1, 14, block, POLYMEM_ACCESS_TYPE_SD);
    print_block(block);




    printf("\n============= POLYMEM_SCHEME_ReCo =============\r\n\n");
	// TEST SCHEME
    hls::polymem<DATA_T, log2_p, log2_q, DIM, DIM, POLYMEM_SCHEME_ReCo> a_ReCo;
    for(i = 0; i < DIM; i++)
        for(j = 0; j < DIM; j++)
        	a_ReCo.write(a[i][j], i, j);

    a_ReCo.print_registers();
    a_ReCo.print();

    printf("\n---> SCHEME_ReCo - ACCESS_TYPE_Re (i=3, j=5): \r\n");
    a_ReCo.read_block(3, 5, block, POLYMEM_ACCESS_TYPE_Re);
    print_block(block);

    printf("\n---> SCHEME_ReCo - ACCESS_TYPE_Co (i=3, j=5): \r\n");
    a_ReCo.read_block(3, 5, block, POLYMEM_ACCESS_TYPE_Co);
    print_block(block);

    printf("\n---> SCHEME_ReCo - ACCESS_TYPE_MD (i=1, j=1): \r\n");
    a_ReCo.read_block(1, 1, block, POLYMEM_ACCESS_TYPE_MD);
    print_block(block);

    printf("\n---> SCHEME_ReCo - ACCESS_TYPE_SD (i=1, j=14): \r\n");
    a_ReCo.read_block(1, 14, block, POLYMEM_ACCESS_TYPE_SD);
    print_block(block);




    printf("\n============= POLYMEM_SCHEME_RoCo =============\r\n\n");
	// TEST SCHEME
    hls::polymem<DATA_T, log2_p, log2_q, DIM, DIM, POLYMEM_SCHEME_RoCo> a_RoCo;
    for(i = 0; i < DIM; i++)
        for(j = 0; j < DIM; j++)
        	a_RoCo.write(a[i][j], i, j);

    a_RoCo.print_registers();
    a_RoCo.print();

    printf("\n---> SCHEME_RoCo - ACCESS_TYPE_Ro (i=3, j=5): \r\n");
    a_RoCo.read_block(3, 5, block, POLYMEM_ACCESS_TYPE_Ro);
    print_block(block);

    printf("\n---> SCHEME_RoCo - ACCESS_TYPE_Co (i=3, j=5): \r\n");
    a_RoCo.read_block(3, 5, block, POLYMEM_ACCESS_TYPE_Co);
    print_block(block);

    printf("\n---> SCHEME_RoCo - ACCESS_TYPE_Re (i=12, j=4): \r\n");
    a_RoCo.read_block(12, 4, block, POLYMEM_ACCESS_TYPE_Re);
    print_block(block);




    printf("\n============= POLYMEM_SCHEME_ReTr =============\r\n\n");
	// TEST SCHEME
    hls::polymem<DATA_T, log2_p, log2_q, DIM, DIM, POLYMEM_SCHEME_ReTr> a_ReTr;
    for(i = 0; i < DIM; i++)
        for(j = 0; j < DIM; j++)
        	a_ReTr.write(a[i][j], i, j);

    a_ReTr.print_registers();
    a_ReTr.print();

    printf("\n---> SCHEME_ReTr - ACCESS_TYPE_Re (i=3, j=5): \r\n");
    a_ReTr.read_block(3, 5, block, POLYMEM_ACCESS_TYPE_Re);
    print_block(block);

    printf("\n---> SCHEME_ReTr - ACCESS_TYPE_Tr (i=3, j=5): \r\n");
    a_ReTr.read_block(3, 5, block, POLYMEM_ACCESS_TYPE_Tr);
    print_block(block);



    printf("\n============= Test successful! =============\r\n\n");
    return 0;

}
