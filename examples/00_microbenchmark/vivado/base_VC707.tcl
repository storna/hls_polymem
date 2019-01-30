#
# STEP#1: define the variables.
#
set poly 0
set hls_folder hls_ReRo_96_2_8

if {$poly} {
	set name polymem_ReRo_96_2_8
	set core polymem_readings
	set core_0 polymem_readings_0
} else {
	set name hlsp_bl_ReRo_96_2_8
	set core hlsp_bl_readings
	set core_0 hlsp_bl_readings_0
}

create_project $name /home/users/luca.stornaiuolo/Desktop/polymem/vivado/$name -part xc7vx485tffg1761-2

set_property board_part xilinx.com:vc707:part0:1.3 [current_project]

create_bd_design "design_1"

update_compile_order -fileset sources_1

startgroup
create_bd_cell -type ip -vlnv xilinx.com:ip:mig_7series:4.0 mig_7series_0
endgroup
apply_bd_automation -rule xilinx.com:bd_rule:mig_7series -config {Board_Interface "ddr3_sdram" }  [get_bd_cells mig_7series_0]
apply_bd_automation -rule xilinx.com:bd_rule:board -config {Board_Interface "reset ( FPGA Reset ) " }  [get_bd_pins mig_7series_0/sys_rst]



startgroup
create_bd_cell -type ip -vlnv xilinx.com:ip:microblaze:10.0 microblaze_0
endgroup
apply_bd_automation -rule xilinx.com:bd_rule:microblaze -config {local_mem "8KB" ecc "None" cache "8KB" debug_module "Debug Only" axi_periph "Enabled" axi_intc "0" clk "/mig_7series_0/ui_addn_clk_0 (100 MHz)" }  [get_bd_cells microblaze_0]
startgroup
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {Master "/microblaze_0 (Cached)" intc_ip "Auto" Clk_xbar "Auto" Clk_master "Auto" Clk_slave "Auto" }  [get_bd_intf_pins mig_7series_0/S_AXI]
apply_bd_automation -rule xilinx.com:bd_rule:board -config {Board_Interface "reset ( FPGA Reset ) " }  [get_bd_pins rst_mig_7series_0_100M/ext_reset_in]
endgroup
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {Slave "/mig_7series_0/S_AXI" intc_ip "/axi_smc" Clk_xbar "Auto" Clk_master "Auto" Clk_slave "Auto" }  [get_bd_intf_pins microblaze_0/M_AXI_DP]




startgroup
set_property -dict [list CONFIG.C_USE_UART {1}] [get_bd_cells mdm_1]
endgroup
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {Master "/microblaze_0 (Periph)" intc_ip "/axi_smc" Clk_xbar "Auto" Clk_master "Auto" Clk_slave "Auto" }  [get_bd_intf_pins mdm_1/S_AXI]



startgroup
create_bd_cell -type ip -vlnv xilinx.com:ip:axi_dma:7.1 axi_dma_0
endgroup
startgroup
set_property -dict [list CONFIG.c_include_sg {0} CONFIG.c_sg_length_width {23} CONFIG.c_m_axi_mm2s_data_width {64} CONFIG.c_m_axis_mm2s_tdata_width {64} CONFIG.c_include_mm2s_dre {1} CONFIG.c_mm2s_burst_size {256} CONFIG.c_include_s2mm_dre {1} CONFIG.c_s2mm_burst_size {256} CONFIG.c_sg_include_stscntrl_strm {0}] [get_bd_cells axi_dma_0]
endgroup
startgroup
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {Master "/microblaze_0 (Periph)" intc_ip "/axi_smc" Clk_xbar "Auto" Clk_master "Auto" Clk_slave "Auto" }  [get_bd_intf_pins axi_dma_0/S_AXI_LITE]
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {Slave "/mig_7series_0/S_AXI" intc_ip "/axi_smc" Clk_xbar "Auto" Clk_master "Auto" Clk_slave "Auto" }  [get_bd_intf_pins axi_dma_0/M_AXI_MM2S]
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {Slave "/mig_7series_0/S_AXI" intc_ip "/axi_smc" Clk_xbar "Auto" Clk_master "Auto" Clk_slave "Auto" }  [get_bd_intf_pins axi_dma_0/M_AXI_S2MM]
endgroup



startgroup
create_bd_cell -type ip -vlnv xilinx.com:ip:axi_timer:2.0 axi_timer_0
endgroup
set_property -dict [list CONFIG.mode_64bit {1}] [get_bd_cells axi_timer_0]
apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {Master "/microblaze_0 (Periph)" intc_ip "/axi_smc" Clk_xbar "Auto" Clk_master "Auto" Clk_slave "Auto" }  [get_bd_intf_pins axi_timer_0/S_AXI]















set_property  ip_repo_paths  /home/users/luca.stornaiuolo/Desktop/polymem/hls/$hls_folder [current_project]
update_ip_catalog

startgroup
create_bd_cell -type ip -vlnv xilinx.com:hls:$core:1.0 $core_0
endgroup
connect_bd_intf_net [get_bd_intf_pins axi_dma_0/S_AXIS_S2MM] [get_bd_intf_pins $core_0/s_out]
connect_bd_intf_net [get_bd_intf_pins $core_0/s_in_V] [get_bd_intf_pins axi_dma_0/M_AXIS_MM2S]
apply_bd_automation -rule xilinx.com:bd_rule:clkrst -config {Clk "/mig_7series_0/ui_clk (200 MHz)" }  [get_bd_pins $core_0/ap_clk]

validate_bd_design
make_wrapper -files [get_files /home/users/luca.stornaiuolo/Desktop/polymem/vivado/$name/$name.srcs/sources_1/bd/design_1/design_1.bd] -top
add_files -norecurse /home/users/luca.stornaiuolo/Desktop/polymem/vivado/$name/$name.srcs/sources_1/bd/design_1/hdl/design_1_wrapper.v
update_compile_order -fileset sources_1

save_bd_design
launch_runs impl_1 -to_step write_bitstream -jobs 4

# open_run impl_1
# file copy -force /home/users/luca.stornaiuolo/Desktop/polymem/vivado/$name/$name.runs/impl_1/design_1_wrapper.sysdef /home/users/luca.stornaiuolo/Projects/repo/hls_prf/polymem/examples/polymem_readings/sdk/impl/$name.hdf



