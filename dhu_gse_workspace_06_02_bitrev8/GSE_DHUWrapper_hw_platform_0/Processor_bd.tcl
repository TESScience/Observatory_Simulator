
################################################################
# This is a generated script based on design: Processor
#
# Though there are limitations about the generated script,
# the main purpose of this utility is to make learning
# IP Integrator Tcl commands easier.
################################################################

################################################################
# Check if script is running in correct Vivado version.
################################################################
set scripts_vivado_version 2014.4
set current_vivado_version [version -short]

if { [string first $scripts_vivado_version $current_vivado_version] == -1 } {
   puts ""
   puts "ERROR: This script was generated using Vivado <$scripts_vivado_version> and is being run in <$current_vivado_version> of Vivado. Please run the script in Vivado <$scripts_vivado_version> then open the design in Vivado <$current_vivado_version>. Upgrade the design by running \"Tools => Report => Report IP Status...\", then run write_bd_tcl to create an updated script."

   return 1
}

################################################################
# START
################################################################

# To test this script, run the following commands from Vivado Tcl console:
# source Processor_script.tcl

# If you do not already have a project created,
# you can create a project using the following command:
#    create_project project_1 myproj -part xc7z045ffg900-2
#    set_property BOARD_PART xilinx.com:zc706:part0:1.1 [current_project]


# CHANGE DESIGN NAME HERE
set design_name Processor

# If you do not already have an existing IP Integrator design open,
# you can create a design using the following command:
#    create_bd_design $design_name

# CHECKING IF PROJECT EXISTS
if { [get_projects -quiet] eq "" } {
   puts "ERROR: Please open or create a project!"
   return 1
}


# Creating design if needed
set errMsg ""
set nRet 0

set cur_design [current_bd_design -quiet]
set list_cells [get_bd_cells -quiet]

if { ${design_name} eq "" } {
   # USE CASES:
   #    1) Design_name not set

   set errMsg "ERROR: Please set the variable <design_name> to a non-empty value."
   set nRet 1

} elseif { ${cur_design} ne "" && ${list_cells} eq "" } {
   # USE CASES:
   #    2): Current design opened AND is empty AND names same.
   #    3): Current design opened AND is empty AND names diff; design_name NOT in project.
   #    4): Current design opened AND is empty AND names diff; design_name exists in project.

   if { $cur_design ne $design_name } {
      puts "INFO: Changing value of <design_name> from <$design_name> to <$cur_design> since current design is empty."
      set design_name [get_property NAME $cur_design]
   }
   puts "INFO: Constructing design in IPI design <$cur_design>..."

} elseif { ${cur_design} ne "" && $list_cells ne "" && $cur_design eq $design_name } {
   # USE CASES:
   #    5) Current design opened AND has components AND same names.

   set errMsg "ERROR: Design <$design_name> already exists in your project, please set the variable <design_name> to another value."
   set nRet 1
} elseif { [get_files -quiet ${design_name}.bd] ne "" } {
   # USE CASES: 
   #    6) Current opened design, has components, but diff names, design_name exists in project.
   #    7) No opened design, design_name exists in project.

   set errMsg "ERROR: Design <$design_name> already exists in your project, please set the variable <design_name> to another value."
   set nRet 2

} else {
   # USE CASES:
   #    8) No opened design, design_name not in project.
   #    9) Current opened design, has components, but diff names, design_name not in project.

   puts "INFO: Currently there is no design <$design_name> in project, so creating one..."

   create_bd_design $design_name

   puts "INFO: Making design <$design_name> as current_bd_design."
   current_bd_design $design_name

}

puts "INFO: Currently the variable <design_name> is equal to \"$design_name\"."

if { $nRet != 0 } {
   puts $errMsg
   return $nRet
}

##################################################################
# DESIGN PROCs
##################################################################



