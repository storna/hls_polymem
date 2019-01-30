#ifndef HLS_POLYMEM_H
#define HLS_POLYMEM_H
/*
 * This file contains the C++ implementation of Polymorphic Register File for Vivado HLS
 *
 * Copyright (C) 2019 Luca Stornaiuolo. All rights reserved.
 */

#include "ap_int.h"

#ifndef __SYNTHESIS__
#include <stdlib.h>
#include <stdio.h>
#endif

namespace hls {

/*
 * SCHEME DEFINITION:
 */
// RectangleOnly
// supports unrestricted conflict-free access for any rectangle access of size (p × q).
#define POLYMEM_SCHEME_ReO  0

// RectangleRow
// supports unrestricted conflict-free access for any rectangular access of size (p × q) or row access of length (p · q);
// supports unrestricted conflict-free access for any main diagonal of size (p · q), if p and (q + 1) are co-prime;
// supports unrestricted conflict-free access for any secondary diagonal of size (p · q), if p and (q - 1) are co-prime.
#define POLYMEM_SCHEME_ReRo 1

// RectangleColumn
// supports unrestricted conflict-free access for any rectangular access of size (p × q) or column access of length (p · q);
// supports unrestricted conflict-free access for any main diagonal of size (p · q), if (p + 1) and q are co-prime;
// supports unrestricted conflict-free access for any secondary diagonal of size (p · q), if (p - 1) and q are co-prime.
#define POLYMEM_SCHEME_ReCo 2

// RowColumn
// supports unrestricted conflict-free access for any row or column access of size (p · q);
// supports conflict-free access for any aligned rectangle access of size (p × q), if either (i % p = 0) or (j % q = 0).
#define POLYMEM_SCHEME_RoCo 3

// RectangleTrasposed
// supports unrestricted conflict-free access for any rectangle access of size (p × q) or (q × p),
// assuming (q % p = 0) if (p < q) and (p % q = 0) if (q < p).
#define POLYMEM_SCHEME_ReTr 4


/*
 * ACCESS TYPEs
 */
#define POLYMEM_ACCESS_TYPE_Re  0 // Rectangle
#define POLYMEM_ACCESS_TYPE_Ro  1 // Row
#define POLYMEM_ACCESS_TYPE_Co  2 // Column
#define POLYMEM_ACCESS_TYPE_Tr  3 // Transposed Rectangle
#define POLYMEM_ACCESS_TYPE_MD  4 // Main Diagonal
#define POLYMEM_ACCESS_TYPE_SD  5 // Secondary Diagonal

/*
 * TEMPLATE DEFINITION:
 * POLYMEM_DATA_T: data type managed by the POLYMEM
 * p: number of rows of the "virtual-2D" matrix in which linear registers are organized within the POLYMEM
 * q: number of columns of the "virtual-2D" matrix in which linear registers are organized within the POLYMEM
 * log2_p: log2(p)
 * log2_q: log2(q)
 * N: number of rows of the input matrix, where N % p == 0
 * M: number of columns of the input matrix, where M % q == 0
 * POLYMEM_SCHEME: scheme used to store data into POLYMEM to optimize parallel reads/writes for different access types:
               0: optimize parallel accesses for Rectangles Only (ReO)
               1: optimize parallel accesses both for Rectangles and Rows (ReRo)
               2: optimize parallel accesses both for Rectangles and Columns (ReCo)
               3: optimize parallel accesses both for Rows and Columns (RoCo)
               4: optimize parallel accesses both for Transposed Rectangles (ReTr)
 */
template<typename POLYMEM_DATA_T, int log2_p, int log2_q, int N, int M, int POLYMEM_SCHEME>
class polymem {

private:
    static const int p = 1 << log2_p;  // power of 2 using bit operations
    static const int q = 1 << log2_q;  // power of 2 using bit operations

    static const int N_LINEAR_REGISTERS = (p * q);  // the number of linear registers (equal to the number of "lanes" to read/write data in parallel)
    static const int N_DATA = (N * M);              // number of input data to be stored in the POLYMEM

