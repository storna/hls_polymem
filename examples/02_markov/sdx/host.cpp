#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <assert.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <CL/opencl.h>


////////////////////////////////////////////////////////////////////////////////

// Use a static matrix for simplicity
#define PRINT_RESULTS 0
#define DIM 256
#define DATA_SIZE (DIM * DIM)


////////////////////////////////////////////////////////////////////////////////


typedef float DATA_TYPE;

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

int load_file_to_memory(const char *filename, char **result)
{
    size_t size = 0;
    FILE *f = fopen(filename, "rb");
    if (f == NULL)
    {
        *result = NULL;
        return -1; // -1 means file opening fail
    }
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    *result = (char *) malloc(size + 1);
    if (size != fread(*result, sizeof(char), size, f))
    {
        free(*result);
        return -2; // -2 means file reading fail
    }
    fclose(f);
    (*result)[size] = 0;
    return size;
}


////////////////////////////////////////////////////////////////////////////////


int main(int argc, char** argv)
{
    //TARGET_DEVICE macro needs to be passed from gcc command line
#if defined(SDA_PLATFORM) && !defined(TARGET_DEVICE)
  #define STR_VALUE(arg)      #arg
  #define GET_STRING(name) STR_VALUE(name)
  #define TARGET_DEVICE GET_STRING(SDA_PLATFORM)
#endif

    char *TARGET_DEVICES[] = {"xilinx_adm-pcie-ku3_2ddr-xpr_4_0", "xilinx:adm-pcie-ku3:2ddr-xpr:4.0", "xilinx_adm-pcie-7v3_1ddr_3_0"};
    char *XCLBIN_FILES[] = {"../kernel_ku3.xclbin", "../kernel_ku3.xclbin", "../kernel_7v3.xclbin"};
    int NUM_SUPPORTED_DEVICES = sizeof(TARGET_DEVICES)/sizeof(char *);
    char *xclbin;

    int err;                            // error code returned from api calls

    DATA_TYPE a[DATA_SIZE];              // original data set given to device
    DATA_TYPE output[DATA_SIZE];             // results returned from device

    unsigned int correct;               // number of correct results returned

    size_t global[2];                   // global domain size for our calculation
    size_t local[2];                    // local domain size for our calculation

    cl_platform_id platforms[16];       // platform id
    cl_platform_id platform_id;         // platform id
    cl_uint platform_count;
    cl_device_id device_id;             // compute device id
    cl_context context;                 // compute context
    cl_command_queue commands;          // compute command queue
    cl_program program;                 // compute program
    cl_kernel kernel;                   // compute kernel

    char cl_platform_vendor[1001];

    cl_mem cl_a;                     // device memory used for the input array
    cl_mem cl_output;                      // device memory used for the output array

    // Fill our data sets with pattern
    int i;
	for(i = 0; i < DATA_SIZE; i++) {
		a[i] = (DATA_TYPE) i;
		output[i] = 0;
	}

    // Get all platforms and then select Xilinx platform
    err = clGetPlatformIDs(16, platforms, &platform_count);
    if (err != CL_SUCCESS)
        {
            printf("Error: Failed to find an OpenCL platform!\n");
            printf("Test failed with err: %d\n", err);
            return EXIT_FAILURE;
        }
    printf("INFO: Found %d platforms\n", platform_count);

    // Find Xilinx Plaftorm
    int platform_found = 0;
    for (unsigned int iplat=0; iplat<platform_count; iplat++) {
        err = clGetPlatformInfo(platforms[iplat], CL_PLATFORM_VENDOR, 1000, (void *)cl_platform_vendor,NULL);
        if (err != CL_SUCCESS) {
            printf("Error: clGetPlatformInfo(CL_PLATFORM_VENDOR) failed!\n");
            printf("Test failed\n");
            return EXIT_FAILURE;
        }
        if (strcmp(cl_platform_vendor, "Xilinx") == 0) {
            printf("INFO: Selected platform %d from %s\n", iplat, cl_platform_vendor);
            platform_id = platforms[iplat];
            platform_found = 1;
        }
    }
    if (!platform_found) {
        printf("ERROR: Platform Xilinx not found. Exit.\n");
        return EXIT_FAILURE;
    }

    // Connect to a compute device
    // find all devices and then select the target device
    cl_device_id devices[16];  // compute device id
    cl_uint device_count;
    unsigned int device_found = 0;
    char cl_device_name[1001];
    err = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_ACCELERATOR,
                         16, devices, &device_count);
    if (err != CL_SUCCESS) {
        printf("Error: Failed to create a device group!\n");
        printf("Test failed\n");
        return EXIT_FAILURE;
    }

    // iterate all devices to select the target device.
    for (int i=0; (i<device_count) && (!device_found); i++) {
        err = clGetDeviceInfo(devices[i], CL_DEVICE_NAME, 1024, cl_device_name, 0);
        if (err != CL_SUCCESS) {
            printf("Error: Failed to get device name for device %d!\n", i);
            printf("Test failed\n");
            return EXIT_FAILURE;
        }
        printf("INFO: Analyzing device: %s\n", cl_device_name);
        for(int t=0; t < NUM_SUPPORTED_DEVICES; t++){
            if(strcmp(cl_device_name, TARGET_DEVICES[t]) == 0) {
                device_id = devices[i];
                device_found = 1;
                xclbin = XCLBIN_FILES[t];
                printf("INFO: Selected %s as the target device\n", cl_device_name);
            }
        }
    }

    if (!device_found) {
        printf("ERROR: Target device not found. Exit.\n");
        return EXIT_FAILURE;
    }


    err = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_ACCELERATOR,
                         1, &device_id, NULL);
    if (err != CL_SUCCESS)
        {
            printf("Error: Failed to create a device group!\n");
            printf("Test failed\n");
            return EXIT_FAILURE;
        }

    // Create a compute context
    context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);
    if (!context)
        {
            printf("Error: Failed to create a compute context!\n");
            printf("Test failed\n");
            return EXIT_FAILURE;
        }

    // Create a command commands
    commands = clCreateCommandQueue(context, device_id, 0, &err);
    if (!commands)
        {
            printf("Error: Failed to create a command commands!\n");
            printf("Error: code %i\n",err);
            printf("Test failed\n");
            return EXIT_FAILURE;
        }

    int status;

    // Create Program Objects

    // Load binary from disk
    unsigned char *kernelbinary;
    printf("INFO: Loading %s\n", xclbin);
    int n_i = load_file_to_memory(xclbin, (char **) &kernelbinary);
    if (n_i < 0) {
        printf("failed to load kernel from xclbin: %s\n", xclbin);
        printf("Test failed\n");
        return EXIT_FAILURE;
    }
    size_t n = n_i;
    // Create the compute program from offline
    program = clCreateProgramWithBinary(context, 1, &device_id, &n,
                                        (const unsigned char **) &kernelbinary, &status, &err);
    if ((!program) || (err!=CL_SUCCESS)) {
        printf("Error: Failed to create compute program from binary %d!\n", err);
        printf("Test failed\n");
        return EXIT_FAILURE;
    }

    // Build the program executable
    //
    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if (err != CL_SUCCESS)
        {
            size_t len;
            char buffer[2048];

            printf("Error: Failed to build program executable!\n");
            clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
            printf("%s\n", buffer);
            printf("Test failed\n");
            return EXIT_FAILURE;
        }

    // Create the compute kernel in the program we wish to run
    //
    kernel = clCreateKernel(program, "mmul_prf", &err);
    if (!kernel || err != CL_SUCCESS)
        {
            printf("Error: Failed to create compute kernel!\n");
            printf("Test failed\n");
            return EXIT_FAILURE;
        }

    // Create the input and output arrays in device memory for our calculation
    //
    cl_a = clCreateBuffer(context,  CL_MEM_READ_ONLY,  sizeof(DATA_TYPE) * DATA_SIZE, NULL, NULL);
    cl_output = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(DATA_TYPE) * DATA_SIZE, NULL, NULL);
    if (!a || !output)
        {
            printf("Error: Failed to allocate device memory!\n");
            printf("Test failed\n");
            return EXIT_FAILURE;
        }

    // Write our data set into the input array in device memory
    //
    err = clEnqueueWriteBuffer(commands, cl_a, CL_TRUE, 0, sizeof(DATA_TYPE) * DATA_SIZE, a, 0, NULL, NULL);
    if (err != CL_SUCCESS)
        {
            printf("Error: Failed to write to source array a!\n");
            printf("Test failed\n");
            return EXIT_FAILURE;
        }

    // Set the arguments to our compute kernel
    //
    err = 0;
    err  = clSetKernelArg(kernel, 0, sizeof(cl_mem), &cl_a);
    err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &cl_output);
    if (err != CL_SUCCESS)
        {
            printf("Error: Failed to set kernel arguments! %d\n", err);
            printf("Test failed\n");
            return EXIT_FAILURE;
        }

    // Execute the kernel over the entire range of our 1d input data set
    // using the maximum number of work group items for this device
    //

