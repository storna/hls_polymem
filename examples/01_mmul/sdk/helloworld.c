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
#include "golden.h"
//#include "xmatr_mul_stream.h" // Not necessary if "HLS INTERFACE ap_ctrl_none"


// ------------------------------------------------
// |      COSTANT DEPENDING ON ARCHITECTURE       |
// ------------------------------------------------
#define DIM 96
#define STREAM_SIZE (DIM * DIM)


// ------------------------------------------------
// |     SOFTWARE EXECUTION TO COMPARE RESULTS    |
// ------------------------------------------------
void matrix_mul_sw(float A[DIM][DIM], float B[DIM][DIM], float C[DIM][DIM]) {
	int i, j, k;
	for (i = 0; i < DIM; i++) {
		for (j = 0; j < DIM; j++) {
			for (k = 0; k < DIM; k++) {
				C[i][j] += A[i][k] * B[k][j];
			}
		}
	}
}


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

    float A[DIM][DIM];
    float B[DIM][DIM];

    /** Matrix Initiation */
    for (i = 0; i < DIM; i++) {
        for (j = 0; j < DIM; j++) {
            A[i][j] = (float) (i + j);
    		B[i][j] = (float) (i * j);
        }
    }

    /** End of Initiation */

	float OUT_SW_AB[DIM][DIM];
	float OUT_SW_BA[DIM][DIM];
	float OUT_HW_AB[DIM][DIM];
	float OUT_HW_BA[DIM][DIM];


	// ========== PARAMETERS NEEDED BY THE CORE ==========
	// Buffers to send (receive) data to (from) streams
	float *a = (float*) malloc(STREAM_SIZE * sizeof(float));
	float *b = (float*) malloc(STREAM_SIZE * sizeof(float));
	float *buff_out = (float*) malloc(STREAM_SIZE * 2 * sizeof(float));


	// ========== INIT OUT MATRIXES ==========
	for (i = 0; i < DIM; i++) {
		for (j = 0; j < DIM; j++) {
			OUT_SW_AB[i][j] = 0;
			OUT_SW_BA[i][j] = 0;
			OUT_HW_AB[i][j] = 0;
			OUT_HW_BA[i][j] = 0;
		}
	}


	// ========== FILL BUFFERS TO STREAM DATA WITH THE CORE ==========
	for (i = 0; i < DIM; i++) {
		for (j = 0; j < DIM; j++) {
			a[i * DIM + j] = A[i][j];
			b[i * DIM + j] = B[i][j];
		}
	}

	for (i = 0; i < STREAM_SIZE * 2; i++) {
		buff_out[i * DIM + j] = 0;
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
	double run_time_sw = 0;
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


	// ========== SOFTWARE EXECUTION ==========
	printf("\n====================\nSW MATRIX MUL:\n====================\n\r");

	// Start HW timer
	XTmrCtr_Start(&timer, 0);
	begin_time = XTmrCtr_GetValue(&timer, 0);

	// Execution on CPU
	//matrix_mul_sw(A, B, OUT_SW_AB);
	//matrix_mul_sw(B, A, OUT_SW_BA);

	// Stop timer
	end_time = XTmrCtr_GetValue(&timer, 0);
	XTmrCtr_Stop(&timer, 0);

	// Compute time
	run_time_sw = (end_time - begin_time - calibration) / 100000000.0;
	printf("CPU Execution Time: %f \n\r", run_time_sw);


	// ========== HARDWARE EXECUTION ==========
	printf("\n====================\nHW MATRIX MUL:\n====================\n");
	// Start timer
	XTmrCtr_Start(&timer, 0);
	begin_time = XTmrCtr_GetValue(&timer, 0);

	// ========== START CORE ==========
	//XMatr_mul_stream_Start(&core); // Not necessary if "HLS INTERFACE ap_ctrl_none"

	// ========== OPEN STREAM TO READ RESULTS A*B && B*A ==========
	// Open stream to receive data from the core and start read as soon as the data are available
	XAxiDma_SimpleTransfer(&dma_0, (u32) buff_out,
			STREAM_SIZE * 2 * sizeof(float), XAXIDMA_DEVICE_TO_DMA);

	// ========== SEND SIGNALS A & B ==========
	// Send signals to the core.
	XAxiDma_SimpleTransfer(&dma_0, (u32) &a[0],
			STREAM_SIZE * sizeof(float),
			XAXIDMA_DMA_TO_DEVICE);
	XAxiDma_SimpleTransfer(&dma_0, (u32) &b[0],
			STREAM_SIZE * sizeof(float),
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

	// Write results
	for (i = 0; i < DIM; i++) {
		for (j = 0; j < DIM; j++) {
			OUT_HW_AB[i][j] = buff_out[i * DIM + j];
			OUT_HW_BA[i][j] = buff_out[STREAM_SIZE + i * DIM + j];
		}
	}

	// COMPARE THE RESULTS
	for (i = 0; i < DIM && !err; i++)
		for (j = 0; j < DIM && !err; j++)
			if (OUT_HW_AB[i][j] != golden_AB[i][j]) {
				err = 1;
				printf("ERROR: AB!\n\r");
				printf("(%d, %d) %f != %f\n\r", i, j, OUT_HW_AB[i][j], golden_AB[i][j]);
			}
	for (i = 0; i < DIM && !err; i++)
		for (j = 0; j < DIM && !err; j++)
			if (OUT_HW_BA[i][j] != golden_BA[i][j]) {
				err = 1;
				printf("ERROR: BA!\n\r");
				printf("(%d, %d) %f != %f\n\r", i, j, OUT_HW_BA[i][j], golden_BA[i][j]);
			}

	if (err == 0)
		print("Results match!\n\n");
	else
		print("ERROR: results mismatch\n\n");

	// HW vs. SW speedup factor
	double acc_factor = run_time_sw / run_time_hw;
	xil_printf("Speedup: %d.%d \n\n", (int) acc_factor,
			(int) (acc_factor * 1000) % 1000);

	cleanup_platform();
	return XST_SUCCESS;
}

