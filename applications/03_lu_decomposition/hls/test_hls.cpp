#include <stdio.h>
#include <stdlib.h>
#include <hls_stream.h>
#include <cmath>
#include "matrices.h"

#define DIM 96
#define SIZE (DIM * DIM)

struct data_struct {
    float data;
    bool last;
};

void ludec(hls::stream<float> &s_in, hls::stream<data_struct> &s_out);
void ludec_prf(hls::stream<float> &s_in, hls::stream<data_struct> &s_out);

void print_matrix(float a[DIM][DIM]) {
    int i, j;
    printf("[");
    for(i=0; i<DIM; i++){
        printf("[");
        for(j=0; j<DIM; j++){
            if(j > 0)
                printf(", ");
            printf("%f", a[i][j]);
        }
        if(i != (DIM - 1))
                printf("],\n");
        else
            printf("]]\n\n");
    }
}

void mmult_sw(float a[DIM][DIM], float b[DIM][DIM], float out[DIM][DIM]) {
    for (int ia = 0; ia < DIM; ++ia)
        for (int ib = 0; ib < DIM; ++ib) {
            float sum = 0;
            for (int id = 0; id < DIM; ++id)
                sum += a[ia][id] * b[id][ib];
            out[ia][ib] = sum;
        }
}

void ludec_sw(float a[DIM][DIM], float l[DIM][DIM], float u[DIM][DIM]) {
    int k=0, i=0, j=0;

    float a_copy[DIM][DIM];
    for(i=0; i<DIM; i++){
        for(j=0; j<DIM; j++){
            a_copy[i][j] = a[i][j];
        }
    }

    // l, u initialization
    for(i=0; i<DIM; i++){
        for(j=0; j<DIM; j++){
            l[i][j] = 0.0;
            u[i][j] = 0.0;
        }
    }

    for(k=0; k<DIM; k++){
        for(i=k; i<DIM; i++){
            l[i][k] = a_copy[i][k] / a_copy[k][k];
            u[k][i] = a_copy[k][i];
        }
        for(i=k; i<DIM; i++){
            for(j=k; j<DIM; j++){
                a_copy[i][j] = a_copy[i][j] - l[i][k] * u[k][j];
            }
        }
    }
}

int main() {
    int ret_val = 0;

    int i, j, err = 0;

    float a[DIM][DIM];
    for (i = 0; i < DIM; i++)
        for (j = 0; j < DIM; j++)
            a[i][j] = a_96[i][j]; // Initialize a

    float l[DIM][DIM];
    float u[DIM][DIM];

    // ================== SW LU Decomposition ==================// SW execution
    ludec_sw(a, l, u);

    // Check SW results
    float a_copy[DIM][DIM];
    mmult_sw(l, u, a_copy);
    for(i=0; i<DIM; i++){
        for(j=0; j<DIM; j++){
            if(abs(a_copy[i][j] - a[i][j]) > 0.001){
                printf("LU Error: %f != %f\n\n", a_copy[i][j], a[i][j]);
                exit(0);
            }
        }
    }


    // ================== HW LU Decomposition ==================
    float lu_hw[DIM][DIM];
    float l_hw[DIM][DIM];
    float u_hw[DIM][DIM];
    // Streams creation
    hls::stream<float> in("in");
    hls::stream<data_struct> out("out");

    for (i = 0; i < DIM; i++)
        for (j = 0; j < DIM; j++)
            in.write(a[i][j]);

    /* HW LU decomposition */
    ludec(in, out);


    // Read Output L
    for (i = 0; i < DIM; i++)
        for (j = 0; j < DIM; j++)
            lu_hw[i][j] = out.read().data;


    // l, u initialization
    for(i=0; i<DIM; i++){
        for(j=0; j<DIM; j++){
            l_hw[i][j] = 0.0;
            u_hw[i][j] = 0.0;
        }
    }

    // l diagonal initialization
    for(i=0; i<DIM; i++)
        l_hw[i][i] = 1.0;

    // write l
    for(i=0; i<DIM; i++)
        for(j=i+1; j<DIM; j++)
            l_hw[j][i] = lu_hw[j][i];

    // write u
    for(i=0; i<DIM; i++)
        for(j=i; j<DIM; j++)
            u_hw[i][j] = lu_hw[i][j];

    // Check HW results
    for(i=0; i<DIM; i++){
        for(j=0; j<DIM; j++){
            if(abs(l[i][j] - l_hw[i][j]) > 0.001){
                print_matrix(l);
                print_matrix(l_hw);
                printf("\n\nL Error: %f != %f\n\n", l[i][j], l_hw[i][j]);
                exit(0);
            }
        }
    }
    for(i=0; i<DIM; i++){
        for(j=0; j<DIM; j++){
            if(abs(u[i][j] - u_hw[i][j]) > 0.001){
                print_matrix(u);
                print_matrix(u_hw);
                printf("\n\nU Error: %f != %f\n\n", u[i][j], u_hw[i][j]);
                exit(0);
            }
        }
    }

    printf("\n\n============= SUCCESS! =============\n\n");
    return 0;

}
