#include <stdio.h>
#include <stdlib.h>
#include <hls_stream.h>

#define DIM 128
#define SIZE (DIM * DIM)

struct data_struct {
    float data;
    bool last;
};

// Test HW function
// 0: mmul
// 1: mmul_prf
#define TEST_HW_FUNCTION 0

void mmul(hls::stream<float> &in, hls::stream<data_struct> &out);
void mmul_prf(hls::stream<float> &in, hls::stream<data_struct> &out);

void mmult_sw(float a[DIM][DIM], float b[DIM][DIM], float out[DIM][DIM]) {
    // matrix multiplication of a A*B matrix
    for (int ia = 0; ia < DIM; ++ia)
        for (int ib = 0; ib < DIM; ++ib) {

            float sum = 0;

            for (int id = 0; id < DIM; ++id)

                sum += a[ia][id] * b[id][ib];

            out[ia][ib] = sum;
        }
}

int main() {
    int ret_val = 0;

    int i, j, err = 0;

    float matOp1[DIM][DIM];
    float matOp2[DIM][DIM];
    float matMult_sw[DIM][DIM];
    float matMult_hw[DIM][DIM];

    /** Matrix Initiation */
    for (i = 0; i < DIM; i++)
        for (j = 0; j < DIM; j++)
            matOp1[i][j] = (float) (i *DIM +  j);

    for (i = 0; i < DIM; i++)
        for (j = 0; j < DIM; j++)
            matOp2[i][j] = (float) (i * j);
    /** End of Initiation */

    // Streams creation
    hls::stream<float> in("in");
    hls::stream<data_struct> out("out");

    for (i = 0; i < DIM; i++)
        for (j = 0; j < DIM; j++)
            in.write(matOp1[i][j]);

    for (i = 0; i < DIM; i++)
        for (j = 0; j < DIM; j++)
            in.write(matOp2[i][j]);

    /* HW Matrix Multiplication A*B e B*A */
    switch(TEST_HW_FUNCTION){
        case 0:
            mmul(in, out);
            break;
        case 1:
            mmul_prf(in, out);
            break;

    }

    // Write Output A*B
    for (i = 0; i < DIM; i++)
        for (j = 0; j < DIM; j++)
            matMult_hw[i][j] = out.read().data;

    /* reference Matrix Multiplication A*B */
    mmult_sw(matOp1, matOp2, matMult_sw);

    /** Matrix comparison A*B */
    for (i = 0; (i < DIM && !err); i++)
        for (j = 0; (j < DIM && !err); j++)
            if (matMult_sw[i][j] != matMult_hw[i][j])
                err++;

    // Write Output B*A
    for (i = 0; i < DIM; i++)
        for (j = 0; j < DIM; j++)
            matMult_hw[i][j] = out.read().data;

    // reference Matrix Multiplication B*A
    mmult_sw(matOp2, matOp1, matMult_sw);

    // Matrix comparison B*A
    for (i = 0; (i < DIM && !err); i++)
        for (j = 0; (j < DIM && !err); j++)
            if (matMult_sw[i][j] != matMult_hw[i][j])
                err++;

    if (err == 0)
        printf("\n============= Matrixes identical ... Test successful! =============\r\n\n");
    else
        printf("\n============= Test failed! =============\r\n\n");

    return 0;

}
