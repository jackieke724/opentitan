import sys
import os

numfiles = 5
runs = [False for i in range(numfiles)]
if (sys.argv[1] == "all"):
    runs[:] = [True for i in range(numfiles)]
    print("Change all files.")
elif (int(sys.argv[1]) <= numfiles and int(sys.argv[1]) >= 1):
    print("Change file ", sys.argv[1])
    runs[int(sys.argv[1])-1] = True
else:
    print("No file is changed.")

if (runs[0]):
    #file 1, dla.sv
    #this file can be changed directly in 'hw/ip/dla/rtl/dla.sv'
    '''
    read_path = r'hw/ip/dla/rtl/dla.sv'
    write_path = r'/home/junmin/genesys2/opentitan-dla-fmc/opentitan-dla-fmc.srcs/sources_1/imports/rtl/dla.sv'
    os.rename(write_path, write_path+".orig")
    with open(read_path, 'r') as reader, open(write_path, 'w') as writer:
        dla = reader.readlines()
        
        #do all the modifications first
        #change register module to use new tlul wires
        print ("Change register module pins to use new wires")
        dla[189] = "        .tl_i(tl_h2d),\n"
        dla[190] = "        .tl_o(tl_d2h),\n"

        #new clock, reset, led ports
        dla[13] = "    input               SYSCLK_P,\n"
        del dla[14]
        s = (   
                "    input               SYSCLK_N,\n"
                "    input               IO_RST_N,\n"
                "    output [3:0]        led,\n"
            )
        dla.insert(14, s)

        #tl_i, tl_o ports -> axi chip2chip pins
        print ("Change tl_i, tl_o ports to use fmc:")
        dla[16] = "    //AXI Chip2Chip FMC ports - converted from TLUL\n"
        del dla[17:19]
        s = (   
                "    output wire axi_c2c_selio_tx_diff_clk_out_p,\n"
                "    output wire axi_c2c_selio_tx_diff_clk_out_n,\n"
                "    input wire axi_c2c_selio_rx_diff_clk_in_p,\n"
                "    input wire axi_c2c_selio_rx_diff_clk_in_n,\n"
                "    output wire [30 : 0] axi_c2c_selio_tx_data_out,\n"
                "    input wire [30 : 0] axi_c2c_selio_rx_data_in,\n"
            )
        dla.insert(17, s)
        

        #add tlul_d2h_fmc module
        print ("Add AXI C2C and AXI2TLUL modules.")
        s = (
            "    tlul_pkg::tl_h2d_t tl_h2d;\n"
            "    tlul_pkg::tl_d2h_t tl_d2h;\n"
            "\n"   
            "    logic sys_clk, clk_i, rst_ni;\n"
            "\n"
            "    dla_clkgen clk_gen_inst(\n"
            "        .SYSCLK_P,\n"
            "        .SYSCLK_N,\n"
            "        .IO_RST_N,\n"
            "        .sys_clk(sys_clk),\n"
            "        .clk_main(clk_i),\n"
            "        .rst_n(rst_ni)\n"
            "    );\n"
            "\n"
            "    wire axi_c2c_link_status_out;\n"
            "    wire axi_c2c_multi_bit_error_out;\n"
            "\n"
            "    chip_device chip_device_inst(\n"
            "        .clk_i,\n"
            "        .rst_ni,\n"
            "        \n"
            "        .tl_h2d,\n"
            "        .tl_d2h,\n"
            "        \n"
            "        .idelay_ref_clk(sys_clk),\n"
            "        .axi_c2c_selio_tx_diff_clk_out_p(axi_c2c_selio_tx_diff_clk_out_p),\n"
            "        .axi_c2c_selio_tx_diff_clk_out_n(axi_c2c_selio_tx_diff_clk_out_n),\n"
            "        .axi_c2c_selio_tx_data_out(axi_c2c_selio_tx_data_out),\n"
            "        .axi_c2c_selio_rx_diff_clk_in_p(axi_c2c_selio_rx_diff_clk_in_p),\n"
            "        .axi_c2c_selio_rx_diff_clk_in_n(axi_c2c_selio_rx_diff_clk_in_n),  \n"
            "        .axi_c2c_selio_rx_data_in(axi_c2c_selio_rx_data_in),\n"
            "        .axi_c2c_link_status_out(axi_c2c_link_status_out),\n"
            "        .axi_c2c_multi_bit_error_out(axi_c2c_multi_bit_error_out)\n"
            "    );\n"
            "\n"
            "    assign led = {  \n"
            "//                    1'b1, \n"
            "//                    1'b0, \n"
            "//                    1'b1,\n"
            "//                    1'b0,\n"
            "                    tl_d2h.d_valid,\n"
            "                    tl_d2h.a_ready,\n"
            "                    axi_c2c_multi_bit_error_out, \n"
            "                    axi_c2c_link_status_out};\n"
            "\n"
            )
        dla.insert(39, s)

        writer.writelines(dla)
    '''

