#ifndef HLS_PRF_H
#define HLS_PRF_H
/*
 * This file contains the C++ implementation of Polymorphic Register File for Vivado HLS
 */

#include "ap_int.h"

#ifndef __SYNTHESIS__
#include <stdlib.h>
#include <stdio.h>
#endif

namespace hls {

#define PRF_SCHEME_ReO  0
#define PRF_SCHEME_ReRo 1
#define PRF_SCHEME_ReCo 2
#define PRF_SCHEME_RoCo 3
#define PRF_SCHEME_ReTr 4

#define PRF_ACCESS_TYPE_Re  0
#define PRF_ACCESS_TYPE_Ro  1
#define PRF_ACCESS_TYPE_Co  2
#define PRF_ACCESS_TYPE_Tr  3

/*
 * TEMPLATE DEFINITION:
 * PRF_DATA_T: data type managed by the PRF
 * p: number of rows of the "virtual-2D" matrix in which linear registers are organized within the PRF
 * q: number of columns of the "virtual-2D" matrix in which linear registers are organized within the PRF
 * N: number of rows of the input matrix, where N % p == 0
 * M: number of columns of the input matrix, where M % q == 0
 * PRF_SCHEME: scheme used to store data into PRF to optimize parallel reads/writes for different access types:
               0: optimize parallel accesses for Rectangles Only (ReO)
               1: optimize parallel accesses both for Rectangles and Rows (ReRo)
               2: optimize parallel accesses both for Rectangles and Columns (ReCo)
               3: optimize parallel accesses both for Rows and Columns (RoCo)
               4: optimize parallel accesses both for Transposed Rectangles (ReTr)
 */
template<typename PRF_DATA_T, int p, int q, int N, int M, int PRF_SCHEME>
class prf {

private:
    static const int N_LINEAR_REGISTERS = (p * q);  // the number of linear registers (equal to the number of "lanes" to read/write data in parallel)
    static const int N_DATA = (N * M);              // number of input data to be stored in the PRF

    static const int LINEAR_REGISTERS_SIZE = (N_DATA / N_LINEAR_REGISTERS); // the size of each linear registers to store all the input data in the PRF

    /*
     * LINEARIZATION OF THE "VIRTUAL-2D" MATRIX THAT CONTAINS THE LINEAR REGISTERS, EACH OF THEM HAS SIZE OF LINEAR_REGISTERS_SIZE
     */
    PRF_DATA_T linearRegisters[N_LINEAR_REGISTERS][LINEAR_REGISTERS_SIZE];


    /*
     * METHODS TO CHECK CORRECT SCHEME DURING THE SIMULATION
     */
    void check_scheme(){
#ifndef __SYNTHESIS__
        if(PRF_SCHEME < 0 || PRF_SCHEME > 4){
            std::cout << "\n" << "ERROR: Unsupported PRF_SCHEME." << "\n" << std::endl;
            exit(-1);
        }
#endif
    }

    /*
     * METHODS TO CHECK CORRECT ACCESS TYPE DURING THE SIMULATION
     */
    void check_access_type(int PRF_ACCESS_TYPE){
#ifndef __SYNTHESIS__
        if(PRF_ACCESS_TYPE < 0 || PRF_ACCESS_TYPE > 3){
            std::cout << "\n" << "ERROR: Unsupported PRF_ACCESS_TYPE." << "\n" << std::endl;
            exit(-1);
        }
        switch(PRF_ACCESS_TYPE){
            case PRF_ACCESS_TYPE_Ro:
                if(PRF_SCHEME != PRF_SCHEME_ReRo && PRF_SCHEME != PRF_SCHEME_RoCo){
                    std::cout << "\n" << "ERROR: Used a PRF_ACCESS_TYPE with the wrong PRF_SCHEME." << "\n" << std::endl;
                    exit(-1);
                }
                break;
            case PRF_ACCESS_TYPE_Co:
                if(PRF_SCHEME != PRF_SCHEME_ReCo && PRF_SCHEME != PRF_SCHEME_RoCo){
                    std::cout << "\n" << "ERROR: Used a PRF_ACCESS_TYPE with the wrong PRF_SCHEME." << "\n" << std::endl;
                    exit(-1);
                }
                break;
        }
#endif
    }

