#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "xil_printf.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <xil_cache.h>
#include "xil_types.h"
#include "xil_io.h"
#include "xtmrctr.h"
#include "xparameters.h"
#include "xaxidma.h"
#include "platform.h"
#include "readings.h"
//#include "xmatr_mul_stream.h" // Not necessary if "HLS INTERFACE ap_ctrl_none"


// ------------------------------------------------
// |      COSTANT DEPENDING ON ARCHITECTURE       |
// ------------------------------------------------
// INPUT/OUTPUT DATA
typedef double DATA_T;

#define DIM 96
#define SIZE (DIM * DIM)

// READINGS
#define N_READINGS 3072 // to create a vector of readings in the shape [(i_0, j_0, ACCESS_TYPE_0), (i_1, j_1, ACCESS_TYPE_1), ...]
#define LEN_READINGS (N_READINGS * 2)

// POLYMEM
#define log2_p 1 // log2(p)
#define log2_q 3 // log2(q)
#define p (1 << log2_p) // number of rows of the matrix in which physical memory modules (linear registers) are organized in the POLYMEM
#define q (1 << log2_q) // number of columns of the matrix in which physical memory modules (linear registers) are organized in the POLYMEM
#define N_LINEAR_REGISTERS (p * q)
#define LINEAR_REGISTERS_SIZE (SIZE / N_LINEAR_REGISTERS)

// RESULTS
#define N_RESULTS_BLOCK 50
#define N_RESULTS (N_RESULTS_BLOCK * N_LINEAR_REGISTERS)


// ------------------------------------------------
// |                INITIALIZE DMAs               |
// ------------------------------------------------
int initDMA(XAxiDma *dma, u32 DeviceId) {
	XAxiDma_Config *CfgPtr;
	int status;

	CfgPtr = XAxiDma_LookupConfig(DeviceId);
	if (!CfgPtr) {
		print("Error looking for AXI DMA config\n\r");
		return XST_FAILURE;
	}
	status = XAxiDma_CfgInitialize(dma, CfgPtr);
	if (status != XST_SUCCESS) {
		print("Error initializing DMA\n\r");
		return XST_FAILURE;
	}
	// Check for scatter gather mode
	if (XAxiDma_HasSg(dma)) {
		print("Error DMA configured in SG mode\n\r");
		return XST_FAILURE;
	}
	// Disable interrupts, we use polling mode
	XAxiDma_IntrDisable(dma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA);
	XAxiDma_IntrDisable(dma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DMA_TO_DEVICE);

	// Reset DMA
	XAxiDma_Reset(dma);
	while (!XAxiDma_ResetIsDone(dma)) {
	}

	printf("Initialization of DMA %d done!\n\r", (int) DeviceId);
	return XST_SUCCESS;
}