if (runs[1]):
    #file 2, top_earlgrey_nexysvideo.sv
    read_path = r'hw/top_earlgrey/rtl/top_earlgrey_nexysvideo.sv'
    write_path = r'build/lowrisc_systems_top_earlgrey_nexysvideo_0.1/src/lowrisc_systems_top_earlgrey_nexysvideo_0.1/rtl/top_earlgrey_nexysvideo.sv'
    os.rename(write_path, write_path+".orig")
    with open(read_path, 'r') as reader, open(write_path, 'w') as writer:
        top_fpga = reader.readlines()
        
        #do all the modifications first


        #then add new lines, because
        #add new lines will make list index not match line number
        
        print("Add FMC ports for dla")
        s = (   "\n"
                "  output [7:0] led,\n"
                "  output wire axi_c2c_selio_tx_diff_clk_out_p,\n"
                "  output wire axi_c2c_selio_tx_diff_clk_out_n,\n"
                "  input wire axi_c2c_selio_rx_diff_clk_in_p,\n"
                "  input wire axi_c2c_selio_rx_diff_clk_in_n,\n"
                "  output wire [30 : 0] axi_c2c_selio_tx_data_out,\n"
                "  input wire [30 : 0] axi_c2c_selio_rx_data_in,\n"
                "//  output              FMC_DLA_RST_N,\n"
                "  input               FMC_DLA_INTR_DONE,\n"
                "  input               FMC_DLA_IDLE,\n"
                "\n"
            )
        top_fpga.insert(13, s)

        print("Add clock and reset wires.")
        s = (   "\n"
                "  logic dla_clk;\n"
                "  logic dla_rst_n;\n"
                "  logic clk_100mhz;\n"
                "  logic clk_200mhz;\n"
            )
        top_fpga.insert(61, s)

        print("Add interconnect wires")
        s = (   "  tlul_pkg::tl_h2d_t dla_tl_h2d;\n"
                "  tlul_pkg::tl_d2h_t dla_tl_d2h;\n"
                "\n"
            )
        top_fpga.insert(304, s)

        print ("Connect dla ports from tlul_fmc_host.")
        s = (   "    // dla fmc\n"
                "    .dla_clk         ( dla_clk       ),\n"
                "    .dla_rst_n       ( dla_rst_n     ),\n"
                "    .intr_dla_done   ( FMC_DLA_INTR_DONE ),\n"
                "    .dla_idle        ( FMC_DLA_IDLE      ),\n"
                "    .dla_tl_req      ( dla_tl_h2d    ),\n"
                "    .dla_tl_rsp      ( dla_tl_d2h    ),\n"
                "\n"
            )
        top_fpga.insert(376, s)


        s = (   
                "    .clk_100MHz(clk_100mhz),\n"
                "    .clk_200MHz(clk_200mhz),\n"
            )
        top_fpga.insert(297, s)

        s = (   
                "  logic axi_c2c_link_status_out;\n"
                "  logic axi_c2c_multi_bit_error_out;\n"
                "  logic axi_c2c_link_error_out;\n"
                "\n"
                "  chip_host chip_host_inst(\n"
                "    .clk_i(dla_clk),\n"
                "    .rst_ni(dla_rst_n),\n"
                "\n"
                "    .tl_h2d(dla_tl_h2d),\n"
                "    .tl_d2h(dla_tl_d2h),\n"
                "\n"
                "    .idelay_ref_clk(clk_200mhz),\n"
                "    .axi_c2c_phy_clk(clk_100mhz),\n"
                "    .axi_c2c_selio_tx_diff_clk_out_p(axi_c2c_selio_tx_diff_clk_out_p),\n"
                "    .axi_c2c_selio_tx_diff_clk_out_n(axi_c2c_selio_tx_diff_clk_out_n),\n"
                "    .axi_c2c_selio_tx_data_out(axi_c2c_selio_tx_data_out),\n"
                "    .axi_c2c_selio_rx_diff_clk_in_p(axi_c2c_selio_rx_diff_clk_in_p),\n"
                "    .axi_c2c_selio_rx_diff_clk_in_n(axi_c2c_selio_rx_diff_clk_in_n),\n"
                "    .axi_c2c_selio_rx_data_in(axi_c2c_selio_rx_data_in),\n"
                "    .axi_c2c_link_status_out(axi_c2c_link_status_out),          // output wire axi_c2c_link_status_out\n"
                "    .axi_c2c_multi_bit_error_out(axi_c2c_multi_bit_error_out),  // output wire axi_c2c_multi_bit_error_out\n"
                "    .axi_c2c_link_error_out(axi_c2c_link_error_out)            // output wire axi_c2c_link_error_out\n"
                "  );\n"
                "\n"
                "  assign led = {  1'b1, \n"
                "                  1'b0, \n"
                "                  1'b1,\n"
                "                  dla_tl_h2d.a_valid,\n"
                "                  dla_tl_h2d.d_ready,\n"
                "                  axi_c2c_link_error_out,\n"
                "                  axi_c2c_multi_bit_error_out, \n"
                "                  axi_c2c_link_status_out};\n"
                "\n"
            )
        top_fpga.insert(383, s)

        writer.writelines(top_fpga)

