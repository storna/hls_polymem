#include <string.h>
#include <stdio.h>
#include "hls_prf.h"
//#include "mmul_prf.h"

#define DIM 32
#define p 2 // number of rows of the matrix in which physical memory modules (linear registers) are organized in the PRF
#define q 2 // number of columns of the matrix in which physical memory modules (linear registers) are organized in the PRF
#define N_LINEAR_REGISTERS (p * q)

#define N_LINEAR_REGISTERS_LOG2 2

typedef double DATA_TYPE;

template<int log2_size> DATA_TYPE reducesum(DATA_TYPE data[1 << log2_size])
{
	#pragma INLINE

	DATA_TYPE tmp[(1 << log2_size)*2-1];
	#pragma HLS ARRAY_PARTITION variable=tmp complete dim=0

	for(int i = 0; i < 1 << log2_size; i++)
	{
		#pragma HLS UNROLL
		tmp[i] = data[i];
	}

	int p1 = 0, p2 = 1 << log2_size;
	for(int e = log2_size - 1; e >= 0; e--)
	{
		#pragma HLS UNROLL
		for(int i = 0; i < 1 << e; i++) {
			#pragma HLS UNROLL
			tmp[i + p2] = tmp[i*2 + p1] + tmp[i*2 + 1 + p1];
		}
		p1 = p2;
		p2 += (1 << e);
	}

	return tmp[(1 << log2_size)*2 - 2];
}


void check_dimension(){
#ifndef __SYNTHESIS__
    if(DIM % N_LINEAR_REGISTERS != 0){
        std::cout << "\n" << "ERROR: N_LINEAR_REGISTERS must be a multiple of DIM." << "\n" << std::endl;
        exit(-1);
    }
#endif
}

void mmul_prf(DATA_TYPE *input, DATA_TYPE *output)
{
#pragma HLS INTERFACE m_axi port=input offset=slave bundle=gmem1
#pragma HLS INTERFACE m_axi port=output offset=slave bundle=gmem2
#pragma HLS INTERFACE s_axilite port=input bundle=control
#pragma HLS INTERFACE s_axilite port=output bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

#pragma HLS inline region
    check_dimension();

    hls::prf<DATA_TYPE, p, q, DIM, DIM, PRF_SCHEME_RoCo> A;
    DATA_TYPE A_temp[DIM][DIM];

    int const FACTOR = N_LINEAR_REGISTERS;
#pragma HLS array_partition variable=A_temp block factor=FACTOR dim=2

    DATA_TYPE temp_row[N_LINEAR_REGISTERS];
    DATA_TYPE temp_col[N_LINEAR_REGISTERS];
    DATA_TYPE temp_mul[N_LINEAR_REGISTERS];
#pragma HLS array_partition variable=temp_row complete dim=1
#pragma HLS array_partition variable=temp_col complete dim=1
#pragma HLS array_partition variable=temp_mul complete dim=1

    int k = 0, i = 0, j = 0, t = 0;

    // Copy in matrix
    L_IN1: for (i = 0; i < DIM; i++) {
    	L_IN2: for (j = 0; j < DIM; j += N_LINEAR_REGISTERS) {
			for(int t = 0; t < N_LINEAR_REGISTERS; t++) {
				#pragma HLS PIPELINE II=1
				temp_row[t] = input [i * DIM + j + t];
			}
			A.write_block(temp_row, i, j, PRF_ACCESS_TYPE_Ro);
    	}
    }

    // A^2
    L1: for (int ia = 0; ia < DIM; ++ia){
        L2: for (int ib = 0; ib < DIM; ++ib) {
        	A_temp[ia][ib] = 0;
            L3: for (int id = 0; id < DIM; id += N_LINEAR_REGISTERS) {
#pragma HLS PIPELINE II=1
                A.read_block(ia, id, temp_row, PRF_ACCESS_TYPE_Ro);
                A.read_block(id, ib, temp_col, PRF_ACCESS_TYPE_Co);
                L4: for (int t = 0; t < N_LINEAR_REGISTERS; t++) {
                	temp_mul[t] = temp_row[t] * temp_col[t];
                }
                A_temp[ia][ib] += reducesum<N_LINEAR_REGISTERS_LOG2>(temp_mul);
            }
        }
    }

    // Copy back results to PRF
    L_CP1: for (int i = 0; i < DIM; ++i){
        L_CP2: for (int t = 0; t < DIM; t += N_LINEAR_REGISTERS) {
#pragma HLS PIPELINE II=1
            A.write_block(&A_temp[i][t], i, t, PRF_ACCESS_TYPE_Ro);
        }
    }

    // A.print();

    // Copy out matrix
    for (i = 0; i < DIM; i++) {
        for (j = 0; j < DIM; j += N_LINEAR_REGISTERS) {
			A.read_block(i, j, temp_row, PRF_ACCESS_TYPE_Ro);
			for(int t = 0; t < N_LINEAR_REGISTERS; t++) {
#pragma HLS PIPELINE II=1
				output[i * DIM + j + t] = temp_row[t];
			}
        }
    }

}
