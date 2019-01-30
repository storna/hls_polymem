#include <hls_stream.h>
#include "hls_polymem.h"
#include "params.h"

void polymem_readings(hls::stream<DATA_T> &s_in, hls::stream<data_struct> &s_out) {
#pragma HLS INTERFACE axis port=s_in
#pragma HLS INTERFACE axis port=s_out
#pragma HLS INTERFACE ap_ctrl_none port=return

#pragma HLS inline region


    int const FACTOR = N_LINEAR_REGISTERS;

    // polymem used to store data
    hls::polymem<DATA_T, log2_p, log2_q, DIM, DIM, POLYMEM_SCHEME> a;

    // array used to store reading coordinates
    int readings[LEN_READINGS];
#pragma HLS array_partition variable=readings cyclic factor=2

    // matrix partitioned by row (dim=2) used to store results
    DATA_T results[N_READINGS][N_LINEAR_REGISTERS];
#pragma HLS array_partition variable=results complete dim=2

    // variable to create output stream data
    data_struct out_data;

    int i, j, r, t;

    // stream in data
in_loop:
    for (i = 0; i < DIM; i++)
        for (j = 0; j < DIM; j++) {
#pragma HLS PIPELINE II=1
            a.write(s_in.read(), i, j);
        }

    // stream in readings coordinate
in_loop_readings:
	for (int r = 0; r < LEN_READINGS; r++) {
#pragma HLS PIPELINE II=1
		readings[r] = (int) s_in.read();
	}

    // process readings
process_loop_Co:
    for (r = 0; r < N_READINGS / 3; r++) {
#pragma HLS PIPELINE II=1
    	i = readings[(r * 2)];
    	j = readings[(r * 2) + 1];

        a.read_block(i, j, results[r], POLYMEM_ACCESS_TYPE_Co);
	}

    // process readings
process_loop_Ro:
    for (r = N_READINGS / 3; r < N_READINGS * 2 / 3; r++) {
#pragma HLS PIPELINE II=1
    	i = readings[(r * 2)];
    	j = readings[(r * 2) + 1];

        a.read_block(i, j, results[r], POLYMEM_ACCESS_TYPE_Ro);
	}

    // process readings
process_loop_Re:
    for (r = N_READINGS * 2 / 3; r < N_READINGS; r++) {
#pragma HLS PIPELINE II=1
    	i = readings[(r * 2)];
    	j = readings[(r * 2) + 1];

        a.read_block(i, j, results[r], POLYMEM_ACCESS_TYPE_Re);
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
