// Copyright lowRISC contributors.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Register Package auto-generated by `reggen` containing data structure

package dla_reg_pkg;

  ////////////////////////////
  // Typedefs for registers //
  ////////////////////////////
  typedef struct packed {
    logic        q;
  } dla_reg2hw_intr_state_reg_t;

  typedef struct packed {
    logic        q;
  } dla_reg2hw_intr_enable_reg_t;

  typedef struct packed {
    logic        q;
    logic        qe;
  } dla_reg2hw_intr_test_reg_t;

  typedef struct packed {
    struct packed {
      logic        q;
    } ddr2gb;
    struct packed {
      logic        q;
    } gb2lb;
    struct packed {
      logic        q;
    } conv;
    struct packed {
      logic        q;
    } fc;
    struct packed {
      logic        q;
    } ppe;
  } dla_reg2hw_gst_intr_reg_t;

  typedef struct packed {
    logic [15:0] q;
  } dla_reg2hw_gst_enable_row_reg_t;

  typedef struct packed {
    logic [15:0] q;
  } dla_reg2hw_gst_enable_col_reg_t;

  typedef struct packed {
    struct packed {
      logic        q;
    } comp;
    struct packed {
      logic        q;
    } ppe;
  } dla_reg2hw_gst_comp_state_reg_t;

  typedef struct packed {
    struct packed {
      logic        q;
      logic        qe;
    } direction;
    struct packed {
      logic        q;
      logic        qe;
    } go;
  } dla_reg2hw_ddr2gb_ctrl_reg_t;

  typedef struct packed {
    logic [31:0] q;
  } dla_reg2hw_ddr2gb_ddr_addr0_reg_t;

  typedef struct packed {
    logic [21:0] q;
  } dla_reg2hw_ddr2gb_ddr_addr1_reg_t;

  typedef struct packed {
    struct packed {
      logic [10:0] q;
    } addr;
    struct packed {
      logic [2:0]  q;
    } mux;
    struct packed {
      logic [5:0]  q;
    } idx;
    struct packed {
      logic [7:0]  q;
    } burst_len;
  } dla_reg2hw_ddr2gb_gb_addr_reg_t;

  typedef struct packed {
    struct packed {
      logic        q;
      logic        qe;
    } mode;
    struct packed {
      logic        q;
      logic        qe;
    } go;
  } dla_reg2hw_gb2lb_ctrl_reg_t;

  typedef struct packed {
    struct packed {
      logic [10:0] q;
    } src_addr;
    struct packed {
      logic [8:0]  q;
    } dest_addr;
  } dla_reg2hw_gb2lb_addr_reg_t;

  typedef struct packed {
    struct packed {
      logic [10:0] q;
    } skip;
    struct packed {
      logic [5:0]  q;
    } iter;
  } dla_reg2hw_gb2lb_src0_reg_t;

  typedef struct packed {
    struct packed {
      logic [10:0] q;
    } len;
    struct packed {
      logic [10:0] q;
    } dila;
  } dla_reg2hw_gb2lb_src1_reg_t;

  typedef struct packed {
    struct packed {
      logic [1:0]  q;
      logic        qe;
    } mode_comp;
    struct packed {
      logic [1:0]  q;
      logic        qe;
    } mode_spar;
    struct packed {
      logic        q;
      logic        qe;
    } go_conv;
    struct packed {
      logic        q;
      logic        qe;
    } go_fc;
  } dla_reg2hw_comp_ctrl_reg_t;

  typedef struct packed {
    struct packed {
      logic [3:0]  q;
    } k_size;
    struct packed {
      logic [4:0]  q;
    } k_scale;
  } dla_reg2hw_comp_k_size_reg_t;

  typedef struct packed {
    struct packed {
      logic [7:0]  q;
    } if_len;
    struct packed {
      logic [7:0]  q;
    } of_len;
  } dla_reg2hw_comp_f_size_reg_t;

  typedef struct packed {
    struct packed {
      logic [7:0]  q;
    } if_chl;
    struct packed {
      logic [7:0]  q;
    } of_chl;
  } dla_reg2hw_comp_c_size_reg_t;

  typedef struct packed {
    struct packed {
      logic [7:0]  q;
    } pad_left;
    struct packed {
      logic [7:0]  q;
    } pad_right;
    struct packed {
      logic [15:0] q;
    } pad_num;
  } dla_reg2hw_comp_p_size_reg_t;

  typedef struct packed {
    struct packed {
      logic [7:0]  q;
    } col;
    struct packed {
      logic [7:0]  q;
    } row;
  } dla_reg2hw_comp_fbload_reg_t;

  typedef struct packed {
    struct packed {
      logic [8:0]  q;
    } lbuf_addr;
    struct packed {
      logic [9:0] q;
    } wbuf_addr;
    struct packed {
      logic [6:0]  q;
    } ibuf_addr;
  } dla_reg2hw_comp_addr_reg_t;

  typedef struct packed {
    struct packed {
      logic [1:0]  q;
      logic        qe;
    } ctrl;
    struct packed {
      logic [1:0]  q;
      logic        qe;
    } post;
    struct packed {
      logic        q;
      logic        qe;
    } mode;
    struct packed {
      logic        q;
      logic        qe;
    } rst;
    struct packed {
      logic        q;
      logic        qe;
    } act;
    struct packed {
      logic        q;
      logic        qe;
    } elem;
    struct packed {
      logic [1:0]  q;
      logic        qe;
    } bias;
    struct packed {
      logic        q;
      logic        qe;
    } pass;
    struct packed {
      logic [1:0]  q;
      logic        qe;
    } oper;
    struct packed {
      logic [3:0]  q;
      logic        qe;
    } row_num;
    struct packed {
      logic        q;
      logic        qe;
    } go;
  } dla_reg2hw_ppe_ctrl_reg_t;

  typedef struct packed {
    struct packed {
      logic [10:0] q;
    } fbuf_src;
    struct packed {
      logic [10:0] q;
    } fbuf_dest;
  } dla_reg2hw_ppe_fbuf_addr_reg_t;

  typedef struct packed {
    logic [9:0] q;
  } dla_reg2hw_ppe_abuf_addr_reg_t;

  typedef struct packed {
    struct packed {
      logic [10:0] q;
    } src_skip;
    struct packed {
      logic [10:0] q;
    } dest_skip;
  } dla_reg2hw_ppe_skip_reg_t;

  typedef struct packed {
    struct packed {
      logic [10:0] q;
    } src_dila;
    struct packed {
      logic [10:0] q;
    } dest_dila;
  } dla_reg2hw_ppe_dila_reg_t;

  typedef struct packed {
    struct packed {
      logic [5:0]  q;
    } len;
    struct packed {
      logic [6:0]  q;
    } iter;
  } dla_reg2hw_ppe_size_reg_t;

  typedef struct packed {
    struct packed {
      logic [15:0] q;
    } act_k0;
    struct packed {
      logic [15:0] q;
    } act_k1;
  } dla_reg2hw_ppe_act_k0_reg_t;

  typedef struct packed {
    struct packed {
      logic [15:0] q;
    } act_k2;
    struct packed {
      logic [15:0] q;
    } act_k3;
  } dla_reg2hw_ppe_act_k1_reg_t;

  typedef struct packed {
    struct packed {
      logic [15:0] q;
    } act_k4;
    struct packed {
      logic [15:0] q;
    } act_k5;
  } dla_reg2hw_ppe_act_k2_reg_t;

  typedef struct packed {
    struct packed {
      logic [15:0] q;
    } act_k6;
    struct packed {
      logic [15:0] q;
    } act_k7;
  } dla_reg2hw_ppe_act_k3_reg_t;

  typedef struct packed {
    struct packed {
      logic [15:0] q;
    } act_k8;
    struct packed {
      logic [15:0] q;
    } act_k9;
  } dla_reg2hw_ppe_act_k4_reg_t;

  typedef struct packed {
    struct packed {
      logic [15:0] q;
    } act_k10;
    struct packed {
      logic [15:0] q;
    } act_k11;
  } dla_reg2hw_ppe_act_k5_reg_t;

  typedef struct packed {
    struct packed {
      logic [15:0] q;
    } act_k12;
    struct packed {
      logic [15:0] q;
    } act_k13;
  } dla_reg2hw_ppe_act_k6_reg_t;

  typedef struct packed {
    struct packed {
      logic [15:0] q;
    } act_k14;
    struct packed {
      logic [15:0] q;
    } act_k15;
  } dla_reg2hw_ppe_act_k7_reg_t;

  typedef struct packed {
    struct packed {
      logic [15:0] q;
    } act_b0;
    struct packed {
      logic [15:0] q;
    } act_b1;
  } dla_reg2hw_ppe_act_b0_reg_t;

  typedef struct packed {
    struct packed {
      logic [15:0] q;
    } act_b2;
    struct packed {
      logic [15:0] q;
    } act_b3;
  } dla_reg2hw_ppe_act_b1_reg_t;

  typedef struct packed {
    struct packed {
      logic [15:0] q;
    } act_b4;
    struct packed {
      logic [15:0] q;
    } act_b5;
  } dla_reg2hw_ppe_act_b2_reg_t;

  typedef struct packed {
    struct packed {
      logic [15:0] q;
    } act_b6;
    struct packed {
      logic [15:0] q;
    } act_b7;
  } dla_reg2hw_ppe_act_b3_reg_t;

  typedef struct packed {
    struct packed {
      logic [15:0] q;
    } act_b8;
    struct packed {
      logic [15:0] q;
    } act_b9;
  } dla_reg2hw_ppe_act_b4_reg_t;

  typedef struct packed {
    struct packed {
      logic [15:0] q;
    } act_b10;
    struct packed {
      logic [15:0] q;
    } act_b11;
  } dla_reg2hw_ppe_act_b5_reg_t;

  typedef struct packed {
    struct packed {
      logic [15:0] q;
    } act_b12;
    struct packed {
      logic [15:0] q;
    } act_b13;
  } dla_reg2hw_ppe_act_b6_reg_t;

  typedef struct packed {
    struct packed {
      logic [15:0] q;
    } act_b14;
    struct packed {
      logic [15:0] q;
    } act_b15;
  } dla_reg2hw_ppe_act_b7_reg_t;

  typedef struct packed {
    struct packed {
      logic [15:0] q;
    } act_x0;
    struct packed {
      logic [15:0] q;
    } act_x1;
  } dla_reg2hw_ppe_act_x0_reg_t;

  typedef struct packed {
    struct packed {
      logic [15:0] q;
    } act_x2;
    struct packed {
      logic [15:0] q;
    } act_x3;
  } dla_reg2hw_ppe_act_x1_reg_t;

  typedef struct packed {
    struct packed {
      logic [15:0] q;
    } act_x4;
    struct packed {
      logic [15:0] q;
    } act_x5;
  } dla_reg2hw_ppe_act_x2_reg_t;

  typedef struct packed {
    struct packed {
      logic [15:0] q;
    } act_x6;
    struct packed {
      logic [15:0] q;
    } act_x7;
  } dla_reg2hw_ppe_act_x3_reg_t;

  typedef struct packed {
    struct packed {
      logic [15:0] q;
    } act_x8;
    struct packed {
      logic [15:0] q;
    } act_x9;
  } dla_reg2hw_ppe_act_x4_reg_t;

  typedef struct packed {
    struct packed {
      logic [15:0] q;
    } act_x10;
    struct packed {
      logic [15:0] q;
    } act_x11;
  } dla_reg2hw_ppe_act_x5_reg_t;

  typedef struct packed {
    struct packed {
      logic [15:0] q;
    } act_x12;
    struct packed {
      logic [15:0] q;
    } act_x13;
  } dla_reg2hw_ppe_act_x6_reg_t;

  typedef struct packed {
    logic [15:0] q;
  } dla_reg2hw_ppe_act_x7_reg_t;

  typedef struct packed {
    logic [31:0] q;
  } dla_reg2hw_ddr_mosi_u_reg_t;

  typedef struct packed {
    logic [31:0] q;
  } dla_reg2hw_ddr_mosi_l_reg_t;

  typedef struct packed {
    logic        q;
    logic        qe;
  } dla_reg2hw_ddr_mosi_valid_reg_t;

  typedef struct packed {
    logic        q;
  } dla_reg2hw_cpu_rd_reg_t;

  typedef struct packed {
    logic        q;
  } dla_reg2hw_cpu_access_ddr_reg_t;


  typedef struct packed {
    logic        d;
    logic        de;
  } dla_hw2reg_intr_state_reg_t;

  typedef struct packed {
    struct packed {
      logic        d;
      logic        de;
    } ddr2gb;
    struct packed {
      logic        d;
      logic        de;
    } gb2lb;
    struct packed {
      logic        d;
      logic        de;
    } conv;
    struct packed {
      logic        d;
      logic        de;
    } fc;
    struct packed {
      logic        d;
      logic        de;
    } ppe;
  } dla_hw2reg_gst_status_reg_t;

  typedef struct packed {
    struct packed {
      logic        d;
      logic        de;
    } ddr2gb;
    struct packed {
      logic        d;
      logic        de;
    } gb2lb;
    struct packed {
      logic        d;
      logic        de;
    } conv;
    struct packed {
      logic        d;
      logic        de;
    } fc;
    struct packed {
      logic        d;
      logic        de;
    } ppe;
  } dla_hw2reg_gst_intr_reg_t;

  typedef struct packed {
    logic        d;
    logic        de;
  } dla_hw2reg_init_calib_complete_reg_t;

  typedef struct packed {
    logic        d;
    logic        de;
  } dla_hw2reg_ddr_miso_valid_reg_t;

  typedef struct packed {
    struct packed {
      logic [15:0] d;
      logic        de;
    } wptr;
    struct packed {
      logic [15:0] d;
      logic        de;
    } rptr;
  } dla_hw2reg_rxf_ctrl_reg_t;


  ///////////////////////////////////////
  // Register to internal design logic //
  ///////////////////////////////////////
  typedef struct packed {
    dla_reg2hw_intr_state_reg_t intr_state; // [1254:1254]
    dla_reg2hw_intr_enable_reg_t intr_enable; // [1253:1253]
    dla_reg2hw_intr_test_reg_t intr_test; // [1252:1251]
    dla_reg2hw_gst_intr_reg_t gst_intr; // [1250:1246]
    dla_reg2hw_gst_enable_row_reg_t gst_enable_row; // [1245:1230]
    dla_reg2hw_gst_enable_col_reg_t gst_enable_col; // [1229:1214]
    dla_reg2hw_gst_comp_state_reg_t gst_comp_state; // [1213:1212]
    dla_reg2hw_ddr2gb_ctrl_reg_t ddr2gb_ctrl; // [1211:1208]
    dla_reg2hw_ddr2gb_ddr_addr0_reg_t ddr2gb_ddr_addr0; // [1207:1176]
    dla_reg2hw_ddr2gb_ddr_addr1_reg_t ddr2gb_ddr_addr1; // [1175:1154]
    dla_reg2hw_ddr2gb_gb_addr_reg_t ddr2gb_gb_addr; // [1153:1126]
    dla_reg2hw_gb2lb_ctrl_reg_t gb2lb_ctrl; // [1125:1122]
    dla_reg2hw_gb2lb_addr_reg_t gb2lb_addr; // [1121:1102]
    dla_reg2hw_gb2lb_src0_reg_t gb2lb_src0; // [1101:1085]
    dla_reg2hw_gb2lb_src1_reg_t gb2lb_src1; // [1084:1063]
    dla_reg2hw_comp_ctrl_reg_t comp_ctrl; // [1062:1053]
    dla_reg2hw_comp_k_size_reg_t comp_k_size; // [1052:1044]
    dla_reg2hw_comp_f_size_reg_t comp_f_size; // [1043:1028]
    dla_reg2hw_comp_c_size_reg_t comp_c_size; // [1027:1012]
    dla_reg2hw_comp_p_size_reg_t comp_p_size; // [1011:980]
    dla_reg2hw_comp_fbload_reg_t comp_fbload; // [979:964]
    dla_reg2hw_comp_addr_reg_t comp_addr; // [963:938]
    dla_reg2hw_ppe_ctrl_reg_t ppe_ctrl; // [937:909]
    dla_reg2hw_ppe_fbuf_addr_reg_t ppe_fbuf_addr; // [908:887]
    dla_reg2hw_ppe_abuf_addr_reg_t ppe_abuf_addr; // [886:877]
    dla_reg2hw_ppe_skip_reg_t ppe_skip; // [876:855]
    dla_reg2hw_ppe_dila_reg_t ppe_dila; // [854:833]
    dla_reg2hw_ppe_size_reg_t ppe_size; // [832:820]
    dla_reg2hw_ppe_act_k0_reg_t ppe_act_k0; // [819:788]
    dla_reg2hw_ppe_act_k1_reg_t ppe_act_k1; // [787:756]
    dla_reg2hw_ppe_act_k2_reg_t ppe_act_k2; // [755:724]
    dla_reg2hw_ppe_act_k3_reg_t ppe_act_k3; // [723:692]
    dla_reg2hw_ppe_act_k4_reg_t ppe_act_k4; // [691:660]
    dla_reg2hw_ppe_act_k5_reg_t ppe_act_k5; // [659:628]
    dla_reg2hw_ppe_act_k6_reg_t ppe_act_k6; // [627:596]
    dla_reg2hw_ppe_act_k7_reg_t ppe_act_k7; // [595:564]
    dla_reg2hw_ppe_act_b0_reg_t ppe_act_b0; // [563:532]
    dla_reg2hw_ppe_act_b1_reg_t ppe_act_b1; // [531:500]
    dla_reg2hw_ppe_act_b2_reg_t ppe_act_b2; // [499:468]
    dla_reg2hw_ppe_act_b3_reg_t ppe_act_b3; // [467:436]
    dla_reg2hw_ppe_act_b4_reg_t ppe_act_b4; // [435:404]
    dla_reg2hw_ppe_act_b5_reg_t ppe_act_b5; // [403:372]
    dla_reg2hw_ppe_act_b6_reg_t ppe_act_b6; // [371:340]
    dla_reg2hw_ppe_act_b7_reg_t ppe_act_b7; // [339:308]
    dla_reg2hw_ppe_act_x0_reg_t ppe_act_x0; // [307:276]
    dla_reg2hw_ppe_act_x1_reg_t ppe_act_x1; // [275:244]
    dla_reg2hw_ppe_act_x2_reg_t ppe_act_x2; // [243:212]
    dla_reg2hw_ppe_act_x3_reg_t ppe_act_x3; // [211:180]
    dla_reg2hw_ppe_act_x4_reg_t ppe_act_x4; // [179:148]
    dla_reg2hw_ppe_act_x5_reg_t ppe_act_x5; // [147:116]
    dla_reg2hw_ppe_act_x6_reg_t ppe_act_x6; // [115:84]
    dla_reg2hw_ppe_act_x7_reg_t ppe_act_x7; // [83:68]
    dla_reg2hw_ddr_mosi_u_reg_t ddr_mosi_u; // [67:36]
    dla_reg2hw_ddr_mosi_l_reg_t ddr_mosi_l; // [35:4]
    dla_reg2hw_ddr_mosi_valid_reg_t ddr_mosi_valid; // [3:2]
    dla_reg2hw_cpu_rd_reg_t cpu_rd; // [1:1]
    dla_reg2hw_cpu_access_ddr_reg_t cpu_access_ddr; // [0:0]
  } dla_reg2hw_t;

  ///////////////////////////////////////
  // Internal design logic to register //
  ///////////////////////////////////////
  typedef struct packed {
    dla_hw2reg_intr_state_reg_t intr_state; // [59:58]
    dla_hw2reg_gst_status_reg_t gst_status; // [57:48]
    dla_hw2reg_gst_intr_reg_t gst_intr; // [47:38]
    dla_hw2reg_init_calib_complete_reg_t init_calib_complete; // [37:36]
    dla_hw2reg_ddr_miso_valid_reg_t ddr_miso_valid; // [35:34]
    dla_hw2reg_rxf_ctrl_reg_t rxf_ctrl; // [33:0]
  } dla_hw2reg_t;

  // Register Address
  parameter logic [15:0] DLA_INTR_STATE_OFFSET = 16'h 0;
  parameter logic [15:0] DLA_INTR_ENABLE_OFFSET = 16'h 4;
  parameter logic [15:0] DLA_INTR_TEST_OFFSET = 16'h 8;
  parameter logic [15:0] DLA_GST_STATUS_OFFSET = 16'h c;
  parameter logic [15:0] DLA_GST_INTR_OFFSET = 16'h 10;
  parameter logic [15:0] DLA_GST_ENABLE_ROW_OFFSET = 16'h 14;
  parameter logic [15:0] DLA_GST_ENABLE_COL_OFFSET = 16'h 18;
  parameter logic [15:0] DLA_GST_COMP_STATE_OFFSET = 16'h 1c;
  parameter logic [15:0] DLA_DDR2GB_CTRL_OFFSET = 16'h 20;
  parameter logic [15:0] DLA_DDR2GB_DDR_ADDR0_OFFSET = 16'h 24;
  parameter logic [15:0] DLA_DDR2GB_DDR_ADDR1_OFFSET = 16'h 28;
  parameter logic [15:0] DLA_DDR2GB_GB_ADDR_OFFSET = 16'h 2c;
  parameter logic [15:0] DLA_GB2LB_CTRL_OFFSET = 16'h 30;
  parameter logic [15:0] DLA_GB2LB_ADDR_OFFSET = 16'h 34;
  parameter logic [15:0] DLA_GB2LB_SRC0_OFFSET = 16'h 38;
  parameter logic [15:0] DLA_GB2LB_SRC1_OFFSET = 16'h 3c;
  parameter logic [15:0] DLA_COMP_CTRL_OFFSET = 16'h 40;
  parameter logic [15:0] DLA_COMP_K_SIZE_OFFSET = 16'h 44;
  parameter logic [15:0] DLA_COMP_F_SIZE_OFFSET = 16'h 48;
  parameter logic [15:0] DLA_COMP_C_SIZE_OFFSET = 16'h 4c;
  parameter logic [15:0] DLA_COMP_P_SIZE_OFFSET = 16'h 50;
  parameter logic [15:0] DLA_COMP_FBLOAD_OFFSET = 16'h 54;
  parameter logic [15:0] DLA_COMP_ADDR_OFFSET = 16'h 58;
  parameter logic [15:0] DLA_PPE_CTRL_OFFSET = 16'h 5c;
  parameter logic [15:0] DLA_PPE_FBUF_ADDR_OFFSET = 16'h 60;
  parameter logic [15:0] DLA_PPE_ABUF_ADDR_OFFSET = 16'h 64;
  parameter logic [15:0] DLA_PPE_SKIP_OFFSET = 16'h 68;
  parameter logic [15:0] DLA_PPE_DILA_OFFSET = 16'h 6c;
  parameter logic [15:0] DLA_PPE_SIZE_OFFSET = 16'h 70;
  parameter logic [15:0] DLA_PPE_ACT_K0_OFFSET = 16'h 74;
  parameter logic [15:0] DLA_PPE_ACT_K1_OFFSET = 16'h 78;
  parameter logic [15:0] DLA_PPE_ACT_K2_OFFSET = 16'h 7c;
  parameter logic [15:0] DLA_PPE_ACT_K3_OFFSET = 16'h 80;
  parameter logic [15:0] DLA_PPE_ACT_K4_OFFSET = 16'h 84;
  parameter logic [15:0] DLA_PPE_ACT_K5_OFFSET = 16'h 88;
  parameter logic [15:0] DLA_PPE_ACT_K6_OFFSET = 16'h 8c;
  parameter logic [15:0] DLA_PPE_ACT_K7_OFFSET = 16'h 90;
  parameter logic [15:0] DLA_PPE_ACT_B0_OFFSET = 16'h 94;
  parameter logic [15:0] DLA_PPE_ACT_B1_OFFSET = 16'h 98;
  parameter logic [15:0] DLA_PPE_ACT_B2_OFFSET = 16'h 9c;
  parameter logic [15:0] DLA_PPE_ACT_B3_OFFSET = 16'h a0;
  parameter logic [15:0] DLA_PPE_ACT_B4_OFFSET = 16'h a4;
  parameter logic [15:0] DLA_PPE_ACT_B5_OFFSET = 16'h a8;
  parameter logic [15:0] DLA_PPE_ACT_B6_OFFSET = 16'h ac;
  parameter logic [15:0] DLA_PPE_ACT_B7_OFFSET = 16'h b0;
  parameter logic [15:0] DLA_PPE_ACT_X0_OFFSET = 16'h b4;
  parameter logic [15:0] DLA_PPE_ACT_X1_OFFSET = 16'h b8;
  parameter logic [15:0] DLA_PPE_ACT_X2_OFFSET = 16'h bc;
  parameter logic [15:0] DLA_PPE_ACT_X3_OFFSET = 16'h c0;
  parameter logic [15:0] DLA_PPE_ACT_X4_OFFSET = 16'h c4;
  parameter logic [15:0] DLA_PPE_ACT_X5_OFFSET = 16'h c8;
  parameter logic [15:0] DLA_PPE_ACT_X6_OFFSET = 16'h cc;
  parameter logic [15:0] DLA_PPE_ACT_X7_OFFSET = 16'h d0;
  parameter logic [15:0] DLA_INIT_CALIB_COMPLETE_OFFSET = 16'h d4;
  parameter logic [15:0] DLA_DDR_MOSI_U_OFFSET = 16'h d8;
  parameter logic [15:0] DLA_DDR_MOSI_L_OFFSET = 16'h dc;
  parameter logic [15:0] DLA_DDR_MOSI_VALID_OFFSET = 16'h e0;
  parameter logic [15:0] DLA_DDR_MISO_VALID_OFFSET = 16'h e4;
  parameter logic [15:0] DLA_RXF_CTRL_OFFSET = 16'h e8;
  parameter logic [15:0] DLA_CPU_RD_OFFSET = 16'h ec;
  parameter logic [15:0] DLA_CPU_ACCESS_DDR_OFFSET = 16'h f0;

  // Window parameter
  parameter logic [15:0] DLA_DMEM_OFFSET = 16'h 8000;
  parameter logic [15:0] DLA_DMEM_SIZE   = 16'h 1000;

  // Register Index
  typedef enum int {
    DLA_INTR_STATE,
    DLA_INTR_ENABLE,
    DLA_INTR_TEST,
    DLA_GST_STATUS,
    DLA_GST_INTR,
    DLA_GST_ENABLE_ROW,
    DLA_GST_ENABLE_COL,
    DLA_GST_COMP_STATE,
    DLA_DDR2GB_CTRL,
    DLA_DDR2GB_DDR_ADDR0,
    DLA_DDR2GB_DDR_ADDR1,
    DLA_DDR2GB_GB_ADDR,
    DLA_GB2LB_CTRL,
    DLA_GB2LB_ADDR,
    DLA_GB2LB_SRC0,
    DLA_GB2LB_SRC1,
    DLA_COMP_CTRL,
    DLA_COMP_K_SIZE,
    DLA_COMP_F_SIZE,
    DLA_COMP_C_SIZE,
    DLA_COMP_P_SIZE,
    DLA_COMP_FBLOAD,
    DLA_COMP_ADDR,
    DLA_PPE_CTRL,
    DLA_PPE_FBUF_ADDR,
    DLA_PPE_ABUF_ADDR,
    DLA_PPE_SKIP,
    DLA_PPE_DILA,
    DLA_PPE_SIZE,
    DLA_PPE_ACT_K0,
    DLA_PPE_ACT_K1,
    DLA_PPE_ACT_K2,
    DLA_PPE_ACT_K3,
    DLA_PPE_ACT_K4,
    DLA_PPE_ACT_K5,
    DLA_PPE_ACT_K6,
    DLA_PPE_ACT_K7,
    DLA_PPE_ACT_B0,
    DLA_PPE_ACT_B1,
    DLA_PPE_ACT_B2,
    DLA_PPE_ACT_B3,
    DLA_PPE_ACT_B4,
    DLA_PPE_ACT_B5,
    DLA_PPE_ACT_B6,
    DLA_PPE_ACT_B7,
    DLA_PPE_ACT_X0,
    DLA_PPE_ACT_X1,
    DLA_PPE_ACT_X2,
    DLA_PPE_ACT_X3,
    DLA_PPE_ACT_X4,
    DLA_PPE_ACT_X5,
    DLA_PPE_ACT_X6,
    DLA_PPE_ACT_X7,
    DLA_INIT_CALIB_COMPLETE,
    DLA_DDR_MOSI_U,
    DLA_DDR_MOSI_L,
    DLA_DDR_MOSI_VALID,
    DLA_DDR_MISO_VALID,
    DLA_RXF_CTRL,
    DLA_CPU_RD,
    DLA_CPU_ACCESS_DDR
  } dla_id_e;

  // Register width information to check illegal writes
  parameter logic [3:0] DLA_PERMIT [61] = '{
    4'b 0001, // index[ 0] DLA_INTR_STATE
    4'b 0001, // index[ 1] DLA_INTR_ENABLE
    4'b 0001, // index[ 2] DLA_INTR_TEST
    4'b 0001, // index[ 3] DLA_GST_STATUS
    4'b 0001, // index[ 4] DLA_GST_INTR
    4'b 0011, // index[ 5] DLA_GST_ENABLE_ROW
    4'b 0011, // index[ 6] DLA_GST_ENABLE_COL
    4'b 0001, // index[ 7] DLA_GST_COMP_STATE
    4'b 1111, // index[ 8] DLA_DDR2GB_CTRL
    4'b 1111, // index[ 9] DLA_DDR2GB_DDR_ADDR0
    4'b 0111, // index[10] DLA_DDR2GB_DDR_ADDR1
    4'b 1111, // index[11] DLA_DDR2GB_GB_ADDR
    4'b 1111, // index[12] DLA_GB2LB_CTRL
    4'b 1111, // index[13] DLA_GB2LB_ADDR
    4'b 0111, // index[14] DLA_GB2LB_SRC0
    4'b 1111, // index[15] DLA_GB2LB_SRC1
    4'b 1111, // index[16] DLA_COMP_CTRL
    4'b 0111, // index[17] DLA_COMP_K_SIZE
    4'b 0111, // index[18] DLA_COMP_F_SIZE
    4'b 0111, // index[19] DLA_COMP_C_SIZE
    4'b 1111, // index[20] DLA_COMP_P_SIZE
    4'b 0011, // index[21] DLA_COMP_FBLOAD
    4'b 1111, // index[22] DLA_COMP_ADDR
    4'b 1111, // index[23] DLA_PPE_CTRL
    4'b 1111, // index[24] DLA_PPE_FBUF_ADDR
    4'b 0011, // index[25] DLA_PPE_ABUF_ADDR
    4'b 1111, // index[26] DLA_PPE_SKIP
    4'b 1111, // index[27] DLA_PPE_DILA
    4'b 0111, // index[28] DLA_PPE_SIZE
    4'b 1111, // index[29] DLA_PPE_ACT_K0
    4'b 1111, // index[30] DLA_PPE_ACT_K1
    4'b 1111, // index[31] DLA_PPE_ACT_K2
    4'b 1111, // index[32] DLA_PPE_ACT_K3
    4'b 1111, // index[33] DLA_PPE_ACT_K4
    4'b 1111, // index[34] DLA_PPE_ACT_K5
    4'b 1111, // index[35] DLA_PPE_ACT_K6
    4'b 1111, // index[36] DLA_PPE_ACT_K7
    4'b 1111, // index[37] DLA_PPE_ACT_B0
    4'b 1111, // index[38] DLA_PPE_ACT_B1
    4'b 1111, // index[39] DLA_PPE_ACT_B2
    4'b 1111, // index[40] DLA_PPE_ACT_B3
    4'b 1111, // index[41] DLA_PPE_ACT_B4
    4'b 1111, // index[42] DLA_PPE_ACT_B5
    4'b 1111, // index[43] DLA_PPE_ACT_B6
    4'b 1111, // index[44] DLA_PPE_ACT_B7
    4'b 1111, // index[45] DLA_PPE_ACT_X0
    4'b 1111, // index[46] DLA_PPE_ACT_X1
    4'b 1111, // index[47] DLA_PPE_ACT_X2
    4'b 1111, // index[48] DLA_PPE_ACT_X3
    4'b 1111, // index[49] DLA_PPE_ACT_X4
    4'b 1111, // index[50] DLA_PPE_ACT_X5
    4'b 1111, // index[51] DLA_PPE_ACT_X6
    4'b 0011, // index[52] DLA_PPE_ACT_X7
    4'b 0001, // index[53] DLA_INIT_CALIB_COMPLETE
    4'b 1111, // index[54] DLA_DDR_MOSI_U
    4'b 1111, // index[55] DLA_DDR_MOSI_L
    4'b 0001, // index[56] DLA_DDR_MOSI_VALID
    4'b 0001, // index[57] DLA_DDR_MISO_VALID
    4'b 1111, // index[58] DLA_RXF_CTRL
    4'b 0001, // index[59] DLA_CPU_RD
    4'b 0001  // index[60] DLA_CPU_ACCESS_DDR
  };
endpackage