#ifdef C_KERNEL
    err = clEnqueueTask(commands, kernel, 0, NULL, NULL);
#else
    global[0] = DIM;
    global[1] = DIM;
    local[0] = DIM;
    local[1] = DIM;
    err = clEnqueueNDRangeKernel(commands, kernel, 2, NULL,
                                 (size_t*)&global, (size_t*)&local, 0, NULL, NULL);
#endif
    if (err)
        {
            printf("Error: Failed to execute kernel! %d\n", err);
            printf("Test failed\n");
            return EXIT_FAILURE;
        }

    // Read back the results from the device to verify the output
    //
    cl_event readevent;
    err = clEnqueueReadBuffer(commands, cl_output, CL_TRUE, 0, sizeof(DATA_TYPE) * DATA_SIZE, output, 0, NULL, &readevent );
    if (err != CL_SUCCESS)
        {
            printf("Error: Failed to read output array! %d\n", err);
            printf("Test failed\n");
            return EXIT_FAILURE;
        }

    clWaitForEvents(1, &readevent);

    DATA_TYPE sw_mmul[DIM][DIM];
    DATA_TYPE a_temp[DIM][DIM];
	for (i = 0; i < DIM; i++) {
		for (int j = 0; j < DIM; j++) {
			a_temp[i][j] = a[i*DIM+j];
		}
	}
    A_k_sw(a_temp, sw_mmul);

    if(PRINT_RESULTS){
		printf("A\n");
		for (i=0;i<DATA_SIZE;i++) {
			printf("%f ",a[i]);
			if (((i+1) % 16) == 0)
				printf("\n");
		}
		printf("res\n");
		for (i=0;i<DATA_SIZE;i++) {
			printf("%f ",output[i]);
			if (((i+1) % 16) == 0)
				printf("\n");
		}
		printf("Software\n");
		for (i = 0; i < DIM; i++) {
			for (int j = 0; j < DIM; j++) {
				printf("%f ", sw_mmul[i][j]);
				if (j == (DIM - 1))
					printf("\n");
			}
		}
    }


    // Validate our results
    correct = 0;

    for (i = 0;i < DIM; i++)
        for (int j = 0;j < DIM; j++)
        	if(output[i*DIM+j] == sw_mmul[i][j])
        		correct++;

    // Print a brief summary detailing the results
    //
    printf("\nComputed '%d/%d' correct values!\n", correct, DATA_SIZE);

    // Shutdown and cleanup
    //
    clReleaseMemObject(cl_a);
    clReleaseMemObject(cl_output);
    clReleaseProgram(program);
    clReleaseKernel(kernel);
    clReleaseCommandQueue(commands);
    clReleaseContext(context);

    if(correct == DATA_SIZE){
        printf("=============== Test passed! ===============\n");
        return EXIT_SUCCESS;
    }
    else{
        printf("=============== Test failed ===============\n");
        return EXIT_FAILURE;
    }
}

