##################################################################
# CHECK VIVADO VERSION
##################################################################

set scripts_vivado_version 2020.1
set current_vivado_version [version -short]

if { [string first $scripts_vivado_version $current_vivado_version] == -1 } {
  catch {common::send_msg_id "IPS_TCL-100" "ERROR" "This script was generated using Vivado <$scripts_vivado_version> and is being run in <$current_vivado_version> of Vivado. Please run the script in Vivado <$scripts_vivado_version> then open the design in Vivado <$current_vivado_version>. Upgrade the design by running \"Tools => Report => Report IP Status...\", then run write_ip_tcl to create an updated script."}
  return 1
}

##################################################################
# START
##################################################################

# To test this script, run the following commands from Vivado Tcl console:
# source vivado_axi_chip2chip_host.tcl
# If there is no project opened, this script will create a
# project, but make sure you do not have an existing project
# <./opentitan-dla-fmc/opentitan-dla-fmc.xpr> in the current working folder.

set list_projs [get_projects -quiet]
if { $list_projs eq "" } {
  create_project opentitan-dla-fmc opentitan-dla-fmc -part xc7k325tffg900-2
  set_property BOARD_PART digilentinc.com:genesys2:part0:1.1 [current_project]
  set_property target_language Verilog [current_project]
  set_property simulator_language Mixed [current_project]
}

##################################################################
# CHECK IPs
##################################################################

set bCheckIPs 1
set bCheckIPsPassed 1
if { $bCheckIPs == 1 } {
  set list_check_ips { xilinx.com:ip:axi_chip2chip:5.0 }
  set list_ips_missing ""
  common::send_msg_id "IPS_TCL-1001" "INFO" "Checking if the following IPs exist in the project's IP catalog: $list_check_ips ."

  foreach ip_vlnv $list_check_ips {
  set ip_obj [get_ipdefs -all $ip_vlnv]
  if { $ip_obj eq "" } {
    lappend list_ips_missing $ip_vlnv
    }
  }

  if { $list_ips_missing ne "" } {
    catch {common::send_msg_id "IPS_TCL-105" "ERROR" "The following IPs are not found in the IP Catalog:\n  $list_ips_missing\n\nResolution: Please add the repository containing the IP(s) to the project." }
    set bCheckIPsPassed 0
  }
}

if { $bCheckIPsPassed != 1 } {
  common::send_msg_id "IPS_TCL-102" "WARNING" "Will not continue with creation of design due to the error(s) above."
  return 1
}

##################################################################
# CREATE IP axi_chip2chip_host
##################################################################

set axi_chip2chip_host [create_ip -name axi_chip2chip -vendor xilinx.com -library ip -version 5.0 -module_name axi_chip2chip_host]

set_property -dict { 
  CONFIG.C_AXI_WUSER_WIDTH {0}
  CONFIG.C_AXI_ID_WIDTH {8}
  CONFIG.C_NUM_OF_IO {64}
  CONFIG.C_INTERFACE_MODE {0}
  CONFIG.C_USE_DIFF_CLK {true}
} [get_ips axi_chip2chip_host]

set_property -dict { 
  GENERATE_SYNTH_CHECKPOINT {1}
} $axi_chip2chip_host

##################################################################