    static const int LINEAR_REGISTERS_SIZE = (N_DATA / N_LINEAR_REGISTERS); // the size of each linear registers to store all the input data in the POLYMEM

    /*
     * LINEARIZATION OF THE "VIRTUAL-2D" MATRIX THAT CONTAINS THE LINEAR REGISTERS, EACH OF THEM HAS SIZE OF LINEAR_REGISTERS_SIZE
     */
    POLYMEM_DATA_T linearRegisters[N_LINEAR_REGISTERS][LINEAR_REGISTERS_SIZE];


    /*
     * METHODS TO CHECK CORRECT SCHEME DURING THE SIMULATION
     */
    void check_scheme(){
#ifndef __SYNTHESIS__
        if(N < N_LINEAR_REGISTERS || M < N_LINEAR_REGISTERS){
            std::cout << "\n" << "ERROR: unsatisfied rule ((N > p * q) && (M > p * q))." << "\n" << std::endl;
            exit(-1);
        }
        if(N % p != 0 || M % q != 0){
            std::cout << "\n" << "ERROR: unsatisfied rule ((N % p == 0) && (M % q == 0))." << "\n" << std::endl;
            exit(-1);
        }
        if(POLYMEM_SCHEME < 0 || POLYMEM_SCHEME > 4){
            std::cout << "\n" << "ERROR: Unsupported POLYMEM_SCHEME." << "\n" << std::endl;
            exit(-1);
        }
#endif
    }

    /*
     * METHODS TO CHECK CORRECT ACCESS TYPE DURING THE SIMULATION
     */
    void check_access_type(int POLYMEM_ACCESS_TYPE){
#ifndef __SYNTHESIS__
        if(POLYMEM_ACCESS_TYPE < 0 || POLYMEM_ACCESS_TYPE > 5){
            std::cout << "\n" << "ERROR: Unsupported POLYMEM_ACCESS_TYPE." << "\n" << std::endl;
            exit(-1);
        }
        switch(POLYMEM_ACCESS_TYPE){
            case POLYMEM_ACCESS_TYPE_Ro:
                if(POLYMEM_SCHEME != POLYMEM_SCHEME_ReRo && POLYMEM_SCHEME != POLYMEM_SCHEME_RoCo){
                    std::cout << "\n" << "ERROR: Used a POLYMEM_ACCESS_TYPE with the wrong POLYMEM_SCHEME." << "\n" << std::endl;
                    exit(-1);
                }
                break;
            case POLYMEM_ACCESS_TYPE_Co:
                if(POLYMEM_SCHEME != POLYMEM_SCHEME_ReCo && POLYMEM_SCHEME != POLYMEM_SCHEME_RoCo){
                    std::cout << "\n" << "ERROR: Used a POLYMEM_ACCESS_TYPE with the wrong POLYMEM_SCHEME." << "\n" << std::endl;
                    exit(-1);
                }
                break;
        }
#endif
    }

    /*
     * METHOD TO COMPUTE floor(n/p) BY USING BIT OPERATIONS (VALID IF p IS A POWER OF 2)
     */
    int floor_p(int n){
    	return n >> log2_p;
    }

    /*
     * METHOD TO COMPUTE floor(n/q) BY USING BIT OPERATIONS (VALID IF q IS A POWER OF 2)
     */
    int floor_q(int n){
    	return n >> log2_q;
    }

    /*
     * METHOD TO COMPUTE (n % p) BY USING BIT OPERATIONS (VALID IF p IS A POWER OF 2)
     */
    int mod_p(int n){
        return n & (p-1);
    }

    /*
     * METHOD TO COMPUTE (n % q) BY USING BIT OPERATIONS (VALID IF q IS A POWER OF 2)
     */
    int mod_q(int n){
        return n & (q-1);
    }