if (runs[2]):
    #file 3, top_earlgrey.sv
    read_path = r'hw/top_earlgrey/rtl/autogen/top_earlgrey.sv'
    write_path = r'build/lowrisc_systems_top_earlgrey_nexysvideo_0.1/src/lowrisc_systems_top_earlgrey_0.1/rtl/autogen/top_earlgrey.sv'
    os.rename(write_path, write_path+".orig")
    with open(read_path, 'r') as reader, open(write_path, 'w') as writer:
        top_earlgrey = reader.readlines()
        
        #do all the modifications first
        print("Comment local dla wires to use ports")
        top_earlgrey[237] = "  //logic intr_dla_done;\n"
        top_earlgrey[342] = "  //tlul_pkg::tl_h2d_t       dla_tl_req;\n"
        top_earlgrey[343] = "  //tlul_pkg::tl_d2h_t       dla_tl_rsp;\n"

        print("Remove u_dla")
        for i in range(1401, 1413):
            top_earlgrey[i] = "//"+top_earlgrey[i]


        #then add/remove lines, because
        #list index will not match line number
        print("Add dla fmc ports")
        s = (   "  //dla fmc\n"
                "  output              dla_clk,\n"
                "  output              dla_rst_n,\n"
                "  input               intr_dla_done,\n"
                "  input               dla_idle,\n"
                "  output tlul_pkg::tl_h2d_t dla_tl_req,\n"
                "  input tlul_pkg::tl_d2h_t dla_tl_rsp,\n"
                "\n"
            )
        top_earlgrey.insert(29, s)

        print("Assign dla fmc ports")
        s = (   "  assign dla_clk = clkmgr_clocks.clk_main_dla;\n"
                "  assign dla_rst_n = rstmgr_resets.rst_sys_n[rstmgr_pkg::Domain0Sel];\n"
                "  assign clkmgr_idle[4] = dla_idle;\n"
                "\n"
                "\n"
            )
        top_earlgrey.insert(385, s)


        writer.writelines(top_earlgrey)


