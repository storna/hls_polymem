#include <hls_stream.h>
#include "params.h"

/*
 * ACCESS TYPEs
 */
#define POLYMEM_ACCESS_TYPE_Re  0 // Rectangle
#define POLYMEM_ACCESS_TYPE_Ro  1 // Row
#define POLYMEM_ACCESS_TYPE_Co  2 // Column
#define POLYMEM_ACCESS_TYPE_Tr  3 // Transposed Rectangle
#define POLYMEM_ACCESS_TYPE_MD  4 // Main Diagonal
#define POLYMEM_ACCESS_TYPE_SD  5 // Secondary Diagonal


int get_row_by_access(int i, int j, int t, int ACCESS_TYPE){
    switch (ACCESS_TYPE) {
		case POLYMEM_ACCESS_TYPE_Ro: return i;
		case POLYMEM_ACCESS_TYPE_Co: return i + t;
		case POLYMEM_ACCESS_TYPE_Re: return i + (t / (N_LINEAR_REGISTERS / 2));
		case POLYMEM_ACCESS_TYPE_Tr: return i + (t / 2);
		case POLYMEM_ACCESS_TYPE_MD: return i + t;
		case POLYMEM_ACCESS_TYPE_SD: return i + t;
    }
	return -1;
}

int get_col_by_access(int i, int j, int t, int ACCESS_TYPE){
    switch (ACCESS_TYPE) {
		case POLYMEM_ACCESS_TYPE_Ro: return j + t;
		case POLYMEM_ACCESS_TYPE_Co: return j;
		case POLYMEM_ACCESS_TYPE_Re: return j + (t % (N_LINEAR_REGISTERS / 2));
		case POLYMEM_ACCESS_TYPE_Tr: return j + (t % 2);
		case POLYMEM_ACCESS_TYPE_MD: return j + t;
		case POLYMEM_ACCESS_TYPE_SD: return j - t;
    }
	return -1;
}


void hlsp_bl_readings(hls::stream<DATA_T> &s_in, hls::stream<data_struct> &s_out) {
#pragma HLS INTERFACE axis port=s_in
#pragma HLS INTERFACE axis port=s_out
#pragma HLS INTERFACE ap_ctrl_none port=return

#pragma HLS inline region

    int const FACTOR = N_LINEAR_REGISTERS;

    // matrix partitioned by row (dim=2) used to store data
    DATA_T a[DIM][DIM];
#pragma HLS array_partition variable=a cyclic factor=FACTOR dim=2


    // array used to store reading coordinates
    int readings[LEN_READINGS];
#pragma HLS array_partition variable=readings cyclic factor=2

    // matrix partitioned by row (dim=2) used to store results
    DATA_T results[N_READINGS][N_LINEAR_REGISTERS];
#pragma HLS array_partition variable=results complete dim=2


    // variable to create output stream data
    data_struct out_data;

    int i, j, r, t, v, h, ACCESS_TYPE;


    // stream in data
in_loop:
	for (i = 0; i < DIM; i++)
        for (j = 0; j < DIM; j++) {
#pragma HLS PIPELINE II=1
            a[i][j] = s_in.read();
        }

    // stream in readings coordinate
in_loop_readings:
	for (r = 0; r < LEN_READINGS; r++) {
#pragma HLS PIPELINE II=1
		readings[r] = (int) s_in.read();
	}

    // process readings
process_loop_Ro:
    for (r = 0; r < N_READINGS / 4; r++) {
#pragma HLS PIPELINE II=1
    	i = readings[(r * 2)];
    	j = readings[(r * 2) + 1];
    	ACCESS_TYPE = POLYMEM_ACCESS_TYPE_Ro;

        for (t = 0; t < N_LINEAR_REGISTERS; t++) {
        	v = get_row_by_access(i, j, t, ACCESS_TYPE);
        	h = get_col_by_access(i, j, t, ACCESS_TYPE);
        	results[r][t] = a[v][h];
        }
	}

    // process readings
process_loop_Re:
    for (r = N_READINGS / 4; r < N_READINGS / 2; r++) {
#pragma HLS PIPELINE II=1
    	i = readings[(r * 2)];
    	j = readings[(r * 2) + 1];
    	ACCESS_TYPE = POLYMEM_ACCESS_TYPE_Re;

        for (t = 0; t < N_LINEAR_REGISTERS; t++) {
        	v = get_row_by_access(i, j, t, ACCESS_TYPE);
        	h = get_col_by_access(i, j, t, ACCESS_TYPE);
        	results[r][t] = a[v][h];
        }
	}

    // process readings
process_loop_MD:
    for (r = N_READINGS / 2; r < N_READINGS * 3 / 4; r++) {
#pragma HLS PIPELINE II=1
    	i = readings[(r * 2)];
    	j = readings[(r * 2) + 1];
    	ACCESS_TYPE = POLYMEM_ACCESS_TYPE_MD;

        for (t = 0; t < N_LINEAR_REGISTERS; t++) {
        	v = get_row_by_access(i, j, t, ACCESS_TYPE);
        	h = get_col_by_access(i, j, t, ACCESS_TYPE);
        	results[r][t] = a[v][h];
        }
	}

    // process readings
process_loop_SD:
    for (r = N_READINGS * 3 / 4; r < N_READINGS; r++) {
#pragma HLS PIPELINE II=1
    	i = readings[(r * 2)];
    	j = readings[(r * 2) + 1];
    	ACCESS_TYPE = POLYMEM_ACCESS_TYPE_SD;

        for (t = 0; t < N_LINEAR_REGISTERS; t++) {
        	v = get_row_by_access(i, j, t, ACCESS_TYPE);
        	h = get_col_by_access(i, j, t, ACCESS_TYPE);
        	results[r][t] = a[v][h];
        }
	}


    // stream out results
out_loop:
	for (r = 0; r < N_RESULTS_BLOCK; r++) {
        for (t = 0; t < N_LINEAR_REGISTERS; t++) {
#pragma HLS PIPELINE II=1
		out_data.data = results[r * (N_READINGS/N_RESULTS_BLOCK)][t];
		out_data.last = (r == (N_RESULTS_BLOCK - 1) && t == (N_LINEAR_REGISTERS - 1)) ? 1 : 0;

		s_out.write(out_data);
        }
    }
}