    /*
     * METHODS TO RETRIEVE OMEGA USING LOOKUP TABLES
     */
    /*
    int omega(int a, int b) {
      if (b == 0)
        return a;
      else
        return omega(b, a % b);
    }
    */
    int omega_q_1(int p, int q) {
    	 switch(p){
        	case 2:
				switch(q){
					case 2: return 1;
					case 4: return 1;
					case 8: return 1;
				}
				break;
			case 4:
				switch(q){
					case 2: return 3;
					case 4: return 1;
					case 8: return 1;
				}
				break;
        }
    	 return -1;
    }
    int omega_q_m1(int p, int q) {
    	 switch(p){
        	case 2:
				switch(q){
					case 2: return 1;
					case 4: return 1;
					case 8: return 1;
				}
				break;
			case 4:
				switch(q){
					case 2: return 1;
					case 4: return 3;
					case 8: return 3;
				}
				break;
        }
    	 return -1;
    }
    int omega_p_1(int p, int q) {
    	 switch(p){
        	case 2:
				switch(q){
					case 2: return 1;
					case 4: return 3;
					case 8: return 3;
				}
				break;
			case 4:
				switch(q){
					case 2: return 1;
					case 4: return 1;
					case 8: return 5;
				}
				break;
        }
    	 return -1;
    }
    int omega_p_m1(int p, int q) {
    	 switch(p){
        	case 2:
				switch(q){
					case 2: return 1;
					case 4: return 1;
					case 8: return 1;
				}
				break;
			case 4:
				switch(q){
					case 2: return 1;
					case 4: return 3;
					case 8: return 3;
				}
				break;
        }
    	 return -1;
    }


    /*
     * METHOD TO RETRIEVE THE ROW INDEX OF A SPECIFIC LINEAR REGISTER WITHIN THE "VIRTUAL-2D" MATRIX,
     * BY USING DIFFERENT SCHEMES: ReO, ReRo, ReCo, RoCo, ReTr
     */
    int m_v(int i, int j){
#pragma HLS inline region

        switch(POLYMEM_SCHEME){
            case POLYMEM_SCHEME_ReO:
                return mod_p(i);
                break;
            case POLYMEM_SCHEME_ReRo:
                return mod_p(i + floor_q(j));
                break;
            case POLYMEM_SCHEME_ReCo:
                return mod_p(i);
                break;
            case POLYMEM_SCHEME_RoCo:
                return mod_p(i + floor_q(j));
                break;
            case POLYMEM_SCHEME_ReTr:
                if (p < q){
                    return mod_p(i);
                } else {
                    return mod_p(i + j - mod_q(j));
                }
                break;
        }
    }

