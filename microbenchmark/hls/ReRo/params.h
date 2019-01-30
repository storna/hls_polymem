// INPUT/OUTPUT DATA
typedef double DATA_T;

#define DIM 96
#define SIZE (DIM * DIM)

struct data_struct {
	DATA_T data;
    bool last;
};

// READINGS
#define N_READINGS 3072 // to create a vector of readings in the shape [(i_0, j_0, ACCESS_TYPE_0), (i_1, j_1, ACCESS_TYPE_1), ...]
#define LEN_READINGS (N_READINGS * 2)

// POLYMEM
#define POLYMEM_SCHEME POLYMEM_SCHEME_ReRo // set the default scheme

#define log2_p 1 // log2(p)
#define log2_q 3 // log2(q)
#define p (1 << log2_p) // number of rows of the matrix in which physical memory modules (linear registers) are organized in the POLYMEM
#define q (1 << log2_q) // number of columns of the matrix in which physical memory modules (linear registers) are organized in the POLYMEM
#define N_LINEAR_REGISTERS (p * q)
#define LINEAR_REGISTERS_SIZE (SIZE / N_LINEAR_REGISTERS)

// RESULTS
#define N_RESULTS_BLOCK 50
#define N_RESULTS (N_RESULTS_BLOCK * N_LINEAR_REGISTERS)