    /*
     * METHOD TO RETRIEVE THE ROW INDEX OF A SPECIFIC LINEAR REGISTER WITHIN THE "VIRTUAL-2D" MATRIX,
     * BY USING DIFFERENT SCHEMES: ReO, ReRo, ReCo, RoCo, ReTr
     */
    int m_v(int i, int j){
        switch(PRF_SCHEME){
            case PRF_SCHEME_ReO:
                return i % p;
                break;
            case PRF_SCHEME_ReRo:
                return (i + (j / q)) % p;
                break;
            case PRF_SCHEME_ReCo:
                return i % p;
                break;
            case PRF_SCHEME_RoCo:
                return (i + (j / q)) % p;
                break;
            case PRF_SCHEME_ReTr:
                if (p < q){
                    return i % p;
                } else {
                    return (i + j - (j % q)) % q;
                }
                break;
        }
    }

    /*
     * METHOD TO RETRIEVE THE COLUMN INDEX OF A SPECIFIC LINEAR REGISTER WITHIN THE "VIRTUAL-2D" MATRIX,
     * BY USING DIFFERENT SCHEMES: ReO, ReRo, ReCo, RoCo, ReTr
     */
    int m_h(int i, int j){
        switch(PRF_SCHEME){
            case PRF_SCHEME_ReO:
                return j % q;
                break;
            case PRF_SCHEME_ReRo:
                return j % q;
                break;
            case PRF_SCHEME_ReCo:
                return ((i / p) + j) % q;
                break;
            case PRF_SCHEME_RoCo:
                return ((i / p) + j) % q;
                break;
            case PRF_SCHEME_ReTr:
                if (p < q) {
                    return (i - i % p + j) % q;
                } else {
                    return j % q;
                }
                break;
        }
    }

    /*
     * METHOD TO RETRIEVE THE INDEX OF A SPECIFIC LINEAR REGISTER WITHIN THE LINEARIZATION OF THE "VIRTUAL-2D" MATRIX,
     * BY USING DIFFERENT SCHEMES: ReO, ReRo, ReCo, RoCo, ReTr
     */
    int m(int i, int j){
#pragma HLS inline region
        return m_v(i, j) * q + m_h(i, j);
    }

    /*
     * METHODS TO RETRIEVE THE DEPTH POSITION WITHIN A SPECIFIC LINEAR REGISTER
     */
    int A_direct(int i, int j) {
        return (i / p) * (M / q) + (j / q);
    }

    /*
     * METHODS TO RETRIEVE THE INDEX WITHIN A SPECIFIC LINEAR REGISTER
     */
    int A_custom(int i, int j, int t, int PRF_ACCESS_TYPE) { // to retrieve the position within the specific linear register
#pragma HLS inline region
        // t = m_v * q + m_h
        int k = (t / q); // m_v
        int l = t % q;   // m_h

        int c_i, c_i1, c_i2, c_j, c_j1, c_j2;
        switch(PRF_ACCESS_TYPE){
            case PRF_ACCESS_TYPE_Ro:
                c_i = 0;
                c_j1 = ((j % q) + (l - ((i / p) % q) - j % q + 2 * q) % q) / q; // added 2 * q to avoid negative numbers in the module
                c_j2 = (k - (i % p) - ((j / q) % p) - c_j1 + 4 * p) % p;        // added 4 * p to avoid negative numbers in the module
                c_j = c_j1 + c_j2;
                break;
            case PRF_ACCESS_TYPE_Co:
                c_i1 = ((i % p) + (k - ((i / q) % p) - i % p + 2 * p) % p) / p; // added 2 * p to avoid negative numbers in the module
                c_i2 = (l - (j % q) - ((i / p) % q) - c_i1 + 4 * q) % q;        // added 4 * q to avoid negative numbers in the module
                c_i = c_i1 + c_i2;
                c_j = 0;
                break;
        }
        return ((i / p) + c_i) * (M / q) + (j / q) + c_j;
    }



public:

