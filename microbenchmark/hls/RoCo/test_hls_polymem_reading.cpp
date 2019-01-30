#include <stdio.h>
#include <stdlib.h>
#include <hls_stream.h>
#include "params.h"

void polymem_readings(hls::stream<DATA_T> &in, hls::stream<data_struct> &out);
void hlsp_bl_readings(hls::stream<DATA_T> &in, hls::stream<data_struct> &out);

void print_matrix(DATA_T a[DIM][DIM]){

	int i, j;

    for(i = 0; i < DIM; i++){
        for(j = 0; j < DIM; j++){
        	std::cout << a[i][j] << "\t";
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}


int main() {

    int i, j, min, max, err = 0;

    DATA_T data[DIM][DIM];

    /** Matrix Initiation */
    for (i = 0; i < DIM; i++)
        for (j = 0; j < DIM; j++)
            data[i][j] = (DATA_T) (i * DIM +  j);
    /** End of Initiation */

    //print_matrix(data);

    DATA_T readings[LEN_READINGS];
    for(i = 0; i < LEN_READINGS * 2 / 3; i++){
        min = 0;
        max = DIM - N_LINEAR_REGISTERS;
    	readings[i] = (DATA_T) (min + (int) ((DATA_T) rand() / ((DATA_T) RAND_MAX / (max - min))));
    }
    for(i = LEN_READINGS * 2 / 3; i < LEN_READINGS; i+=2){
        min = 0;
        max = DIM - N_LINEAR_REGISTERS;
    	readings[i] = (DATA_T) ((int) (min + (int) ((DATA_T) rand() / ((DATA_T) RAND_MAX / (max - min)))) / p) * p;
    	readings[i + 1] = (DATA_T) ((int) (min + (int) ((DATA_T) rand() / ((DATA_T) RAND_MAX / (max - min)))) / q) * q;
    }

    for(i = 0; i < LEN_READINGS; i++){
    	std::cout << readings[i] << ", ";
    }
	std::cout << std::endl;


    // Streams creation
    hls::stream<DATA_T> in("in");
    hls::stream<data_struct> out("out");
    hls::stream<data_struct> out_2("out_2");

    // Streams input data
    for (i = 0; i < DIM; i++)
        for (j = 0; j < DIM; j++)
            in.write(data[i][j]);

    // Streams readings commands
	for (j = 0; j < LEN_READINGS; j++){
        in.write(readings[j]);
	}

    /* HW execution */
    polymem_readings(in, out);

    // Streams input data
    for (i = 0; i < DIM; i++)
        for (j = 0; j < DIM; j++)
            in.write(data[i][j]);



    // Streams readings commands
	for (j = 0; j < LEN_READINGS; j++){
        in.write(readings[j]);
	}


    /* HW execution */
	hlsp_bl_readings(in, out_2);

    // Write Output
    data_struct out_data;
    data_struct out_data_2;
	for (j = 0; j < N_RESULTS; j++){
		out_data = out.read();
		out_data_2 = out_2.read();
		if(out_data.data != out_data_2.data)
			err++;
		//std::cout << err << "\t" << out_data.data << "\t last_" << out_data.last << "\t\t" << out_data_2.data << "\t last_" << out_data_2.last << std::endl;
	}

    return (err != 0);

}