if (runs[3]):
    #file 4, pins_nexysvideo.xdc
    read_path = r'hw/top_earlgrey/data/pins_nexysvideo.xdc'
    write_path = r'build/lowrisc_systems_top_earlgrey_nexysvideo_0.1/src/lowrisc_systems_top_earlgrey_nexysvideo_0.1/data/pins_nexysvideo.xdc'
    os.rename(write_path, write_path+".orig")
    #write_path = r'tmp.xdc'
    with open(read_path, 'r') as reader, open(write_path, 'w') as writer:
        xdc = reader.readlines()

        xdc[48] = "set_property -dict { PACKAGE_PIN R19   IOSTANDARD LVCMOS25} [get_ports { IO_RST_N }]; #IO_L12N_T1_MRCC_35 Sch=cpu_resetn\n"
        xdc[331] = "set_property CONFIG_VOLTAGE 2.5 [current_design]\n"
        
        #do all the modifications first
        s = (
            "set_property -dict { PACKAGE_PIN C27   IOSTANDARD LVDS_25 } [get_ports { axi_c2c_selio_tx_diff_clk_out_n }]; #IO_L13N_T2_MRCC_16 Sch=fmc_la_n[00]\n"
            "set_property -dict { PACKAGE_PIN D27   IOSTANDARD LVDS_25 } [get_ports { axi_c2c_selio_tx_diff_clk_out_p }]; #IO_L13P_T2_MRCC_16 Sch=fmc_la_p[00]\n"
            "set_property -dict { PACKAGE_PIN C26   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_tx_data_out[0] }]; #IO_L11N_T1_SRCC_16 Sch=fmc_la_n[01]\n"
            "set_property -dict { PACKAGE_PIN D26   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_tx_data_out[1] }]; #IO_L11P_T1_SRCC_16 Sch=fmc_la_p[01]\n"
            "set_property -dict { PACKAGE_PIN G30   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_tx_data_out[2] }]; #IO_L24N_T3_16 Sch=fmc_la_n[02]\n"
            "set_property -dict { PACKAGE_PIN H30   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_tx_data_out[3] }]; #IO_L24P_T3_16 Sch=fmc_la_p[02]\n"
            "set_property -dict { PACKAGE_PIN E30   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_tx_data_out[4] }]; #IO_L18N_T2_16 Sch=fmc_la_n[03]\n"
            "set_property -dict { PACKAGE_PIN E29   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_tx_data_out[5] }]; #IO_L18P_T2_16 Sch=fmc_la_p[03]\n"
            "set_property -dict { PACKAGE_PIN H27   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_tx_data_out[6] }]; #IO_L23N_T3_16 Sch=fmc_la_n[04]\n"
            "set_property -dict { PACKAGE_PIN H26   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_tx_data_out[7] }]; #IO_L23P_T3_16 Sch=fmc_la_p[04]\n"
            "set_property -dict { PACKAGE_PIN A30   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_tx_data_out[8] }]; #IO_L17N_T2_16 Sch=fmc_la_n[05]\n"
            "set_property -dict { PACKAGE_PIN B30   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_tx_data_out[9] }]; #IO_L17P_T2_16 Sch=fmc_la_p[05]\n"
            "set_property -dict { PACKAGE_PIN C30   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_tx_data_out[10] }]; #IO_L16N_T2_16 Sch=fmc_la_n[06]\n"
            "set_property -dict { PACKAGE_PIN D29   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_tx_data_out[11] }]; #IO_L16P_T2_16 Sch=fmc_la_p[06]\n"
            "set_property -dict { PACKAGE_PIN E25   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_tx_data_out[12] }]; #IO_L3N_T0_DQS_16 Sch=fmc_la_n[07]\n"
            "set_property -dict { PACKAGE_PIN F25   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_tx_data_out[13] }]; #IO_L3P_T0_DQS_16 Sch=fmc_la_p[07]\n"
            "set_property -dict { PACKAGE_PIN B29   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_tx_data_out[14] }]; #IO_L15N_T2_DQS_16 Sch=fmc_la_n[08]\n"
            "set_property -dict { PACKAGE_PIN C29   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_tx_data_out[15] }]; #IO_L15P_T2_DQS_16 Sch=fmc_la_p[08]\n"
            "set_property -dict { PACKAGE_PIN A28   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_tx_data_out[16] }]; #IO_L9N_T1_DQS_16 Sch=fmc_la_n[09]\n"
            "set_property -dict { PACKAGE_PIN B28   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_tx_data_out[17] }]; #IO_L9P_T1_DQS_16 Sch=fmc_la_p[09]\n"
            "set_property -dict { PACKAGE_PIN A27   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_tx_data_out[18] }]; #IO_L7N_T1_16 Sch=fmc_la_n[10]\n"
            "set_property -dict { PACKAGE_PIN B27   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_tx_data_out[19] }]; #IO_L7P_T1_16 Sch=fmc_la_p[10]\n"
            "set_property -dict { PACKAGE_PIN A26   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_tx_data_out[20] }]; #IO_L10N_T1_16 Sch=fmc_la_n[11]\n"
            "set_property -dict { PACKAGE_PIN A25   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_tx_data_out[21] }]; #IO_L10P_T1_16 Sch=fmc_la_p[11]\n"
            "set_property -dict { PACKAGE_PIN E26   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_tx_data_out[22] }]; #IO_L5N_T0_16 Sch=fmc_la_n[12]\n"
            "set_property -dict { PACKAGE_PIN F26   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_tx_data_out[23] }]; #IO_L5P_T0_16 Sch=fmc_la_p[12]\n"
            "set_property -dict { PACKAGE_PIN D24   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_tx_data_out[24] }]; #IO_L4N_T0_16 Sch=fmc_la_n[13]\n"
            "set_property -dict { PACKAGE_PIN E24   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_tx_data_out[25] }]; #IO_L4P_T0_16 Sch=fmc_la_p[13]\n"
            "set_property -dict { PACKAGE_PIN B24   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_tx_data_out[26] }]; #IO_L8N_T1_16 Sch=fmc_la_n[14]\n"
            "set_property -dict { PACKAGE_PIN C24   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_tx_data_out[27] }]; #IO_L8P_T1_16 Sch=fmc_la_p[14]\n"
            "set_property -dict { PACKAGE_PIN A23   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_tx_data_out[28] }]; #IO_L1N_T0_16 Sch=fmc_la_n[15]\n"
            "set_property -dict { PACKAGE_PIN B23   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_tx_data_out[29] }]; #IO_L1P_T0_16 Sch=fmc_la_p[15]\n"
            "set_property -dict { PACKAGE_PIN D23   IOSTANDARD LVCMOS25 } [get_ports { FMC_DLA_INTR_DONE }]; #IO_L2N_T0_16 Sch=fmc_la_n[16]\n"
            "set_property -dict { PACKAGE_PIN E23   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_tx_data_out[30] }]; #IO_L2P_T0_16 Sch=fmc_la_p[16]\n"
            "set_property -dict { PACKAGE_PIN E21   IOSTANDARD LVDS_25 } [get_ports { axi_c2c_selio_rx_diff_clk_in_n }]; #IO_L11N_T1_SRCC_17 Sch=fmc_la_n[17]\n"
            "set_property -dict { PACKAGE_PIN F21   IOSTANDARD LVDS_25 } [get_ports { axi_c2c_selio_rx_diff_clk_in_p }]; #IO_L11P_T1_SRCC_17 Sch=fmc_la_p[17]\n"
            "set_property -dict { PACKAGE_PIN D18   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_rx_data_in[0] }]; #IO_L13N_T2_MRCC_17 Sch=fmc_la_n[18]\n"
            "set_property -dict { PACKAGE_PIN D17   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_rx_data_in[1] }]; #IO_L13P_T2_MRCC_17 Sch=fmc_la_p[18]\n"
            "set_property -dict { PACKAGE_PIN H22   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_rx_data_in[2] }]; #IO_L7N_T1_17 Sch=fmc_la_n[19]\n"
            "set_property -dict { PACKAGE_PIN H21   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_rx_data_in[3] }]; #IO_L7P_T1_17 Sch=fmc_la_p[19]\n"
            "set_property -dict { PACKAGE_PIN F22   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_rx_data_in[4] }]; #IO_L9N_T1_DQS_17 Sch=fmc_la_n[20]\n"
            "set_property -dict { PACKAGE_PIN G22   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_rx_data_in[5] }]; #IO_L9P_T1_DQS_17 Sch=fmc_la_p[20]\n"
            "set_property -dict { PACKAGE_PIN L18   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_rx_data_in[6] }]; #IO_L5N_T0_17 Sch=fmc_la_n[21]\n"
            "set_property -dict { PACKAGE_PIN L17   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_rx_data_in[7] }]; #IO_L5P_T0_17 Sch=fmc_la_p[21]\n"
            "set_property -dict { PACKAGE_PIN H17   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_rx_data_in[8] }]; #IO_L3N_T0_DQS_17 Sch=fmc_la_n[22]\n"
            "set_property -dict { PACKAGE_PIN J17   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_rx_data_in[9] }]; #IO_L3P_T0_DQS_17 Sch=fmc_la_p[22]\n"
            "set_property -dict { PACKAGE_PIN F17   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_rx_data_in[10] }]; #IO_L18N_T2_17 Sch=fmc_la_n[23]\n"
            "set_property -dict { PACKAGE_PIN G17   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_rx_data_in[11] }]; #IO_L18P_T2_17 Sch=fmc_la_p[23]\n"
            "set_property -dict { PACKAGE_PIN G20   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_rx_data_in[12] }]; #IO_L2N_T0_17 Sch=fmc_la_n[24]\n"
            "set_property -dict { PACKAGE_PIN H20   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_rx_data_in[13] }]; #IO_L2P_T0_17 Sch=fmc_la_p[24]\n"
            "set_property -dict { PACKAGE_PIN C22   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_rx_data_in[14] }]; #IO_L10N_T1_17 Sch=fmc_la_n[25]\n"
            "set_property -dict { PACKAGE_PIN D22   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_rx_data_in[15] }]; #IO_L10P_T1_17 Sch=fmc_la_p[25]\n"
            "set_property -dict { PACKAGE_PIN A22   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_rx_data_in[16] }]; #IO_L23N_T3_17 Sch=fmc_la_n[26]\n"
            "set_property -dict { PACKAGE_PIN B22   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_rx_data_in[17] }]; #IO_L23P_T3_17 Sch=fmc_la_p[26]\n"
            "set_property -dict { PACKAGE_PIN A21   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_rx_data_in[18] }]; #IO_L21N_T3_DQS_17 Sch=fmc_la_n[27]\n"
            "set_property -dict { PACKAGE_PIN A20   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_rx_data_in[19] }]; #IO_L21P_T3_DQS_17 Sch=fmc_la_p[27]\n"
            "set_property -dict { PACKAGE_PIN H19   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_rx_data_in[20] }]; #IO_L4N_T0_17 Sch=fmc_la_n[28]\n"
            "set_property -dict { PACKAGE_PIN J19   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_rx_data_in[21] }]; #IO_L4P_T0_17 Sch=fmc_la_p[28]\n"
            "set_property -dict { PACKAGE_PIN A18   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_rx_data_in[22] }]; #IO_L22N_T3_17 Sch=fmc_la_n[29]\n"
            "set_property -dict { PACKAGE_PIN B18   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_rx_data_in[23] }]; #IO_L22P_T3_17 Sch=fmc_la_p[29]\n"
            "set_property -dict { PACKAGE_PIN A17   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_rx_data_in[24] }]; #IO_L20N_T3_17 Sch=fmc_la_n[30]\n"
            "set_property -dict { PACKAGE_PIN A16   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_rx_data_in[25] }]; #IO_L20P_T3_17 Sch=fmc_la_p[30]\n"
            "set_property -dict { PACKAGE_PIN B17   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_rx_data_in[26] }]; #IO_L17N_T2_17 Sch=fmc_la_n[31]\n"
            "set_property -dict { PACKAGE_PIN C17   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_rx_data_in[27] }]; #IO_L17P_T2_17 Sch=fmc_la_p[31]\n"
            "set_property -dict { PACKAGE_PIN J18   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_rx_data_in[28] }]; #IO_L1N_T0_17 Sch=fmc_la_n[32]\n"
            "set_property -dict { PACKAGE_PIN K18   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_rx_data_in[29] }]; #IO_L1P_T0_17 Sch=fmc_la_p[32]\n"
            "set_property -dict { PACKAGE_PIN C16   IOSTANDARD LVCMOS25 } [get_ports { FMC_DLA_IDLE }]; #IO_L15N_T2_DQS_17 Sch=fmc_la_n[33]\n"
            "set_property -dict { PACKAGE_PIN D16   IOSTANDARD LVCMOS25 } [get_ports { axi_c2c_selio_rx_data_in[30] }]; #IO_L15P_T2_DQS_17 Sch=fmc_la_p[33]\n"
            "\n"
        )
    
        del xdc[256:328]
        xdc.insert(256, s)

        s = (
            "set_property -dict { PACKAGE_PIN V29   IOSTANDARD LVCMOS25 DRIVE 8 SLEW FAST } [get_ports { IO_USB_DP0 }]; #IO_L21P_T3_DQS_34 Sch=jb_p[1]\n"
            "set_property -dict { PACKAGE_PIN V30   IOSTANDARD LVCMOS25 DRIVE 8 SLEW FAST } [get_ports { IO_USB_DN0 }]; #IO_L21N_T3_DQS_34 Sch=jb_n[1]\n"
            "set_property -dict { PACKAGE_PIN V25   IOSTANDARD LVCMOS25 } [get_ports { IO_USB_DPPULLUP0 }]; #IO_L19P_T3_34 Sch=jb_p[2]\n"
            "set_property -dict { PACKAGE_PIN W26   IOSTANDARD LVCMOS25 } [get_ports { IO_USB_SENSE0 }]; #IO_L19N_T3_VREF_34 Sch=jb_n[2]\n"
            "set_property -dict { PACKAGE_PIN U22   IOSTANDARD LVCMOS25 } [get_ports { IO_USB_DNPULLUP0 }]; #IO_L23P_T3_34 Sch=jb_p[4]\n"
            )
        del xdc[130:135]
        xdc.insert(130, s)

        s = (
            "set_property -dict { PACKAGE_PIN G19   IOSTANDARD LVCMOS25 } [get_ports { IO_GP0 }]; #IO_L22P_T3_16 Sch=sw[0]\n"
            "set_property -dict { PACKAGE_PIN G25   IOSTANDARD LVCMOS25 } [get_ports { IO_GP1 }]; #IO_25_16 Sch=sw[1]\n"
            "set_property -dict { PACKAGE_PIN H24   IOSTANDARD LVCMOS25 } [get_ports { IO_GP2 }]; #IO_L24P_T3_16 Sch=sw[2]\n"
            "set_property -dict { PACKAGE_PIN K19   IOSTANDARD LVCMOS25 } [get_ports { IO_GP3 }]; #IO_L24N_T3_16 Sch=sw[3]\n"
            "set_property -dict { PACKAGE_PIN N19   IOSTANDARD LVCMOS25 } [get_ports { IO_GP4 }]; #IO_L6P_T0_15 Sch=sw[4]\n"
            "set_property -dict { PACKAGE_PIN P19   IOSTANDARD LVCMOS25 } [get_ports { IO_GP5 }]; #IO_0_15 Sch=sw[5]\n"
            "set_property -dict { PACKAGE_PIN P26   IOSTANDARD LVCMOS25 } [get_ports { IO_GP6 }]; #IO_L19P_T3_A22_15 Sch=sw[6]\n"
            "set_property -dict { PACKAGE_PIN P27   IOSTANDARD LVCMOS25 } [get_ports { IO_GP7 }]; #IO_25_15 Sch=sw[7]\n"
            )
        del xdc[52:60]
        xdc.insert(52, s)

        s = (
            "set_property -dict { PACKAGE_PIN K29   IOSTANDARD LVCMOS25 } [get_ports { IO_GP8  }]; #IO_L13N_T2_MRCC_15 Sch=fmc_ha_n[00]\n"
            "set_property -dict { PACKAGE_PIN K28   IOSTANDARD LVCMOS25 } [get_ports { IO_GP9  }]; #IO_L13P_T2_MRCC_15 Sch=fmc_ha_p[00]\n"
            "set_property -dict { PACKAGE_PIN L28   IOSTANDARD LVCMOS25 } [get_ports { IO_GP10 }]; #IO_L14N_T2_SRCC_15 Sch=fmc_ha_n[01]\n"
            "set_property -dict { PACKAGE_PIN M28   IOSTANDARD LVCMOS25 } [get_ports { IO_GP11 }]; #IO_L14P_T2_SRCC_15 Sch=fmc_ha_p[01]\n"
            "set_property -dict { PACKAGE_PIN P22   IOSTANDARD LVCMOS25 } [get_ports { IO_GP12 }]; #IO_L22N_T3_A16_15 Sch=fmc_ha_n[02]\n"
            "set_property -dict { PACKAGE_PIN P21   IOSTANDARD LVCMOS25 } [get_ports { IO_GP13 }]; #IO_L22P_T3_A17_15 Sch=fmc_ha_p[02]\n"
            "set_property -dict { PACKAGE_PIN N26   IOSTANDARD LVCMOS25 } [get_ports { IO_GP14 }]; #IO_L18N_T2_A23_15 Sch=fmc_ha_n[03]\n"
            "set_property -dict { PACKAGE_PIN N25   IOSTANDARD LVCMOS25 } [get_ports { IO_GP15 }]; #IO_L18P_T2_A24_15 Sch=fmc_ha_p[03]\n"
            "\n"
            "set_property -dict { PACKAGE_PIN T28   IOSTANDARD LVCMOS25 } [get_ports { led[0] }]; #IO_L15P_T2_DQS_13 Sch=led[0]\n"
            "set_property -dict { PACKAGE_PIN V19   IOSTANDARD LVCMOS25 } [get_ports { led[1] }]; #IO_L15N_T2_DQS_13 Sch=led[1]\n"
            "set_property -dict { PACKAGE_PIN U30   IOSTANDARD LVCMOS25 } [get_ports { led[2] }]; #IO_L17P_T2_13 Sch=led[2]\n"
            "set_property -dict { PACKAGE_PIN U29   IOSTANDARD LVCMOS25 } [get_ports { led[3] }]; #IO_L17N_T2_13 Sch=led[3]\n"
            "set_property -dict { PACKAGE_PIN V20   IOSTANDARD LVCMOS25 } [get_ports { led[4] }]; #IO_L14N_T2_SRCC_13 Sch=led[4]\n"
            "set_property -dict { PACKAGE_PIN V26   IOSTANDARD LVCMOS25 } [get_ports { led[5] }]; #IO_L16N_T2_13 Sch=led[5]\n"
            "set_property -dict { PACKAGE_PIN W24   IOSTANDARD LVCMOS25 } [get_ports { led[6] }]; #IO_L16P_T2_13 Sch=led[6]\n"
            "set_property -dict { PACKAGE_PIN W23   IOSTANDARD LVCMOS25 } [get_ports { led[7] }]; #IO_L5P_T0_13 Sch=led[7]\n"
            )
        del xdc[32:40]
        xdc.insert(32, s)


        writer.writelines(xdc)
        


