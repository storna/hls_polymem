#include <hls_stream.h>
#include "ap_int.h"
#include "hls_prf.h"

#define DIM 96

struct data_struct {
    float data;
    bool last;
};


void ludec(hls::stream<float> &s_in, hls::stream<data_struct> &s_out) {
#pragma HLS INTERFACE axis port=s_in
#pragma HLS INTERFACE axis port=s_out
#pragma HLS INTERFACE ap_ctrl_none port=return

#pragma HLS inline region

    float a[DIM][DIM];
    float u_row[DIM];
    float l_col[DIM];

    const int FACTOR = 4;
#pragma HLS array_partition variable=a cyclic factor=FACTOR dim=2
#pragma HLS array_partition variable=u_row cyclic factor=FACTOR dim=1
#pragma HLS array_partition variable=l_col cyclic factor=FACTOR dim=1


    data_struct out_data;

    // stream in matrix
    L_IN: for (int i = 0; i < DIM; i++)
        for (int j = 0; j < DIM; j++) {
#pragma HLS PIPELINE II=1
            a[i][j] = s_in.read();
        }

    int k = 0, i = 0, j = 0;

    // ============= LU Decomposition Loop =============
    L1: for (k = 0; k < DIM; k++) {
        float diag = a[k][k];

        // Compute the k L-column and the k U-row
        L2_1: for (i = k; i < DIM; i ++) {
#pragma HLS PIPELINE II=1 rewind
#pragma HLS UNROLL factor=FACTOR
            l_col[i] = a[i][k] / a[k][k];
            u_row[i] = a[k][i];
        }

        // Upgrade the k a-submatrix
        L4: for (i = k; i < DIM; i++) {

            L5_1: for (j = k; j < DIM; j ++) {
#pragma HLS PIPELINE II=1 rewind
#pragma HLS UNROLL factor=FACTOR
#pragma HLS dependence variable=a inter false
#pragma HLS dependence variable=a intra false
                a[i][j] = a[i][j] - l_col[i] * u_row[j];
            }
        }

        // Use the k a-row and the k a-column to store partially L and U
        L_LU: for (i = k; i < DIM; i ++) {
#pragma HLS PIPELINE II=1 rewind
#pragma HLS UNROLL factor=FACTOR
            a[i][k] = l_col[i];
            a[k][i] = u_row[i];
        }

        // Use the a-diagonal to store the u-diagonal (l-diagonal is always composed by 1.0)
        a[k][k] = u_row[k];

    }

    // stream out lu matrix
    L_OUT_L: for (int i = 0; i < DIM; i++)
        for (int j = 0; j < DIM; j++) {
#pragma HLS PIPELINE II=1
            out_data.data = a[i][j];
            out_data.last = (i == (DIM - 1) && j == (DIM - 1)) ? 1 : 0;

            s_out.write(out_data);
        }

}
