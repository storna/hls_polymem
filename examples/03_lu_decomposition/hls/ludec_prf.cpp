#include <hls_stream.h>
#include "ap_int.h"
#include "hls_prf.h"

#define DIM 96
#define p 2 // number of rows of the matrix in which physical memory modules (linear registers) are organized in the PRF
#define q 4 // number of columns of the matrix in which physical memory modules (linear registers) are organized in the PRF
#define N_LINEAR_REGISTERS (p * q)

struct data_struct {
    float data;
    bool last;
};


void ludec_prf(hls::stream<float> &s_in, hls::stream<data_struct> &s_out) {
#pragma HLS INTERFACE axis port=s_in
#pragma HLS INTERFACE axis port=s_out
#pragma HLS INTERFACE ap_ctrl_none port=return

#pragma HLS inline region

    hls::prf<float, p, q, DIM, DIM, PRF_SCHEME_RoCo> a;
    hls::prf<float, p, q, p, DIM, PRF_SCHEME_RoCo> u_row;
    hls::prf<float, p, q, p, DIM, PRF_SCHEME_RoCo> l_col;

    float temp_row[N_LINEAR_REGISTERS];
    float temp_col[N_LINEAR_REGISTERS];
#pragma HLS array_partition variable=temp_row complete dim=1
#pragma HLS array_partition variable=temp_col complete dim=1

    data_struct out_data;

    // stream in matrix
    L_IN: for (int i = 0; i < DIM; i++)
        for (int j = 0; j < DIM; j++) {
#pragma HLS PIPELINE II=1
            a.write(s_in.read(), i, j);
        }

    int k = 0, i = 0, j = 0;
    ap_uint<N_LINEAR_REGISTERS> mask;

    // ============= LU Decomposition Loop =============
    L1: for (k = 0; k < DIM; k++) {
        float diag = a.read(k, k);

        // Compute the k L-column and the k U-row
        L2_1: for (i = k; i < DIM - N_LINEAR_REGISTERS; i += N_LINEAR_REGISTERS) {
#pragma HLS PIPELINE II=1

            a.read_block(i, k, temp_col, PRF_ACCESS_TYPE_Co);
            a.read_block(k, i, temp_row, PRF_ACCESS_TYPE_Ro);
            L3: for (int t = 0; t < N_LINEAR_REGISTERS; t++) {
#pragma HLS UNROLL
                temp_col[t] = temp_col[t] / diag;
            }
            l_col.write_block(temp_col, 0, i, PRF_ACCESS_TYPE_Ro);
            u_row.write_block(temp_row, 0, i, PRF_ACCESS_TYPE_Ro);
        }

        // Compute the k L-column and the k U-row: the last block is always executed
        i = DIM - N_LINEAR_REGISTERS;

        a.read_block(i, k, temp_col, PRF_ACCESS_TYPE_Co);
        a.read_block(k, i, temp_row, PRF_ACCESS_TYPE_Ro);
        L2_2: for (int t = 0; t < N_LINEAR_REGISTERS; t++) {
#pragma HLS UNROLL
            temp_col[t] = temp_col[t] / diag;
        }
        l_col.write_block(temp_col, 0, i, PRF_ACCESS_TYPE_Ro);
        u_row.write_block(temp_row, 0, i, PRF_ACCESS_TYPE_Ro);

        // Upgrade the k a-submatrix
        L4: for (i = k; i < DIM; i++) {
#pragma HLS loop_tripcount min=0 max=8 avg=4

            L5_1: for (j = k; j < DIM - N_LINEAR_REGISTERS; j += N_LINEAR_REGISTERS) {
#pragma HLS loop_tripcount min=0 max=8 avg=4
#pragma HLS PIPELINE II=1

                a.loop_dependence_free();
                a.read_block(i, j, temp_row, PRF_ACCESS_TYPE_Ro);
                u_row.read_block(0, j, temp_col, PRF_ACCESS_TYPE_Ro);
                float l_col_val = l_col.read(0, i);

                L6: for (int t = 0; t < N_LINEAR_REGISTERS; t++) {
#pragma HLS UNROLL
                    temp_row[t] = temp_row[t] - l_col_val * temp_col[t];
                }
                a.write_block(temp_row, i, j, PRF_ACCESS_TYPE_Ro);
            }

            // Upgrade the k a-submatrix: the last block is always executed and a mask is applied to avoid overwrite data
            mask = (1 << (DIM - j)) - 1;
            j = DIM - N_LINEAR_REGISTERS;
            a.read_block(i, j, temp_row, PRF_ACCESS_TYPE_Ro);
            u_row.read_block(0, j, temp_col, PRF_ACCESS_TYPE_Ro);
            float l_col_val = l_col.read(0, i);

            L5_2: for (int t = 0; t < N_LINEAR_REGISTERS; t++) {
#pragma HLS UNROLL
                temp_row[t] = temp_row[t] - l_col_val * temp_col[t];
            }
            a.write_block_masked(temp_row, mask, i, j, PRF_ACCESS_TYPE_Ro);
        }

        // Use the k a-row and the k a-column to store partially L and U
        L_LU: for (i = k; i < DIM - N_LINEAR_REGISTERS; i += N_LINEAR_REGISTERS) {
#pragma HLS loop_tripcount min=0 max=8 avg=4
#pragma HLS PIPELINE II=1
            a.loop_dependence_free();
            u_row.read_block(0, i, temp_row, PRF_ACCESS_TYPE_Ro);
            l_col.read_block(0, i, temp_col, PRF_ACCESS_TYPE_Ro);
            a.write_block(temp_col, i, k, PRF_ACCESS_TYPE_Co);
            a.write_block(temp_row, k, i, PRF_ACCESS_TYPE_Ro);
        }

        // Use the k a-row and the k a-column to store partially L and U: the last block is always executed and a mask is applied to avoid overwrite data
        mask = (1 << (DIM - i)) - 1;
        i = DIM - N_LINEAR_REGISTERS;
        u_row.read_block(0, i, temp_row, PRF_ACCESS_TYPE_Ro);
        l_col.read_block(0, i, temp_col, PRF_ACCESS_TYPE_Ro);
        a.write_block_masked(temp_col, mask, i, k, PRF_ACCESS_TYPE_Co);
        a.write_block_masked(temp_row, mask, k, i, PRF_ACCESS_TYPE_Ro);

        // Use the a-diagonal to store the u-diagonal (l-diagonal is always composed by 1.0)
        a.write(u_row.read(0, k), k, k);

    }

    // stream out lu matrix
    L_OUT_L: for (int i = 0; i < DIM; i++)
        for (int j = 0; j < DIM; j++) {
#pragma HLS PIPELINE II=1
            out_data.data = a.read(i, j);
            out_data.last = (i == (DIM - 1) && j == (DIM - 1)) ? 1 : 0;

            s_out.write(out_data);
        }

}
