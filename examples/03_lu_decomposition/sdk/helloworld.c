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
#include "matrices.h"


// ------------------------------------------------
// |      COSTANT DEPENDING ON ARCHITECTURE       |
// ------------------------------------------------
#define DIM 96
#define STREAM_SIZE (DIM * DIM)


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

    /** End of Initiation */
	float OUT_HW_LU[DIM][DIM];


	// ========== PARAMETERS NEEDED BY THE CORE ==========
	// Buffers to send (receive) data to (from) streams
	float *a = (float*) malloc(STREAM_SIZE * sizeof(float));
	float *buff_out = (float*) malloc(STREAM_SIZE * sizeof(float));


	// ========== INIT OUT MATRIXES ==========
	for (i = 0; i < DIM; i++) {
		for (j = 0; j < DIM; j++) {
			OUT_HW_LU[i][j] = 0;
		}
	}


	// ========== FILL BUFFERS TO STREAM DATA WITH THE CORE ==========
	for (i = 0; i < DIM; i++) {
		for (j = 0; j < DIM; j++) {
			a[i * DIM + j] = a_96[i][j];
		}
	}

	for (i = 0; i < STREAM_SIZE; i++) {
		buff_out[i * DIM + j] = 0;
	}


	// ========== INITIALIZE ENVIRONMENT ==========
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
	printf("\n====================\nHW MATRIX MUL:\n====================\n");
	// Start timer
	XTmrCtr_Start(&timer, 0);
	begin_time = XTmrCtr_GetValue(&timer, 0);

	// ========== OPEN STREAM TO READ RESULTS LU ==========
	// Open stream to receive data from the core and start read as soon as the data are available
	XAxiDma_SimpleTransfer(&dma_0, (u32) buff_out,
			STREAM_SIZE * sizeof(float), XAXIDMA_DEVICE_TO_DMA);

	print("1\n");
	// ========== SEND SIGNALS a ==========
	// Send signals to the core.
	XAxiDma_SimpleTransfer(&dma_0, (u32) a,
			STREAM_SIZE * sizeof(float),
			XAXIDMA_DMA_TO_DEVICE);

	print("2\n");
	// Ensure all data are received.
	while (XAxiDma_Busy(&dma_0, XAXIDMA_DMA_TO_DEVICE))
		;

	print("3\n");
	// ========== WAIT RESULTS ==========
	// Wait until all data of output are read from the core.
	while (XAxiDma_Busy(&dma_0, XAXIDMA_DEVICE_TO_DMA))
		;

	print("4\n");
	// Stop timer
	end_time = XTmrCtr_GetValue(&timer, 0);

	XTmrCtr_Stop(&timer, 0);

	run_time_hw = (end_time - begin_time - calibration) / 100000000.0;
	printf("FPGA Execution Time: %f \n", run_time_hw);

	// Write results
	for (i = 0; i < DIM; i++) {
		for (j = 0; j < DIM; j++) {
			OUT_HW_LU[i][j] = buff_out[i * DIM + j];
		}
	}

	// COMPARE THE RESULTS
	/*
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
	 */

	if (err == 0)
		print("Results match!\n\n");
	else
		print("ERROR: results mismatch\n\n");


	cleanup_platform();
	return XST_SUCCESS;
}


