#include <hls_stream.h>
#include "hls_prf.h"

#define DIM 128
#define p 4 // number of rows of the matrix in which physical memory modules (linear registers) are organized in the PRF
#define q 8 // number of columns of the matrix in which physical memory modules (linear registers) are organized in the PRF
#define N_LINEAR_REGISTERS (p * q)

struct data_struct {
    float data;
    bool last;
};

void mmul_prf(hls::stream<float> &s_in, hls::stream<data_struct> &s_out) {
#pragma HLS INTERFACE axis port=s_in
#pragma HLS INTERFACE axis port=s_out
#pragma HLS INTERFACE ap_ctrl_none port=return

#pragma HLS inline region

    hls::prf<float, p, q, DIM, DIM, PRF_SCHEME_RoCo> a;
    hls::prf<float, p, q, DIM, DIM, PRF_SCHEME_RoCo> b;
    float c[DIM][DIM];


    float temp_row[N_LINEAR_REGISTERS];
    float temp_col[N_LINEAR_REGISTERS];
#pragma HLS array_partition variable=temp_row complete dim=1
#pragma HLS array_partition variable=temp_col complete dim=1

    data_struct out_data;

    // stream in first matrix
    for (int i = 0; i < DIM; i++)
        for (int j = 0; j < DIM; j++) {
#pragma HLS PIPELINE II=1
            a.write(s_in.read(), i, j);

        }

    // stream in second matrix
    for (int i = 0; i < DIM; i++)
        for (int j = 0; j < DIM; j++) {
#pragma HLS PIPELINE II=1
            b.write(s_in.read(), i, j);
        }

    // A*B matrix multiplication
    L1: for (int ia = 0; ia < DIM; ++ia){
        L2: for (int ib = 0; ib < DIM; ++ib) {
#pragma HLS PIPELINE II=1
            float sum = 0;
            L3: for (int id = 0; id < DIM; id += N_LINEAR_REGISTERS) {
                a.read_block(ia, id, temp_row, PRF_ACCESS_TYPE_Ro);
                b.read_block(id, ib, temp_col, PRF_ACCESS_TYPE_Co);
                L4: for (int t = 0; t < N_LINEAR_REGISTERS; t++) {
                    sum += temp_row[t] * temp_col[t];
                }
            }
            c[ia][ib] = sum;
        }
    }

    // stream out result matrix
    for (int i = 0; i < DIM; i++)
        for (int j = 0; j < DIM; j++) {
#pragma HLS PIPELINE II=1
            out_data.data = c[i][j];
            out_data.last = 0;

            s_out.write(out_data);
        }


    // B*A matrix multiplication
    L5: for (int ia = 0; ia < DIM; ++ia){
        L6: for (int ib = 0; ib < DIM; ++ib) {
#pragma HLS PIPELINE II=1
            float sum = 0;
            L7: for (int id = 0; id < DIM; id += N_LINEAR_REGISTERS) {
                b.read_block(ia, id, temp_row, PRF_ACCESS_TYPE_Ro);
                a.read_block(id, ib, temp_col, PRF_ACCESS_TYPE_Co);
                L8: for (int t = 0; t < N_LINEAR_REGISTERS; t++) {
                    sum += temp_row[t] * temp_col[t];
                }
            }
            c[ia][ib] = sum;
        }
    }

    // stream out result matrix
    for (int i = 0; i < DIM; i++)
        for (int j = 0; j < DIM; j++) {
#pragma HLS PIPELINE II=1
            out_data.data = c[i][j];
            out_data.last = (i == (DIM - 1) && j == (DIM - 1)) ? 1 : 0;

            s_out.write(out_data);
        }

}
