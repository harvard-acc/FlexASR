/*
 * All rights reserved - Harvard University. 
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the "License"); 
 * you may not use this file except in compliance with the License.  
 * You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef ACTUNIT_H
#define ACTUNIT_H

#include <systemc.h>
#include <nvhls_int.h>
#include <nvhls_types.h>
#include <nvhls_vector.h>
#include <nvhls_module.h>

#include <ac_int.h>
#include <ac_fixed.h>
#include <ac_math.h>
#include <ac_math/ac_sigmoid_pwl.h>
#include <ac_math/ac_tanh_pwl.h>
#include <ArbitratedScratchpadDP.h>

#include "SM6Spec.h"
#include "AxiSpec.h"
#include "AdpfloatSpec.h"
#include "AdpfloatUtils.h"
#include "ActUnitSpec.h"

#include "PPU/PPU.h"

// Use terminology OP A2 A1
class ActUnit : public match::Module {
  static const int kDebugLevel = 4;
  SC_HAS_PROCESS(ActUnit);
 public:
  Connections::In<bool> start;  
  Connections::In<spec::ActVectorType> act_port;
  Connections::In<spec::Axi::SlaveToRVA::Write> rva_in;


  
  Connections::Out<spec::Axi::SlaveToRVA::Read> rva_out;
  Connections::Out<spec::StreamType> output_port;
  Connections::Out<bool> done;
  
 protected:
  // Internal states
  spec::ActVectorType act_regs[spec::kNumActEntries];       //  XXX: cannot be configured anymore
  
  ArbitratedScratchpadDP<spec::Act::kNumBanks,      // 1
                         spec::Act::kNumReadPorts,  // 1
                         spec::Act::kNumWritePorts, // 1
                         spec::Act::kEntriesPerBank, 
                         spec::Act::WordType, 
                         false, true> act_mem;
  ActConfig act_config;
  bool is_start;
  
  
  
 public:
  // Constructor
  ActUnit (sc_module_name nm)
      : match::Module(nm),
        start("start"),
        act_port("act_port"),
        rva_in("rva_in"),
        rva_out("rva_out"),
        output_port("output_port"),
        done("done")
  {
    SC_THREAD(ActUnitRun);
    sensitive << clk.pos();
    async_reset_signal_is(rst, false);
  }
  
  // define SRAM I/O     
  spec::Act::Address  act_read_addrs          [spec::Act::kNumReadPorts]; 
  bool                act_read_req_valid      [spec::Act::kNumReadPorts];     
  spec::Act::Address  act_write_addrs         [spec::Act::kNumWritePorts];
  bool                act_write_req_valid     [spec::Act::kNumWritePorts];
  spec::Act::WordType act_write_data          [spec::Act::kNumWritePorts];
  bool                act_read_ack            [spec::Act::kNumReadPorts]; 
  bool                act_write_ack           [spec::Act::kNumWritePorts];
  bool                act_read_ready          [spec::Act::kNumReadPorts];
  spec::Act::WordType act_port_read_out       [spec::Act::kNumReadPorts];
  bool                act_port_read_out_valid [spec::Act::kNumReadPorts];       
  
  // while loop internal states
//  bool w_axi_req, w_axi_rsp, w_out, w_load, w_done;
  bool w_axi_rsp, w_out, w_load, w_done;      
  bool is_incr;
  spec::Axi::SlaveToRVA::Read rva_out_reg;  
  //NVUINT8 curr_inst;
  
  //******* Reset Families
  void Reset() {
    act_config.Reset();
    ResetPorts();
    ResetActRegs();
    is_start = 0;
  }
 
  void ResetPorts() {
    start.Reset();
    act_port.Reset();
    rva_in.Reset();
    rva_out.Reset();
    output_port.Reset();
    done.Reset();
  }
  
  void ResetActRegs(){
    #pragma hls_unroll yes 
    for (int i = 0; i < spec::kNumActEntries; i++) {
      act_regs[i] = 0;
    }
  }

  //****** End Reset Families 
  /*** change Act Config format ***/
  void DecodeAxiRead(const spec::Axi::SlaveToRVA::Write& rva_in_reg) {
    NVUINT4 tmp = nvhls::get_slc<4>(rva_in_reg.addr, 20);
    rva_out_reg.data = 0;
    if (tmp == 0x8) {         // Act Config
      NVUINT8 local_index = nvhls::get_slc<8>(rva_in_reg.addr, 4);
      act_config.ActConfigRead(local_index, rva_out_reg.data);
      //cout << rva_out_reg.data << endl;
    }
    else if (tmp == 0x9) {    // Act buffer
      NVUINT16 local_index = nvhls::get_slc<16>(rva_in_reg.addr, 4);
      act_read_ready[0] = 1; 
      act_read_addrs[0] = local_index;
      act_read_req_valid[0] = 1;
    }
  }
  
  void DecodeAxiWrite(const spec::Axi::SlaveToRVA::Write& rva_in_reg){
    NVUINT4 tmp = nvhls::get_slc<4>(rva_in_reg.addr, 20);
    //NVUINT16    local_index = nvhls::get_slc<16>(rva_in_reg.addr, 4);
        
    if (tmp == 0x8) {         // Act Config
      NVUINT8 local_index = nvhls::get_slc<8>(rva_in_reg.addr, 4);
      act_config.ActConfigWrite(local_index, rva_in_reg.data);
    }
    else if (tmp == 0x9) {    // Act buffer
      NVUINT16 local_index = nvhls::get_slc<16>(rva_in_reg.addr, 4);
      act_write_addrs[0] = local_index;
      act_write_req_valid[0] = 1;
      act_write_data[0] = rva_in_reg.data;
    }
    //else if (tmp == 0xF) {
    //  if (local_index == 0xFFFF) {
    //    Reset();
    //  }
    //}  
    
  }
  
  void ResetBufferInputs(){ 
    act_read_addrs          [0] = 0; 
    act_read_req_valid      [0] = 0;     
    act_write_addrs         [0] = 0;
    act_write_req_valid     [0] = 0;
    act_write_data          [0] = 0;
    act_read_ready          [0] = 0;
  }
  
  void Initialize() {
    ResetBufferInputs();
    w_axi_rsp = 0;
    w_out = 0;
    w_load = 0;
    w_done = 0;
    is_incr = 1;
  }  
  
  void CheckStart() {
    bool start_reg;
    if (start.PopNB(start_reg)) {
      CDCOUT(sc_time_stamp()  << " ActUnit: " << name() << " Start" << endl, kDebugLevel);
      is_start = act_config.is_valid && start_reg;
    }
  }
  
  // Axi RVA address is 24 bits long for ActUnit
  // (note that slave to RVA should be able to different address for PECore and address for Act)
  void DecodeAxi() {
    spec::Axi::SlaveToRVA::Write rva_in_reg;
    if (rva_in.PopNB(rva_in_reg)) {
      CDCOUT(sc_time_stamp()  << " Act: " << name() << "RVA Pop " << endl, kDebugLevel);
      if(rva_in_reg.rw) {
        DecodeAxiWrite(rva_in_reg);
      }
      else {
        w_axi_rsp = 1;
        DecodeAxiRead(rva_in_reg);
      }
    }
  }
  
  // TODO: Might implement Formal RF architecture in later updates
  
  
  void RunInst(ActConfig act_config_in) {
    // lock if recieve AXI request or the ActUnit is not started 
    NVUINT8 curr_inst = act_config_in.InstFetch();
    NVUINT4 op = nvhls::get_slc<4>(curr_inst, 4);
    NVUINT2 a2 = nvhls::get_slc<2>(curr_inst, 2);
    NVUINT2 a1 = nvhls::get_slc<2>(curr_inst, 0);
      
    switch (op) {
      case 0x1: { // LOAD SRAM -> A2 FIXME: load address is determined by the output_counter + buffer_addr_base
        w_load = 1;
        if (act_config_in.is_zero_first == 0) {
          act_read_ready[0] = 1;
          act_read_addrs[0] = act_config_in.output_counter + act_config_in.buffer_addr_base;
          act_read_req_valid[0] = 1;
        }
        break;
      }
      case 0x2: { // STORE SRAM <- A2 FIXME: Store address is determined by the output_counter + buffer_addr_base
        act_write_addrs[0] = act_config_in.output_counter + act_config_in.buffer_addr_base;
        act_write_req_valid[0] = 1;
        Fixed2Adpfloat(act_regs[a2], act_write_data[0], act_config_in.adpfloat_bias);
        break;
      }
      case 0x3: { // INPE act_port -> A2 FIXME: Do not increment instruction if nothing recieved 
        spec::ActVectorType act_port_reg;
        if (act_port.PopNB(act_port_reg)) {   
          act_regs[a2] = act_port_reg;
        }
        else {
          is_incr = 0; // Stall instruction if not recieve act data
        }
        break;
      }
      case 0x4: { // OUTGB A2 -> Output
        w_out = 1;
        break;
      }
      case 0x7: { // COPY A1 -> A2
        act_regs[a2] = act_regs[a1];
        break;      
      } 
      case 0x8: { // EADD
        EAdd(act_regs[a1], act_regs[a2], act_regs[a2]); 
        break;
      }
      case 0x9: { // EMUL
        EMul(act_regs[a1], act_regs[a2], act_regs[a2]); 
        break;
      }        
      case 0xA: { // SIGM Sigm(A2) -> A2 
        Sigmoid(act_regs[a2], act_regs[a2]);
        break;
      }
      case 0xB: { // TANH
        Tanh(act_regs[a2], act_regs[a2]);
        break;
      }
      case 0xC: { // RELU
        Relu(act_regs[a2], act_regs[a2]);
        break;
      }
      case 0xD: { // ONEX
        OneX(act_regs[a2], act_regs[a2]);
        break;
      }
      default: {
        break;
      }
    }
  }
  
  void BufferAccress() {
    act_mem.run(
      act_read_addrs          , 
      act_read_req_valid      ,     
      act_write_addrs         ,
      act_write_req_valid     ,
      act_write_data          ,
      act_read_ack            ,
      act_write_ack           ,
      act_read_ready          ,
      act_port_read_out       ,
      act_port_read_out_valid 
    );    
  }
  
  
  void RunLoad(ActConfig act_config_in) {
    if (w_load) {  // need to move SRAM data to actreg
      NVUINT8 curr_inst = act_config_in.InstFetch();
      // need write zero function to preform skipping 
      NVUINT2 a2 = nvhls::get_slc<2>(curr_inst, 2);  
      // Write Zero instead if is_zero first is set    
      if (act_config_in.is_zero_first == 1) {
        act_regs[a2] = 0;
      }
      // or convert from Adpfloat to fixed point and load to reg
      else {
        Adpfloat2Fixed(act_port_read_out[0], act_regs[a2], act_config_in.adpfloat_bias);
      }
    }
  }
  
  void PushOutput(ActConfig act_config_in) {
    // output port
    if (w_out) {
      spec::StreamType output_port_reg;
      NVUINT8 curr_inst = act_config_in.InstFetch();
      NVUINT2 a2 = nvhls::get_slc<2>(curr_inst, 2);     
      //NVUINT2 a1 = nvhls::get_slc<2>(curr_inst, 0);         
      // fix2float
      Fixed2Adpfloat(act_regs[a2], output_port_reg.data, act_config_in.adpfloat_bias);
      
      // push output 
      // 0322 this follows the format of GB
      // GB does not need index for PE Output
      output_port_reg.index = 0; 
      output_port_reg.logical_addr = act_config_in.output_counter + act_config_in.output_addr_base;
      
      output_port.Push(output_port_reg);
    }
  }
  
  void PushAxiRsp() {
    // use out valid to check for axi read on sram
    if (w_axi_rsp) {
      // indicates the read req is for SRAM
      // else already set by axi read config
      if (act_port_read_out_valid[0]) {  
        rva_out_reg.data = act_port_read_out[0].to_rawbits();     //  conversion from vector to plain bit seq
      }
      rva_out.Push(rva_out_reg);
    }  
  }

  void ActUnitRun() {
    Reset();
  
    while(1) {
      Initialize();
      if (is_start == 0) {
        DecodeAxi();
      }
      else {
        RunInst(act_config);
      }
      
      BufferAccress();
      
      if (is_start == 0) {
        PushAxiRsp(); 
        CheckStart();    
      }
      else {
        PushOutput(act_config);      
        RunLoad(act_config);
        if (is_incr) {
          bool is_end;
          is_end = act_config.InstIncr();  
          CDCOUT(sc_time_stamp()  << " ActUnit: " << name() << " is_end signal: " << is_end << endl, kDebugLevel);
           
          // End condition (num_output iterations on instruction is done)
          if (is_end == 1) {
            w_done = 1;     // Push done signal
            done.Push(1);
            is_start = 0;   // Stop ActUnit in next while iter
          }
        }
      }
      
      wait();
    }
  }
  
};


 
#endif