    /*
     * CONSTRUCTOR
     */
    prf() {
#pragma HLS ARRAY_PARTITION variable=linearRegisters dim=1 complete
        check_scheme();
    }

    /*
     * DESTRUCTOR
     */
    virtual ~prf() {
    }

    /*
     * METHOD TO PRINT LINEAR REGISTERS CONTENT
     */
    void print() {
#ifndef __SYNTHESIS__
        std::cout << "PRF" << "{" << std::endl;
        for(int i=0; i<N_LINEAR_REGISTERS; i++){
            std::cout << "LinearRegister_" << i << ": [";
            for(int j=0; j<LINEAR_REGISTERS_SIZE; j++){
                if(j != 0)
                    std::cout << "; ";
                std::cout << linearRegisters[i][j];
            }
            std::cout << "]" << std::endl;
        }
        std::cout << "}" << std::endl;
#endif
    }

    /*
     * METHOD TO WRITE ONE DATA INTO PRF BASED ON PRE-SELECTED SCHEME: NOT OPTIMIZED FOR PARALLEL WRITES
     */
    void write(PRF_DATA_T data, int i, int j) {
#pragma HLS inline region
        linearRegisters[m(i, j)][A_direct(i, j)] = data;
    }

    /*
     * METHOD TO READ ONE DATA FROM PRF BASED ON PRE-SELECTED SCHEME: NOT OPTIMIZED FOR PARALLEL READS
     */
    PRF_DATA_T read(int i, int j) {
#pragma HLS inline region
        return linearRegisters[m(i, j)][A_direct(i, j)];
    }

    /*
     * METHOD TO REMOVE HLS FAKE DEPENDENCIES WITHIN A SPECIFIC LOOP
     * WARNING: to be use only when there are not data dependencies
     */
    void loop_dependence_free() {
#pragma HLS inline region
#pragma HLS dependence variable=linearRegisters inter false
#pragma HLS dependence variable=linearRegisters intra false
    }

    /*
     * METHOD TO WRITE DATA IN PARALLEL TO PRF BY USING DIFFERENT ACCESS TYPE: Re, Ro, Co, Tr
     * The m function is executed in separate steps to reduce the crossbar dimension
     */
    void write_block(PRF_DATA_T in[N_LINEAR_REGISTERS], int i, int j, int PRF_ACCESS_TYPE) {
#pragma HLS inline region

        check_access_type(PRF_ACCESS_TYPE);

        PRF_DATA_T tmp[p][q]; // Used to store unordered data
#pragma HLS array_partition variable=tmp complete dim=0
        PRF_DATA_T tmp2[p][q]; // Used to store unordered data
#pragma HLS array_partition variable=tmp2 complete dim=0

        for (int tp = 0; tp < p; tp++)
            for (int tq = 0; tq < q; tq++)
#pragma HLS UNROLL
                switch (PRF_ACCESS_TYPE) {
                case PRF_ACCESS_TYPE_Ro:
                    tmp2[m_v(i, j + tp * q + tq)][tq] = in[tp * q + tq];
                    break;
                case PRF_ACCESS_TYPE_Co:
                    tmp2[tp][m_h(i + tq * p + tp, j)] = in[tq * p + tp];
                    break;
                }

        for (int tp = 0; tp < p; tp++)
            for (int tq = 0; tq < q; tq++)
#pragma HLS UNROLL
                switch (PRF_ACCESS_TYPE) {
                case PRF_ACCESS_TYPE_Ro:
                    tmp[tp][m_h(i, j + tq)] = tmp2[tp][tq];
                    break;
                case PRF_ACCESS_TYPE_Co:
                    tmp[m_v(i + tp, j)][tq] = tmp2[tp][tq];
                    break;
                }

        for (int tp = 0; tp < p; tp++)
            for (int tq = 0; tq < q; tq++)
#pragma HLS UNROLL
                linearRegisters[tp * q + tq][A_custom(i, j, tp * q + tq, PRF_ACCESS_TYPE)] = tmp[tp][tq];

    }