// ------------------------------------------------
// |          FUNCTION THAT TEST CORE             |
// ------------------------------------------------
int main() {
	init_platform();
	print("Platform initialized\n\r");

	// ========== LOCAL VARIABLES ==========
	int status, err=0, i, j;

    DATA_T data[DIM][DIM];

    /** Matrix Initiation */
    for (i = 0; i < DIM; i++) {
        for (j = 0; j < DIM; j++) {
        	data[i][j] = (DATA_T) (i * DIM + j);
        }
    }
    /** End of Initiation */

	// ========== PARAMETERS NEEDED BY THE CORE ==========
	// Buffers to send (receive) data to (from) streams
	DATA_T *a = (DATA_T*) malloc((SIZE + LEN_READINGS) * sizeof(DATA_T));
	DATA_T *buff_out = (DATA_T*) malloc(N_RESULTS * sizeof(DATA_T));



	// ========== FILL BUFFERS TO STREAM DATA WITH THE CORE ==========
	for (i = 0; i < DIM; i++) {
		for (j = 0; j < DIM; j++) {
			a[i * DIM + j] = data[i][j];
		}
	}

	for (i = 0; i < LEN_READINGS; i++) {
		a[SIZE + i] = (DATA_T) readings_RoCo[i];
	}

	for (i = 0; i < N_RESULTS; i++) {
		buff_out[i] = 0;
	}


	// ========== INITIALIZE ENVIRONMENT ==========
	// Declaration of the core
	//XMatr_mul_stream core; // Not necessary if "HLS INTERFACE ap_ctrl_none"

	// Initialization of the core
	//XMatr_mul_stream_Initialize(&core, 0); // Not necessary if "HLS INTERFACE ap_ctrl_none"
	//XMatr_mul_stream_DisableAutoRestart(&core); // Not necessary if "HLS INTERFACE ap_ctrl_none"

	// Disable the cache
	Xil_DCacheDisable();
	Xil_ICacheDisable();

	// Declaration and Initialization of the HW timer
	unsigned int begin_time;
	unsigned int end_time;
	unsigned int calibration;
	double run_time_hw = 0;

	XTmrCtr timer;

	status = XTmrCtr_Initialize(&timer, XPAR_AXI_TIMER_0_DEVICE_ID);
	if (status != XST_SUCCESS) {
		print("Error: timer setup failed\n");
		return XST_FAILURE;
	}
	XTmrCtr_SetOptions(&timer, 0,
			XTC_AUTO_RELOAD_OPTION | XTC_CASCADE_MODE_OPTION);

	XTmrCtr_Reset(&timer, 0);
	XTmrCtr_Reset(&timer, 1);

	// Calibrate HW timer
	XTmrCtr_Start(&timer, 0);
	begin_time = XTmrCtr_GetValue(&timer, 0);
	end_time = XTmrCtr_GetValue(&timer, 0);
	XTmrCtr_Stop(&timer, 0);

	calibration = end_time - begin_time;

	// Declaration and Initialization of the DMA
	XAxiDma dma_0;

	status = initDMA(&dma_0, 0);
	if (status != XST_SUCCESS) {
		print("\rError: DMA init failed\n");
		return XST_FAILURE;
	}

	// ========== HARDWARE EXECUTION ==========
	printf("\n====================\nHW EXECUTION:\n====================\n");
	// Start timer
	XTmrCtr_Start(&timer, 0);
	begin_time = XTmrCtr_GetValue(&timer, 0);

	// ========== START CORE ==========
	//XMatr_mul_stream_Start(&core); // Not necessary if "HLS INTERFACE ap_ctrl_none"

	// ========== OPEN STREAM TO READ RESULTS ==========
	// Open stream to receive data from the core and start read as soon as the data are available
	XAxiDma_SimpleTransfer(&dma_0, (u32) buff_out,
			N_RESULTS * sizeof(DATA_T), XAXIDMA_DEVICE_TO_DMA);

	// ========== SEND SIGNALS A & B ==========
	// Send signals to the core.
	XAxiDma_SimpleTransfer(&dma_0, (u32) &a[0],
			(SIZE + LEN_READINGS) * sizeof(DATA_T),
			XAXIDMA_DMA_TO_DEVICE);

	// Ensure all data are received.
	while (XAxiDma_Busy(&dma_0, XAXIDMA_DMA_TO_DEVICE))
		;

	// ========== WAIT RESULTS ==========
	// Wait until all data of output are read from the core.
	while (XAxiDma_Busy(&dma_0, XAXIDMA_DEVICE_TO_DMA))
		;

	// Stop timer
	end_time = XTmrCtr_GetValue(&timer, 0);

	XTmrCtr_Stop(&timer, 0);

	run_time_hw = (end_time - begin_time - calibration) / 100000000.0;
	printf("FPGA Execution Time: %f \n", run_time_hw);

	// Print results
	for (i = 0; i < 10; i++) {
		printf("%f\n\r", buff_out[i]);
	}


	cleanup_platform();
	return XST_SUCCESS;
}

