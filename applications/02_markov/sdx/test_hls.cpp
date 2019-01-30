#include <stdio.h>
#include <stdlib.h>

#define PRINT_RESULTS 0
#define DIM 256
#define SIZE (DIM * DIM)

typedef float DATA_TYPE;

void mmul_prf(DATA_TYPE *input, DATA_TYPE *output);

void mmult_sw(DATA_TYPE a[DIM][DIM], DATA_TYPE b[DIM][DIM], DATA_TYPE out[DIM][DIM]) {
    // matrix multiplication of a A*B matrix
    for (int ia = 0; ia < DIM; ++ia)
        for (int ib = 0; ib < DIM; ++ib) {

            DATA_TYPE sum = 0;

            for (int id = 0; id < DIM; ++id)

                sum += a[ia][id] * b[id][ib];

            out[ia][ib] = sum;
        }
}

void A_k_sw(DATA_TYPE a[DIM][DIM], DATA_TYPE out[DIM][DIM]) {

	DATA_TYPE a_temp[DIM][DIM];
	for(int i=0; i<DIM; i++){
		for(int j=0; j<DIM; j++){
			a_temp[i][j] = a[i][j];
		}
	}

	for(int iter=0; iter<8; iter ++){
		mmult_sw(a_temp, a_temp, out);
		for(int i=0; i<DIM; i++){
			for(int j=0; j<DIM; j++){
				a_temp[i][j] = out[i][j];
			}
		}
	}
}

void print_matrix(DATA_TYPE a[DIM][DIM]) {
    // matrix multiplication of a A*B matrix
    printf("\n");
    for (int ia = 0; ia < DIM; ++ia){
        for (int ib = 0; ib < DIM; ++ib) {
            if(ib > 0){
                printf(", ");
            }
            printf("%f", a[ia][ib]);
        }
        printf("\n");
    }
    printf("\n");
}

int main() {
    int ret_val = 0;

    int i, j, err = 0;

    DATA_TYPE A[DIM][DIM];
    DATA_TYPE matMult_sw[DIM][DIM];
    DATA_TYPE matMult_hw[DIM][DIM];

    /** Matrix Initiation */
    for (i = 0; i < DIM; i++)
        for (j = 0; j < DIM; j++)
            A[i][j] = (DATA_TYPE) 1;//(i *DIM +  j);
    /** End of Initiation */

    // Streams creation
    DATA_TYPE *in;
    DATA_TYPE *out;

    in = (DATA_TYPE *) malloc(SIZE*sizeof(DATA_TYPE));
    out = (DATA_TYPE *) malloc(SIZE*sizeof(DATA_TYPE));

    for (i = 0; i < DIM; i++)
        for (j = 0; j < DIM; j++)
            in[i*DIM+j] = A[i][j];


    /* HW Matrix Multiplication A*A */
    mmul_prf(in, out);

    // Read Output A*A
    for (i = 0; i < DIM; i++)
        for (j = 0; j < DIM; j++)
            matMult_hw[i][j] = out[i*DIM+j];

    /* reference Matrix Multiplication A*A */
    A_k_sw(A, matMult_sw);

    if(PRINT_RESULTS){
        print_matrix(matMult_hw);
        print_matrix(matMult_sw);
    }

    /** Matrix comparison A*A */
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