    /*
     * METHOD TO WRITE DATA IN PARALLEL TO PRF BY USING DIFFERENT ACCESS TYPE: Re, Ro, Co, Tr
     */
    void write_block_masked(PRF_DATA_T in[N_LINEAR_REGISTERS], ap_uint<N_LINEAR_REGISTERS> mask, int i, int j, int PRF_ACCESS_TYPE) {
#pragma HLS inline region

        check_access_type(PRF_ACCESS_TYPE);

        ap_uint<N_LINEAR_REGISTERS> mask_tmp;  // Used to store unordered bit
        PRF_DATA_T tmp[N_LINEAR_REGISTERS];    // Used to store unordered data
#pragma HLS array_partition variable=tmp complete dim=1

        PRF_LOOP_ORDER_DATA_TO_WRITE: for (int t = 0; t < N_LINEAR_REGISTERS; t++)
#pragma HLS UNROLL
            switch (PRF_ACCESS_TYPE) {
            case PRF_ACCESS_TYPE_Ro:
                tmp[m(i, j + t)] = in[t];
                mask_tmp[m(i, j + t)] = mask[N_LINEAR_REGISTERS - 1 - t];
                break;
            case PRF_ACCESS_TYPE_Co:
                tmp[m(i + t, j)] = in[t];
                mask_tmp[m(i + t, j)] = mask[N_LINEAR_REGISTERS - 1 - t];
                break;
            }

        PRF_LOOP_WRITE_PARALLEL_DATA: for (int t = 0; t < N_LINEAR_REGISTERS; t++)
#pragma HLS UNROLL
            linearRegisters[t][A_custom(i, j, t, PRF_ACCESS_TYPE)] = mask_tmp[t] ? tmp[t] : linearRegisters[t][A_custom(i, j, t, PRF_ACCESS_TYPE)];
    }


    /*
     * METHOD TO READ DATA IN PARALLEL FROM PRF BY USING DIFFERENT ACCESS TYPE: Re, Ro, Co, Tr
     * The m function is executed in separate steps to reduce the crossbar dimension
     */
    void read_block(int i, int j, PRF_DATA_T out[N_LINEAR_REGISTERS], int PRF_ACCESS_TYPE) {
#pragma HLS inline region

        check_access_type(PRF_ACCESS_TYPE);

        PRF_DATA_T tmp[p][q];  // Used to store unordered data
#pragma HLS array_partition variable=tmp complete dim=0
        PRF_DATA_T tmp2[p][q]; // Used to store unordered data
#pragma HLS array_partition variable=tmp2 complete dim=0

        for (int tp = 0; tp < p; tp++)
            for (int tq = 0; tq < q; tq++)
#pragma HLS UNROLL
                tmp[tp][tq] = linearRegisters[tp * q + tq][A_custom(i, j, tp * q + tq, PRF_ACCESS_TYPE)];

        for (int tp = 0; tp < p; tp++)
            for (int tq = 0; tq < q; tq++)
#pragma HLS UNROLL
                switch (PRF_ACCESS_TYPE) {
                case PRF_ACCESS_TYPE_Ro:
                    tmp2[tp][tq] = tmp[tp][m_h(i, j + tq)];
                    break;
                case PRF_ACCESS_TYPE_Co:
                    tmp2[tp][tq] = tmp[m_v(i + tp, j)][tq];
                    break;
                }

        for (int tp = 0; tp < p; tp++)
            for (int tq = 0; tq < q; tq++)
#pragma HLS UNROLL
                switch (PRF_ACCESS_TYPE) {
                case PRF_ACCESS_TYPE_Ro:
                    out[tp * q + tq] = tmp2[m_v(i, j + tp * q + tq)][tq];
                    break;
                case PRF_ACCESS_TYPE_Co:
                    out[tq * p + tp] = tmp2[tp][m_h(i + tq * p + tp, j)];
                    break;
                }

    }


}; // end class prf

}  // end namespace hls

#endif  // end HLS_PRF_H
