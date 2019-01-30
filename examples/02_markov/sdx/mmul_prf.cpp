#include <string.h>
#include <stdio.h>
#include "hls_prf.h"
#include "mmul_prf.h"

#define DIM_LOG2 8
#define DIM (1 << DIM_LOG2)
#define p 4 // number of rows of the matrix in which physical memory modules (linear registers) are organized in the PRF
#define q 4 // number of columns of the matrix in which physical memory modules (linear registers) are organized in the PRF
#define N_LINEAR_REGISTERS (p * q)

#define BR 2 // number of rows of the matrix in which small PRF are organized
#define BC BR // number of columns of the matrix in which small PRF are organized
#define ROW_PER_PRF (DIM/BR)
#define COL_PER_PRF (DIM/BC)

#define N_LINEAR_REGISTERS_LOG2 4 // (1,1)=0 | (1,2)=1 | (2,2)=2 | (2,4)=3 | (4,4)=4 | (4,8)=5 | (8,8)=6 | ...
#define B_LOG2 1 // (1,1)=0 | (2,2)=1 | (4,4)=2 | (8,8)=3 | ...

typedef float DATA_TYPE;

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
#pragma HLS INTERFACE m_axi port=input offset=slave bundle=gmem
#pragma HLS INTERFACE m_axi port=output offset=slave bundle=gmem
#pragma HLS INTERFACE s_axilite port=input bundle=control
#pragma HLS INTERFACE s_axilite port=output bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

#pragma HLS inline region
    check_dimension();

    hls::prf<DATA_TYPE, p, q, ROW_PER_PRF, COL_PER_PRF, PRF_SCHEME_RoCo> A[BR][BC];
#pragma HLS array_partition variable=A complete dim=1
#pragma HLS array_partition variable=A complete dim=2

    DATA_TYPE A_temp[BR][BC][ROW_PER_PRF][COL_PER_PRF];
    int const FACTOR = N_LINEAR_REGISTERS;
#pragma HLS array_partition variable=A_temp complete dim=1
#pragma HLS array_partition variable=A_temp complete dim=2
#pragma HLS array_partition variable=A_temp cyclic factor=FACTOR dim=4

	DATA_TYPE temp_row[BR][BC][N_LINEAR_REGISTERS];
	DATA_TYPE temp_col[BR][BC][N_LINEAR_REGISTERS];
	DATA_TYPE temp_mul[BR][BC][N_LINEAR_REGISTERS*COL_PER_PRF];
	DATA_TYPE temp_sum[BR][BC][COL_PER_PRF / N_LINEAR_REGISTERS];
	//DATA_TYPE temp_sum2[BR][BC][REDUCE_FACTOR];

