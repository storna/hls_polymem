## HLS_PRF: Matrix Multiplication Streams ##

### Vivado HLS (version 2017.2) ###
The **hls** folder contains the following C++ files:

* **mmul.cpp**: implementation of the matrix multiplication that uses the default Vivado HLS array partitioning to perform parallel accesses to data (inspired from [A Zynq Accelerator for Floating Point Matrix Multiplication Designed with Vivado HLS](https://www.xilinx.com/support/documentation/application_notes/xapp1170-zynq-hls.pdf))
* **hls_prf.h**: implementation of the HLS_PRF for Row/Column parallel acesses
* **mmul_prf.cpp**: implementation of the matrix multiplication that uses the PRF to perform parallel accesses to data
* **test_hls.cpp**: test to perform the simulation of both the matrix multiplication implementation

How to Vivado HLS:

* Create new project and select as part: **Virtex 7 VC707 Evaluation Platform (xc7vx485tffg1761-2)**
* Import source files: **mmul.cpp**, **hls_prf.h**, **mmul_prf.cpp**
* Import test file: **test_hls.cpp**
* Start **Simulation**
* Select Top Function: Project -> Project Settings -> Synthesis -> Top Function -> **mmul_prf**
* Start **Synthesis**
* Start **Co-simulation**
* Solution -> **Export RTL**

### Vivado (version 2017.2) ###
How to Vivado:

* Create new project and select as part: **Virtex 7 VC707 Evaluation Platform (xc7vx485tffg1761-2)**
* Create Block Design
* **Import HLS IP Core**: IP settting -> IP -> Repository Management -> Add Cores by selecting Vivado HLS project directory (if the directory does not contain any IP Interface, go back to Vivado HLS and Export RTL)
* **Add** the following **components** to Block Design:
    * **MIG 7** (after Auto -> double click -> Next... -> Controller Option -> 400MHz (2500ns) -> Next... -> Memory Option -> remove Generate Additional Clock)
    * **Microblaze** (while Auto -> check clock 100MHz) (after Auto -> select **mdm_1** component ->  double click -> ENABLE JTAG UART)
    * **Axi DMA** (before Auto -> double click -> Disable Scatter Gather Engine / Buffer Length 23 / Select data width (eg. 32) / Mux Burst Size 256 /  Allow Unaligned Transfer)
    * **Custom IP Core** (mmul_prf)
    * **AXI Timer** (before Auto -> double click -> Enable 64 bit)
    
* **Validate Design**
* **Create HDL Wrapper**: Sources -> Design (dx click) -> create HDL Wrapper [needed to create drivers interfaces]
* **Optimize timing constraints**: Tools -> Settings -> Implementation -> Strategy=ExtraTimeOpt; Place Design directive=ExtraTimeOpt; Post-Place Phys Opt Design directive=AddRetime; Post-Route Phys Opt Design directive=AddRetime
* **Generate Bitstream**
* **Export HDF**: File -> Export -> Hardware -> Include Bitstream

### Vivado SDK (version 2016.4) ###
How to Vivado SDK:

* Load the **hdf file** from the example directory: File -> New Application Proj -> Target Hardware -> Hardware Platform -> New... -> Browse..
* **Create new project**: File -> New Application Proj -> "add project name" -> Next -> Hello World
* Copy the code files from the example directory
* Edit **lscript.ld** into project directory to extend heap and stack: src -> lscript.ld -> Heap and Stack (eg. 0x1f000000)
* **Program FPGA** -> Program
* Edit **Run Configurations..**: Xilinx Application (GDB) -> Target Setup (Bitstream file: [select] / Initialization file: [select] / Reset Processor: select / Program FPGA: activate / Run ps7_init: activate / Run ps7_post_config: activate); Application (select application and proj name); STDIO Connection (Connect STDIO to console -> JTAG UART)
* **Run**