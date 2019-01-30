#include <hls_stream.h>

/*
 * This core is inspired from:
 * A Zynq Accelerator for Floating Point Matrix Multiplication Designed with Vivado HLS
 * https://www.xilinx.com/support/documentation/application_notes/xapp1170-zynq-hls.pdf
 */

#define DIM 96

struct data_struct {
    float data;
    bool last;
};

void mmul(hls::stream<float> &s_in, hls::stream<data_struct> &s_out) {
#pragma HLS INTERFACE axis port=s_in
#pragma HLS INTERFACE axis port=s_out
#pragma HLS INTERFACE ap_ctrl_none port=return

    float a[DIM][DIM];
    float b[DIM][DIM];
    float c[DIM][DIM];

    int const FACTOR = 16;
#pragma HLS array_partition variable=a block factor=4 dim=1
#pragma HLS array_partition variable=a block factor=4 dim=2
#pragma HLS array_partition variable=b block factor=4 dim=1
#pragma HLS array_partition variable=b block factor=4 dim=2

    data_struct out_data;

    // stream in first matrix
    for (int i = 0; i < DIM; i++)
        for (int j = 0; j < DIM; j++) {
#pragma HLS PIPELINE II=1
            a[i][j] = s_in.read();

        }

    // stream in second matrix
    for (int i = 0; i < DIM; i++)
        for (int j = 0; j < DIM; j++) {
#pragma HLS PIPELINE II=1
            b[i][j] = s_in.read();
        }

    // A*B matrix multiplication
    L1: for (int ia = 0; ia < DIM; ++ia)
        L2: for (int ib = 0; ib < DIM; ++ib) {
#pragma HLS PIPELINE II=1
            float sum = 0;
            L3: for (int id = 0; id < DIM; ++id)
                sum += a[ia][id] * b[id][ib];
            c[ia][ib] = sum;
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
    L4: for (int ia = 0; ia < DIM; ++ia)
        L5: for (int ib = 0; ib < DIM; ++ib) {
#pragma HLS PIPELINE II=1
            float sum = 0;
            L6: for (int id = 0; id < DIM; ++id)
                sum += b[ia][id] * a[id][ib];
            c[ia][ib] = sum;
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
