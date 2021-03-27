#ddr3 mig7
create_ip -name mig_7series -version 4.2 -vendor xilinx.com -library ip -module_name mig7
set_property CONFIG.XML_INPUT_FILE "${workroot}/vivado_mig7.prj" [get_ips mig7]

#async fifo
source vivado_ddrif_fifo_u2h.tcl
source vivado_ddrif_fifo_h2u.tcl