# Procedure to create entire design; Provide argument to make
# procedure reusable. If parentCell is "", will use root.
proc create_root_design { parentCell } {

  if { $parentCell eq "" } {
     set parentCell [get_bd_cells /]
  }

  # Get object for parentCell
  set parentObj [get_bd_cells $parentCell]
  if { $parentObj == "" } {
     puts "ERROR: Unable to find parent cell <$parentCell>!"
     return
  }

  # Make sure parentObj is hier blk
  set parentType [get_property TYPE $parentObj]
  if { $parentType ne "hier" } {
     puts "ERROR: Parent <$parentObj> has TYPE = <$parentType>. Expected to be <hier>."
     return
  }

  # Save current instance; Restore later
  set oldCurInst [current_bd_instance .]

  # Set parent object as current
  current_bd_instance $parentObj


  # Create interface ports
  set DDR [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:ddrx_rtl:1.0 DDR ]
  set FIXED_IO [ create_bd_intf_port -mode Master -vlnv xilinx.com:display_processing_system7:fixedio_rtl:1.0 FIXED_IO ]
  set led_4bits [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:gpio_rtl:1.0 led_4bits ]

  # Create ports
  set DDR_Rdy_Cam1_Int [ create_bd_port -dir I -type intr DDR_Rdy_Cam1_Int ]
  set DDR_Rdy_Cam2_Int [ create_bd_port -dir I -type intr DDR_Rdy_Cam2_Int ]
  set DDR_Req_Cam1_Int [ create_bd_port -dir I -type intr DDR_Req_Cam1_Int ]
  set DDR_Req_Cam2_Int [ create_bd_port -dir I -type intr DDR_Req_Cam2_Int ]
  set DHU_Cam1_Int [ create_bd_port -dir I -type intr DHU_Cam1_Int ]
  set DHU_Cam2_Int [ create_bd_port -dir I -type intr DHU_Cam2_Int ]
  set PL_Clock [ create_bd_port -dir O -type clk PL_Clock ]
  set Proc_Data1_Addr [ create_bd_port -dir O -from 12 -to 0 Proc_Data1_Addr ]
  set Proc_Data1_Clk [ create_bd_port -dir O -type clk Proc_Data1_Clk ]
  set Proc_Data1_En [ create_bd_port -dir O Proc_Data1_En ]
  set Proc_Data1_Rdata [ create_bd_port -dir I -from 31 -to 0 -type data Proc_Data1_Rdata ]
  set Proc_Data1_Wdata [ create_bd_port -dir O -from 31 -to 0 -type data Proc_Data1_Wdata ]
  set Proc_Data1_We [ create_bd_port -dir O -from 3 -to 0 Proc_Data1_We ]
  set Proc_Data2_Addr [ create_bd_port -dir O -from 12 -to 0 Proc_Data2_Addr ]
  set Proc_Data2_Clk [ create_bd_port -dir O -type clk Proc_Data2_Clk ]
  set Proc_Data2_En [ create_bd_port -dir O Proc_Data2_En ]
  set Proc_Data2_Rdata [ create_bd_port -dir I -from 31 -to 0 -type data Proc_Data2_Rdata ]
  set Proc_Data2_Wdata [ create_bd_port -dir O -from 31 -to 0 -type data Proc_Data2_Wdata ]
  set Proc_Data2_We [ create_bd_port -dir O -from 3 -to 0 Proc_Data2_We ]
  set Proc_PCIe_Addr [ create_bd_port -dir O -from 12 -to 0 Proc_PCIe_Addr ]
  set Proc_PCIe_Clk [ create_bd_port -dir O -type clk Proc_PCIe_Clk ]
  set Proc_PCIe_En [ create_bd_port -dir O Proc_PCIe_En ]
  set Proc_PCIe_Rdata [ create_bd_port -dir I -from 31 -to 0 -type data Proc_PCIe_Rdata ]
  set Proc_PCIe_Wdata [ create_bd_port -dir O -from 31 -to 0 -type data Proc_PCIe_Wdata ]
  set Proc_PCIe_We [ create_bd_port -dir O -from 3 -to 0 Proc_PCIe_We ]
  set reset_rtl [ create_bd_port -dir I -type rst reset_rtl ]
  set_property -dict [ list CONFIG.POLARITY {ACTIVE_HIGH}  ] $reset_rtl

  # Create instance: DDR1_bram_ctrl, and set properties
  set DDR1_bram_ctrl [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_bram_ctrl:4.0 DDR1_bram_ctrl ]
  set_property -dict [ list CONFIG.SINGLE_PORT_BRAM {1}  ] $DDR1_bram_ctrl

  # Create instance: DDR2_bram_ctrl, and set properties
  set DDR2_bram_ctrl [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_bram_ctrl:4.0 DDR2_bram_ctrl ]
  set_property -dict [ list CONFIG.SINGLE_PORT_BRAM {1}  ] $DDR2_bram_ctrl

  # Create instance: PCIe_bram_ctrl, and set properties
  set PCIe_bram_ctrl [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_bram_ctrl:4.0 PCIe_bram_ctrl ]
  set_property -dict [ list CONFIG.SINGLE_PORT_BRAM {1}  ] $PCIe_bram_ctrl

  # Create instance: axi_gpio_0, and set properties
  set axi_gpio_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_gpio:2.0 axi_gpio_0 ]
  set_property -dict [ list CONFIG.GPIO_BOARD_INTERFACE {led_4bits} CONFIG.USE_BOARD_FLOW {true}  ] $axi_gpio_0

  # Create instance: axi_mem_intercon, and set properties
  set axi_mem_intercon [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_interconnect:2.1 axi_mem_intercon ]
  set_property -dict [ list CONFIG.NUM_MI {4}  ] $axi_mem_intercon

  # Create instance: proc_sys_reset_0, and set properties
  set proc_sys_reset_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:proc_sys_reset:5.0 proc_sys_reset_0 ]

  # Create instance: processing_system7_0, and set properties
  set processing_system7_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:processing_system7:5.5 processing_system7_0 ]
  set_property -dict [ list CONFIG.PCW_APU_PERIPHERAL_FREQMHZ {800} CONFIG.PCW_CORE0_FIQ_INTR {0} CONFIG.PCW_CORE0_IRQ_INTR {0} CONFIG.PCW_CORE1_FIQ_INTR {0} CONFIG.PCW_CORE1_IRQ_INTR {0} CONFIG.PCW_ENET0_PERIPHERAL_FREQMHZ {1000 Mbps} CONFIG.PCW_FPGA0_PERIPHERAL_FREQMHZ {200} CONFIG.PCW_IRQ_F2P_INTR {1} CONFIG.PCW_UIPARAM_DDR_FREQ_MHZ {300} CONFIG.PCW_USE_FABRIC_INTERRUPT {1} CONFIG.preset {ZC702*}  ] $processing_system7_0

  # Create instance: xlconcat_0, and set properties
  set xlconcat_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlconcat:2.1 xlconcat_0 ]
  set_property -dict [ list CONFIG.NUM_PORTS {6}  ] $xlconcat_0

  # Create interface connections
  connect_bd_intf_net -intf_net axi_gpio_0_GPIO [get_bd_intf_ports led_4bits] [get_bd_intf_pins axi_gpio_0/GPIO]
  connect_bd_intf_net -intf_net axi_mem_intercon_M00_AXI [get_bd_intf_pins PCIe_bram_ctrl/S_AXI] [get_bd_intf_pins axi_mem_intercon/M00_AXI]
  connect_bd_intf_net -intf_net axi_mem_intercon_M01_AXI [get_bd_intf_pins DDR1_bram_ctrl/S_AXI] [get_bd_intf_pins axi_mem_intercon/M01_AXI]
  connect_bd_intf_net -intf_net axi_mem_intercon_M02_AXI [get_bd_intf_pins DDR2_bram_ctrl/S_AXI] [get_bd_intf_pins axi_mem_intercon/M02_AXI]
  connect_bd_intf_net -intf_net axi_mem_intercon_M03_AXI [get_bd_intf_pins axi_gpio_0/S_AXI] [get_bd_intf_pins axi_mem_intercon/M03_AXI]
  connect_bd_intf_net -intf_net processing_system7_0_DDR [get_bd_intf_ports DDR] [get_bd_intf_pins processing_system7_0/DDR]
  connect_bd_intf_net -intf_net processing_system7_0_FIXED_IO [get_bd_intf_ports FIXED_IO] [get_bd_intf_pins processing_system7_0/FIXED_IO]
  connect_bd_intf_net -intf_net processing_system7_0_M_AXI_GP0 [get_bd_intf_pins axi_mem_intercon/S00_AXI] [get_bd_intf_pins processing_system7_0/M_AXI_GP0]

  # Create port connections
  connect_bd_net -net DDR_Rdy_Cam1_Int_1 [get_bd_ports DDR_Rdy_Cam1_Int] [get_bd_pins xlconcat_0/In1]
  connect_bd_net -net DDR_Rdy_Cam2_Int_1 [get_bd_ports DDR_Rdy_Cam2_Int] [get_bd_pins xlconcat_0/In4]
  connect_bd_net -net DDR_Req_Cam1_Int_1 [get_bd_ports DDR_Req_Cam1_Int] [get_bd_pins xlconcat_0/In2]
  connect_bd_net -net DDR_Req_Cam2_Int_1 [get_bd_ports DDR_Req_Cam2_Int] [get_bd_pins xlconcat_0/In5]
  connect_bd_net -net DHU_Cam1_Int_1 [get_bd_ports DHU_Cam1_Int] [get_bd_pins xlconcat_0/In0]
  connect_bd_net -net DHU_Cam2_Int_1 [get_bd_ports DHU_Cam2_Int] [get_bd_pins xlconcat_0/In3]
  connect_bd_net -net Proc_Data1_Rdata_1 [get_bd_ports Proc_Data1_Rdata] [get_bd_pins DDR1_bram_ctrl/bram_rddata_a]
  connect_bd_net -net Proc_Data2_Rdata_1 [get_bd_ports Proc_Data2_Rdata] [get_bd_pins DDR2_bram_ctrl/bram_rddata_a]
  connect_bd_net -net Proc_PCIe_Rdata_1 [get_bd_ports Proc_PCIe_Rdata] [get_bd_pins PCIe_bram_ctrl/bram_rddata_a]
  connect_bd_net -net axi_bram_ctrl_0_bram_addr_a [get_bd_ports Proc_PCIe_Addr] [get_bd_pins PCIe_bram_ctrl/bram_addr_a]
  connect_bd_net -net axi_bram_ctrl_0_bram_clk_a [get_bd_ports Proc_PCIe_Clk] [get_bd_pins PCIe_bram_ctrl/bram_clk_a]
  connect_bd_net -net axi_bram_ctrl_0_bram_en_a [get_bd_ports Proc_PCIe_En] [get_bd_pins PCIe_bram_ctrl/bram_en_a]
  connect_bd_net -net axi_bram_ctrl_0_bram_we_a [get_bd_ports Proc_PCIe_We] [get_bd_pins PCIe_bram_ctrl/bram_we_a]
  connect_bd_net -net axi_bram_ctrl_0_bram_wrdata_a [get_bd_ports Proc_PCIe_Wdata] [get_bd_pins PCIe_bram_ctrl/bram_wrdata_a]
  connect_bd_net -net axi_bram_ctrl_1_bram_addr_a [get_bd_ports Proc_Data1_Addr] [get_bd_pins DDR1_bram_ctrl/bram_addr_a]
  connect_bd_net -net axi_bram_ctrl_1_bram_clk_a [get_bd_ports Proc_Data1_Clk] [get_bd_pins DDR1_bram_ctrl/bram_clk_a]
  connect_bd_net -net axi_bram_ctrl_1_bram_en_a [get_bd_ports Proc_Data1_En] [get_bd_pins DDR1_bram_ctrl/bram_en_a]
  connect_bd_net -net axi_bram_ctrl_1_bram_we_a [get_bd_ports Proc_Data1_We] [get_bd_pins DDR1_bram_ctrl/bram_we_a]
  connect_bd_net -net axi_bram_ctrl_1_bram_wrdata_a [get_bd_ports Proc_Data1_Wdata] [get_bd_pins DDR1_bram_ctrl/bram_wrdata_a]
  connect_bd_net -net axi_bram_ctrl_2_bram_addr_a [get_bd_ports Proc_Data2_Addr] [get_bd_pins DDR2_bram_ctrl/bram_addr_a]
  connect_bd_net -net axi_bram_ctrl_2_bram_clk_a [get_bd_ports Proc_Data2_Clk] [get_bd_pins DDR2_bram_ctrl/bram_clk_a]
  connect_bd_net -net axi_bram_ctrl_2_bram_en_a [get_bd_ports Proc_Data2_En] [get_bd_pins DDR2_bram_ctrl/bram_en_a]
  connect_bd_net -net axi_bram_ctrl_2_bram_we_a [get_bd_ports Proc_Data2_We] [get_bd_pins DDR2_bram_ctrl/bram_we_a]
  connect_bd_net -net axi_bram_ctrl_2_bram_wrdata_a [get_bd_ports Proc_Data2_Wdata] [get_bd_pins DDR2_bram_ctrl/bram_wrdata_a]
  connect_bd_net -net proc_sys_reset_0_interconnect_aresetn [get_bd_pins axi_mem_intercon/ARESETN] [get_bd_pins proc_sys_reset_0/interconnect_aresetn]
  connect_bd_net -net proc_sys_reset_0_peripheral_aresetn [get_bd_pins DDR1_bram_ctrl/s_axi_aresetn] [get_bd_pins DDR2_bram_ctrl/s_axi_aresetn] [get_bd_pins PCIe_bram_ctrl/s_axi_aresetn] [get_bd_pins axi_gpio_0/s_axi_aresetn] [get_bd_pins axi_mem_intercon/M00_ARESETN] [get_bd_pins axi_mem_intercon/M01_ARESETN] [get_bd_pins axi_mem_intercon/M02_ARESETN] [get_bd_pins axi_mem_intercon/M03_ARESETN] [get_bd_pins axi_mem_intercon/S00_ARESETN] [get_bd_pins proc_sys_reset_0/peripheral_aresetn]
  connect_bd_net -net processing_system7_0_FCLK_CLK0 [get_bd_ports PL_Clock] [get_bd_pins DDR1_bram_ctrl/s_axi_aclk] [get_bd_pins DDR2_bram_ctrl/s_axi_aclk] [get_bd_pins PCIe_bram_ctrl/s_axi_aclk] [get_bd_pins axi_gpio_0/s_axi_aclk] [get_bd_pins axi_mem_intercon/ACLK] [get_bd_pins axi_mem_intercon/M00_ACLK] [get_bd_pins axi_mem_intercon/M01_ACLK] [get_bd_pins axi_mem_intercon/M02_ACLK] [get_bd_pins axi_mem_intercon/M03_ACLK] [get_bd_pins axi_mem_intercon/S00_ACLK] [get_bd_pins proc_sys_reset_0/slowest_sync_clk] [get_bd_pins processing_system7_0/FCLK_CLK0] [get_bd_pins processing_system7_0/M_AXI_GP0_ACLK]
  connect_bd_net -net reset_rtl_1 [get_bd_ports reset_rtl] [get_bd_pins proc_sys_reset_0/ext_reset_in]
  connect_bd_net -net xlconcat_0_dout [get_bd_pins processing_system7_0/IRQ_F2P] [get_bd_pins xlconcat_0/dout]

  # Create address segments
  create_bd_addr_seg -range 0x2000 -offset 0x40000000 [get_bd_addr_spaces processing_system7_0/Data] [get_bd_addr_segs PCIe_bram_ctrl/S_AXI/Mem0] SEG_axi_bram_ctrl_0_Mem0
  create_bd_addr_seg -range 0x2000 -offset 0x40010000 [get_bd_addr_spaces processing_system7_0/Data] [get_bd_addr_segs DDR1_bram_ctrl/S_AXI/Mem0] SEG_axi_bram_ctrl_1_Mem0
  create_bd_addr_seg -range 0x2000 -offset 0x40020000 [get_bd_addr_spaces processing_system7_0/Data] [get_bd_addr_segs DDR2_bram_ctrl/S_AXI/Mem0] SEG_axi_bram_ctrl_2_Mem0
  create_bd_addr_seg -range 0x10000 -offset 0x41200000 [get_bd_addr_spaces processing_system7_0/Data] [get_bd_addr_segs axi_gpio_0/S_AXI/Reg] SEG_axi_gpio_0_Reg
  

  # Restore current instance
  current_bd_instance $oldCurInst

  save_bd_design
}
# End of create_root_design()


##################################################################
# MAIN FLOW
##################################################################

create_root_design ""