#pragma HLS array_partition variable=temp_row complete dim=0
#pragma HLS array_partition variable=temp_col complete dim=0
#pragma HLS array_partition variable=temp_mul complete dim=0
#pragma HLS array_partition variable=temp_sum complete dim=0

    int k = 0, i = 0, j = 0, t = 0, bi, bj;

	// Copy in matrix
	for (int br = 0; br < BR; br++) {
		for (int bc = 0; bc < BC; bc++) {
			for (i = 0; i < ROW_PER_PRF; i++) {
				for (j = 0; j < COL_PER_PRF; j += N_LINEAR_REGISTERS) {

					bi = br * ROW_PER_PRF + i;
					bj = bc * COL_PER_PRF + j;

					for (int t = 0; t < N_LINEAR_REGISTERS; t++) {
#pragma HLS PIPELINE II=1
						temp_row[0][0][t] = input[bi * DIM + bj + t];
					}
					A[br][bc].write_block(temp_row[0][0], i, j, PRF_ACCESS_TYPE_Ro);
				}
			}
		}
	}

	for(int iter=0; iter<8; iter ++){
		// A^2
		L1: for (int i = 0; i < ROW_PER_PRF; i++) {
			L2: for (int j = 0; j < COL_PER_PRF; j++) {

				Lreset_3: for (int bi = 0; bi < BR; bi++) {
					#pragma HLS UNROLL
					Lreset_4: for (int bj = 0; bj < BC; bj++) {
						#pragma HLS UNROLL
						Lreset_45: for (int t = 0; t < COL_PER_PRF / N_LINEAR_REGISTERS; t++) {
							#pragma HLS UNROLL
							temp_sum[bi][bj][t] = (DATA_TYPE) 0;
							//temp_sum2[bi][bj][t] = (DATA_TYPE) 0;
						}
					}
				}

				L3: for (int k = 0; k < (COL_PER_PRF / N_LINEAR_REGISTERS); k++) {

//#pragma HLS dependence variable=temp_sum inter false
//#pragma HLS dependence variable=temp_sum intra false
#pragma HLS PIPELINE
					//L3111: for (int qq = 0; qq < REDUCE_FACTOR; qq++)
					//{

						Lreset_5: for (int bi = 0; bi < BR; bi++) {
							Lreset_6: for (int bj = 0; bj < BC; bj++) {
								Lreset_7: for (int t = 0; t < N_LINEAR_REGISTERS; t++) {
									temp_mul[bi][bj][t] = (DATA_TYPE) 0;
								}
							}
						}

						// read rows and cols partions from all the available PRFs
						L4: for (int bi = 0; bi < BR; bi++) {
							L5: for (int bj = 0; bj < BC; bj++) {
								A[bi][bj].read_block(i, (k )*N_LINEAR_REGISTERS, temp_row[bi][bj],
										PRF_ACCESS_TYPE_Ro);
								A[bi][bj].read_block((k )*N_LINEAR_REGISTERS, j, temp_col[bi][bj],
										PRF_ACCESS_TYPE_Co);
							}
						}

						// perform all the products using all the combinations of rows and cols to compute
						L4_2: for (int bi = 0; bi < BR; bi++) {
							L5_2: for (int bj = 0; bj < BC; bj++) {
								L6_2: for (int bk = 0; bk < BC; bk++) {
									L7_2: for (int t = 0; t < N_LINEAR_REGISTERS; t++) {
										temp_mul[bi][bj][t + N_LINEAR_REGISTERS*bk] = temp_row[bi][bk][t] * temp_col[bk][bj][t];
									}
								}
							}
						}


						// reduce the products for all the BR * BC vector products
						L4_3: for (int bi = 0; bi < BR; bi++) {
							L5_3: for (int bj = 0; bj < BC; bj++) {
								temp_sum[bi][bj][k] = reducesum<N_LINEAR_REGISTERS_LOG2 + B_LOG2>(temp_mul[bi][bj]);
							}
						}
					//}
/*
					Lred1: for (int bi = 0; bi < BR; bi++) {
						Lred2: for (int bj = 0; bj < BC; bj++) {
							Lred3: for (int qq = 0; qq < REDUCE_FACTOR; qq++) {
								#pragma HLS PIPELINE
								temp_sum2[bi][bj][qq] += temp_sum[bi][bj][qq];
							}
						}
					}
*/

				}

				// copy the final result of the BR * BC row column products to the output matrix
				// TODO: maybe useful to partition the temporary output matrix as well
				LS_3: for (int bi = 0; bi < BR; bi++) {
					#pragma HLS UNROLL
					LS_4: for (int bj = 0; bj < BC; bj++) {
						#pragma HLS UNROLL
						A_temp[bi][bj][i][j] = reducesum<DIM_LOG2 - B_LOG2 - N_LINEAR_REGISTERS_LOG2>(temp_sum[bi][bj]);
					}
				}
			}
		}


		// Copy back results to PRF
		for (int i = 0; i < ROW_PER_PRF; i++) {
			for (int j = 0; j < COL_PER_PRF; j += N_LINEAR_REGISTERS) {
	#pragma HLS PIPELINE II=1
				for (int bi = 0; bi < BR; bi++) {
					for (int bj = 0; bj < BC; bj++) {
						for (int t = 0; t < N_LINEAR_REGISTERS; t++) {
							temp_row[bi][bj][t] = A_temp[bi][bj][i][j + t];
						}

						A[bi][bj].write_block(temp_row[bi][bj], i, j, PRF_ACCESS_TYPE_Ro);
					}
				}
			}
		}
	}

	// Copy out matrix
	for (int br = 0; br < BR; br++) {
		for (i = 0; i < ROW_PER_PRF; i++) {
			for (int bc = 0; bc < BC; bc++) {
				for (j = 0; j < COL_PER_PRF; j += N_LINEAR_REGISTERS) {

					bi = br * ROW_PER_PRF + i;
					bj = bc * COL_PER_PRF + j;

					for (int t = 0; t < N_LINEAR_REGISTERS; t++) {
#pragma HLS PIPELINE II=1
						output[bi * DIM + bj + t] = A_temp[br][bc][i][j + t];
					}
				}
			}
		}
	}
}