if (runs[4]):
    #file 5, clkgen_xil7series.sv
    read_path = r'hw/top_earlgrey/rtl/clkgen_xil7series.sv'
    write_path = r'build/lowrisc_systems_top_earlgrey_nexysvideo_0.1/src/lowrisc_systems_top_earlgrey_nexysvideo_0.1/rtl/clkgen_xil7series.sv'
    os.rename(write_path, write_path+".orig")
    with open(read_path, 'r') as reader, open(write_path, 'w') as writer:
        clkgen = reader.readlines()
        
        clkgen[55] = "    .CLKOUT2             (clk_100_unbuf),\n"
        clkgen[56] = "    .CLKOUT3             (clk_200_unbuf),\n"

        s = (
            "  BUFG clk_100_bufg (\n"
            "      .I (clk_100_unbuf),\n"
            "      .O (clk_100_buf)\n"
            "    );\n"
            "\n"
            "  BUFG clk_200_bufg (    \n"
            "    .I (clk_200_unbuf),\n"
            "    .O (clk_200_buf)   \n"
            "  );                   \n"
            "\n"
            "  assign clk_100MHz = clk_100_buf;\n"
            "  assign clk_200MHz = clk_200_buf;\n"
            )
        clkgen.insert(106, s)

        s = (
            "    .CLKOUT2_DIVIDE       (12),\n"
            "    .CLKOUT2_PHASE        (0.000),\n"
            "    .CLKOUT2_DUTY_CYCLE   (0.500),\n"
            "    .CLKOUT3_DIVIDE       (6),\n"
            "    .CLKOUT3_PHASE        (0.000),\n"
            "    .CLKOUT3_DUTY_CYCLE   (0.500),\n"
            )
        clkgen.insert(50, s)

        s = (
            "  logic clk_100_buf;\n"
            "  logic clk_100_unbuf;\n"
            "  logic clk_200_buf;\n"
            "  logic clk_200_unbuf;\n"
            )
        clkgen.insert(24, s)

        s = (
            "  output clk_100MHz,\n"
            "  output clk_200MHz,\n"
            )
        clkgen.insert(13, s)


        writer.writelines(clkgen)