    /*
     * METHOD TO RETRIEVE THE COLUMN INDEX OF A SPECIFIC LINEAR REGISTER WITHIN THE "VIRTUAL-2D" MATRIX,
     * BY USING DIFFERENT SCHEMES: ReO, ReRo, ReCo, RoCo, ReTr
     */
    int m_h(int i, int j){
#pragma HLS inline region

        switch(POLYMEM_SCHEME){
            case POLYMEM_SCHEME_ReO:
                return mod_q(j);
                break;
            case POLYMEM_SCHEME_ReRo:
                return mod_q(j);
                break;
            case POLYMEM_SCHEME_ReCo:
                return mod_q(floor_p(i) + j);
                break;
            case POLYMEM_SCHEME_RoCo:
                return mod_q(floor_p(i) + j);
                break;
            case POLYMEM_SCHEME_ReTr:
                if (p < q) {
                    return mod_q(i - mod_p(i) + j);
                } else {
                    return mod_q(j);
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

    int A_direct(int i, int j) {
#pragma HLS inline region
        return floor_p(i) * (M / q) + floor_q(j);
    }


    /*
     * METHODS TO RETRIEVE THE INDEX WITHIN A SPECIFIC LINEAR REGISTER
     */
    int A_custom(int i, int j, int k, int l, int POLYMEM_ACCESS_TYPE) { // to retrieve the position within the specific linear register
#pragma HLS inline region

        int c_i, c_i1, c_i2, c_j, c_j1, c_j2, delta;

        switch(POLYMEM_SCHEME){
        	case POLYMEM_SCHEME_ReO:
				switch(POLYMEM_ACCESS_TYPE){
					case POLYMEM_ACCESS_TYPE_Re:
						c_i = mod_p(i) > k ? 1 : 0;
						c_j = mod_q(j) > l ? 1 : 0;
						break;
				}
				break;

			case POLYMEM_SCHEME_ReRo:
				switch(POLYMEM_ACCESS_TYPE){
					case POLYMEM_ACCESS_TYPE_Re:
						c_j = mod_q(j) > l ? 1 : 0;
						c_i = floor_p(mod_p(i) + mod_p(k - mod_p(floor_q(j)) - c_j - mod_p(i)));
						break;
					case POLYMEM_ACCESS_TYPE_Ro:
						c_i = 0;
						c_j1 = mod_q(j) > l ? 1 : 0;
						c_j2 = mod_p(k - mod_p(i) - mod_p(floor_q(j)) - c_j1);
						c_j = c_j1 + c_j2;
						break;
					case POLYMEM_ACCESS_TYPE_MD:
						c_j1 = mod_q(j) > l ? 1 : 0;
						delta = k - mod_p(i) - mod_p(mod_q(l - mod_q(j))) - c_j1 - mod_p(floor_q(j));
						c_j2 = mod_p(mod_p(delta) * omega_q_1(p, q));
						c_j = c_j1 + c_j2;
						c_i = floor_p(mod_p(i) + mod_q(l - mod_q(j)) + q * c_j2);
						break;
					case POLYMEM_ACCESS_TYPE_SD:
						c_j1 = mod_q(j) < l ? -1 : 0;
						delta = k - mod_p(i) - mod_p(mod_q(mod_q(j) - l)) - mod_p(c_j1) - mod_p(floor_q(j));
						c_j2 = mod_p(mod_p(delta) * omega_q_m1(p, q));
						c_j = c_j1 - c_j2;
						c_i = floor_p(mod_p(i) + mod_q(mod_q(j) - l) + q * c_j2);
						break;
				}
				break;

				case POLYMEM_SCHEME_ReCo:
					switch(POLYMEM_ACCESS_TYPE){
						case POLYMEM_ACCESS_TYPE_Re:
							c_i = mod_p(i) > k ? 1 : 0;
							c_j = floor_q(mod_q(j) + mod_q(l - mod_q(j) - mod_q(floor_p(i)) - c_i));
							break;
						case POLYMEM_ACCESS_TYPE_Co:
							c_j = 0;
							c_i1 = mod_p(i) > k ? 1 : 0;
							c_i2 = mod_q(l - mod_q(floor_p(i)) - mod_q(j) - c_i1);
							c_i = c_i1 + c_i2;
							break;
						case POLYMEM_ACCESS_TYPE_MD:
							c_i1 = mod_p(i) > k ? 1 : 0;
							delta = l - mod_q(j) - mod_q(mod_p(k - mod_p(i))) - c_i1 - mod_q(floor_p(i));
							c_i2 = mod_q(mod_q(delta) * omega_p_1(p, q));
							c_i = c_i1 + c_i2;
							c_j = floor_q(mod_q(j) + mod_p(k - mod_p(i)) + p * c_i2);
							break;
						case POLYMEM_ACCESS_TYPE_SD:
							c_i1 = mod_p(i) > k ? 1 : 0;
							delta = c_i1 + mod_q(floor_p(i)) + mod_q(j) - (mod_q(mod_p(k - mod_p(i)))) - l;
							c_i2 = mod_q(mod_q(delta) * omega_p_m1(p, q));
							c_i = c_i1 + c_i2;
							c_j = floor_q(mod_q(j) - mod_p(k - mod_p(i)) - p * c_i2);
							break;
					}
					break;

			case POLYMEM_SCHEME_RoCo:
				switch(POLYMEM_ACCESS_TYPE){
					case POLYMEM_ACCESS_TYPE_Ro:
						c_i = 0;
						c_j1 = floor_q(mod_q(j) + mod_q(l - mod_q(floor_p(i)) - mod_q(j)));
						c_j2 = mod_p(k - mod_p(i) - mod_p(floor_q(j)) - c_j1);
						c_j = c_j1 + c_j2;
						break;
					case POLYMEM_ACCESS_TYPE_Co:
						c_i1 = floor_p(mod_p(i) + mod_p(k - mod_p(floor_q(j)) - mod_p(i)));
						c_i2 = mod_q(l - mod_q(j) - mod_q(floor_p(i)) - c_i1);
						c_i = c_i1 + c_i2;
						c_j = 0;
						break;
					case POLYMEM_ACCESS_TYPE_Re:
						c_i = floor_p(mod_p(i) + mod_p(k - mod_p(i) - mod_p(floor_q(j))));
						c_j = floor_q(mod_q(j) + mod_q(l - mod_q(j) - mod_q(floor_p(i))));
						break;
				}
				break;

				case POLYMEM_SCHEME_ReTr:
					switch(POLYMEM_ACCESS_TYPE){
						case POLYMEM_ACCESS_TYPE_Re:
							c_i = mod_p(i) > k ? 1 : 0;
							delta = l - mod_q(c_i * p) - mod_q(floor_p(i) * p) - mod_q(j);
							c_j = floor_q(mod_q(j) + mod_q(delta));
							break;
						case POLYMEM_ACCESS_TYPE_Tr:
							c_i1 = mod_p(i) > k ? 1 : 0;
							c_i2 = floor_p(mod_q(l - c_i1 * p - mod_q(floor_p(i) * p) - mod_q(j) - mod_p(mod_p(l) - mod_p(j))));
							c_i = c_i1 + c_i2;
							c_j = floor_q(mod_q(j) + mod_p(mod_p(l) - mod_p(j)));
							break;
					}
					break;
        }

        return (floor_p(i) + c_i) * (M / q) + floor_q(j) + c_j;
    }

    int A_custom_t(int i, int j, int t, int POLYMEM_ACCESS_TYPE) { // to retrieve the position within the specific linear register
#pragma HLS inline region
        // t = m_v * q + m_h
        int k = floor_q(t); // k = m_v
        int l = mod_q(t);   // l = m_h

        return A_custom(i, j, k, l, POLYMEM_ACCESS_TYPE);
    }


public:

    /*
     * CONSTRUCTOR
     */
    polymem() {
#pragma HLS ARRAY_PARTITION variable=linearRegisters dim=1 complete
        check_scheme();
    }

    /*
     * DESTRUCTOR
     */
    virtual ~polymem() {
    }

    /*
     * METHOD TO PRINT LINEAR REGISTERS CONTENT
     */
    void print_registers() {
#ifndef __SYNTHESIS__
        std::cout << "POLYMEM" << "{" << std::endl;
        for(int i=0; i<N_LINEAR_REGISTERS; i++){
            std::cout << "LinearRegister_" << i << ": [";
            for(int j=0; j<LINEAR_REGISTERS_SIZE; j++){
                if(j != 0)
                    std::cout << "\t";
                std::cout << linearRegisters[i][j];
            }
            std::cout << "]" << std::endl;
        }
        std::cout << "}" << std::endl;
#endif
    }

    /*
     * METHOD TO PRINT POLYMEM DATA
     */
    void print() {
#ifndef __SYNTHESIS__
        std::cout << "POLYMEM_DATA" << "[" << std::endl;
        for(int i=0; i<N; i++){
            for(int j=0; j<M; j++){
                if(j != 0)
                    std::cout << "\t";
                std::cout << read(i,j);
            }
            std::cout << std::endl;
        }
        std::cout << "]" << std::endl;
#endif
    }

    /*
     * METHOD TO ZERO ALL THE POLYMEM VALUES
     */
    void initialize() {
        for (int j = 0; j < LINEAR_REGISTERS_SIZE; j++){
#pragma HLS PIPELINE II=1
            for (int i = 0; i < N_LINEAR_REGISTERS; i++){
                linearRegisters[i][j] = (POLYMEM_DATA_T) 0;
            }
        }
    }

    /*
     * METHOD TO WRITE ONE DATA INTO POLYMEM BASED ON PRE-SELECTED SCHEME: NOT OPTIMIZED FOR PARALLEL WRITES
     */
    void write(POLYMEM_DATA_T data, int i, int j) {
#pragma HLS inline region
        linearRegisters[m(i, j)][A_direct(i, j)] = data;
    }

    /*
     * METHOD TO READ ONE DATA FROM POLYMEM BASED ON PRE-SELECTED SCHEME: NOT OPTIMIZED FOR PARALLEL READS
     */
    POLYMEM_DATA_T read(int i, int j) {
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
     * METHOD TO WRITE DATA IN PARALLEL TO POLYMEM BY USING DIFFERENT ACCESS TYPE: Re, Ro, Co, Tr
     * The m function is executed in separate steps to reduce the crossbar dimension
     */
    void write_block(POLYMEM_DATA_T in[N_LINEAR_REGISTERS], int i, int j, int POLYMEM_ACCESS_TYPE) {
#pragma HLS inline region

        check_access_type(POLYMEM_ACCESS_TYPE);

        POLYMEM_DATA_T tmp[p][q]; // Used to store unordered data
#pragma HLS array_partition variable=tmp complete dim=0
        POLYMEM_DATA_T tmp2[p][q]; // Used to store unordered data
#pragma HLS array_partition variable=tmp2 complete dim=0

        for (int tp = 0; tp < p; tp++)
            for (int tq = 0; tq < q; tq++)
#pragma HLS UNROLL
                switch (POLYMEM_ACCESS_TYPE) {
					case POLYMEM_ACCESS_TYPE_Ro:
						tmp2[m_v(i, j + tp * q + tq)][tq] = in[tp * q + tq];
						break;
					case POLYMEM_ACCESS_TYPE_Co:
						tmp2[tp][m_h(i + tq * p + tp, j)] = in[tq * p + tp];
						break;
                }

        for (int tp = 0; tp < p; tp++)
            for (int tq = 0; tq < q; tq++)
#pragma HLS UNROLL
                switch (POLYMEM_ACCESS_TYPE) {
					case POLYMEM_ACCESS_TYPE_Ro:
						tmp[tp][m_h(i, j + tq)] = tmp2[tp][tq];
						break;
					case POLYMEM_ACCESS_TYPE_Co:
						tmp[m_v(i + tp, j)][tq] = tmp2[tp][tq];
						break;
                }

        for (int tp = 0; tp < p; tp++)
            for (int tq = 0; tq < q; tq++)
#pragma HLS UNROLL
                linearRegisters[tp * q + tq][A_custom(i, j, tp, tq, POLYMEM_ACCESS_TYPE)] = tmp[tp][tq];

    }

    /*
     * METHOD TO WRITE DATA IN PARALLEL TO POLYMEM BY USING DIFFERENT ACCESS TYPE: Re, Ro, Co, Tr
     */
    void write_block_masked(POLYMEM_DATA_T in[N_LINEAR_REGISTERS], ap_uint<N_LINEAR_REGISTERS> mask, int i, int j, int POLYMEM_ACCESS_TYPE) {
#pragma HLS inline region

        check_access_type(POLYMEM_ACCESS_TYPE);

        ap_uint<N_LINEAR_REGISTERS> mask_tmp;  // Used to store unordered bit
        POLYMEM_DATA_T tmp[N_LINEAR_REGISTERS];    // Used to store unordered data
#pragma HLS array_partition variable=tmp complete

        for (int t = 0; t < N_LINEAR_REGISTERS; t++)
#pragma HLS UNROLL
            switch (POLYMEM_ACCESS_TYPE) {
            case POLYMEM_ACCESS_TYPE_Ro:
                tmp[m(i, j + t)] = in[t];
                mask_tmp[m(i, j + t)] = mask[N_LINEAR_REGISTERS - 1 - t];
                break;
            case POLYMEM_ACCESS_TYPE_Co:
                tmp[m(i + t, j)] = in[t];
                mask_tmp[m(i + t, j)] = mask[N_LINEAR_REGISTERS - 1 - t];
                break;
            }

        for (int t = 0; t < N_LINEAR_REGISTERS; t++)
#pragma HLS UNROLL
            linearRegisters[t][A_custom_t(i, j, t, POLYMEM_ACCESS_TYPE)] = mask_tmp[t] ? tmp[t] : linearRegisters[t][A_custom_t(i, j, t, POLYMEM_ACCESS_TYPE)];
    }


    /*
     * METHOD TO READ DATA IN PARALLEL FROM POLYMEM BY USING DIFFERENT ACCESS TYPE: Re, Ro, Co, Tr
     * The m function is executed in separate steps to reduce the crossbar dimension
     */
    void read_block(int i, int j, POLYMEM_DATA_T out[N_LINEAR_REGISTERS], int POLYMEM_ACCESS_TYPE) {
#pragma HLS inline region

        check_access_type(POLYMEM_ACCESS_TYPE);

        POLYMEM_DATA_T tmp[p][q];  // Used to store unordered data
#pragma HLS array_partition variable=tmp complete dim=0


        // RETRIEVE DATA A_CUSTOM
        for (int k = 0; k < p; k++)
            for (int l = 0; l < q; l++)
#pragma HLS UNROLL
                tmp[k][l] = linearRegisters[k * q + l][A_custom(i, j, k, l, POLYMEM_ACCESS_TYPE)];



        ///*
        // INDEX
        int m_v_row[N_LINEAR_REGISTERS];  // Used to store row index
#pragma HLS array_partition variable=m_v_row complete
        int m_h_col[N_LINEAR_REGISTERS];  // Used to store colum index
#pragma HLS array_partition variable=m_h_col complete


        for (int k = 0; k < p; k++)
            for (int l = 0; l < q; l++)
#pragma HLS UNROLL
                switch (POLYMEM_ACCESS_TYPE) {
					case POLYMEM_ACCESS_TYPE_Ro:
						m_v_row[k * q + l] = m_v(i, j + k * q + l);
						m_h_col[k * q + l] = m_h(i, j + l);
						break;
					case POLYMEM_ACCESS_TYPE_Co:
						m_v_row[l * p + k] = m_v(i + k, j);
						m_h_col[l * p + k] = m_h(i + l * p + k, j);
						break;
					case POLYMEM_ACCESS_TYPE_Re:
						m_v_row[k * q + l] = m_v(i + k, j + l);
						m_h_col[k * q + l] = m_h(i + k, j + l);
						break;
					case POLYMEM_ACCESS_TYPE_Tr:
						m_v_row[l * p + k] = m_v(i + l, j + k);
						m_h_col[l * p + k] = m_h(i + l, j + k);
						break;
					case POLYMEM_ACCESS_TYPE_MD:
						m_v_row[k * q + l] = m_v(i + l, j + k * q + l);
						m_h_col[k * q + l] = m_h(i + l + k * q, j + l);
						break;
					case POLYMEM_ACCESS_TYPE_SD:
						m_v_row[k * q + l] = m_v(i + l, j - k * q - l);
						m_h_col[k * q + l] = m_h(i + l + k * q, j - l);
						break;
                }



        // SHUFFLE
        for (int t = 0; t < N_LINEAR_REGISTERS; t++)
#pragma HLS UNROLL
			out[t] = tmp[m_v_row[t]][m_h_col[t]];
		
    }


}; // end class polymem

}  // end namespace hls

#endif  // end HLS_POLYMEM_H
