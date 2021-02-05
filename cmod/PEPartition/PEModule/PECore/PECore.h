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

#ifndef PECORE_H
#define PECORE_H

#include <systemc.h>
#include <nvhls_int.h>
#include <nvhls_types.h>
#include <nvhls_vector.h>
#include <nvhls_module.h>
#include <ArbitratedScratchpadDP.h>

#include "SM6Spec.h"
#include "AxiSpec.h"
#include "PECoreSpec.h"
#include "Datapath/Datapath.h"

class PECore : public match::Module {
  static const int kDebugLevel = 4;
  SC_HAS_PROCESS(PECore);
 public:
  Connections::In<bool> start;
  Connections::In<spec::StreamType> input_port;
  Connections::In<spec::Axi::SlaveToRVA::Write> rva_in;
  Connections::Out<spec::Axi::SlaveToRVA::Read> rva_out;
  Connections::Out<spec::ActVectorType> act_port;
  sc_in <NVUINT32>  SC_SRAM_CONFIG;
  
  // single port weight SRAM
  ArbitratedScratchpadDP<spec::PE::Weight::kNumBanks,      
                         spec::PE::Weight::kNumReadPorts,
                         spec::PE::Weight::kNumWritePorts,
                         spec::PE::Weight::kEntriesPerBank, 
                         spec::PE::Weight::WordType, 
                         false, true> weight_mem;        
                       
  // single port input SRAM
  ArbitratedScratchpadDP<spec::PE::Input::kNumBanks,
                         spec::PE::Input::kNumReadPorts,
                         spec::PE::Input::kNumWritePorts,
                         spec::PE::Input::kEntriesPerBank, 
                         spec::PE::Input::WordType, 
                         false, true> input_mem;            
  
  // Use weight address width as the address format of PEManager
  PEManager<spec::PE::Weight::kAddressWidth>              
              pe_manager[spec::PE::kNumPEManagers];

  PEConfig    pe_config;
  
  // FSM
  enum FSM {
    IDLE, PRE, MAC, BIAS, OUT
  };  
  FSM state;
  
  // accumulator regs
  spec::AccumVectorType accum_vector;   
  spec::ActVectorType act_port_reg;   
    
  // Indicate the Computation part is activated 
  bool is_start;
  
  
  // while loop control signal (including SRAM I/O)
  bool w_axi_rsp;
  spec::Axi::SlaveToRVA::Read rva_out_reg;  

  // SRAM buffer signals
  // Weight Buffer signals                           
  spec::PE::Weight::Address   weight_read_addrs           [spec::PE::Weight::kNumReadPorts];
  bool                        weight_read_req_valid       [spec::PE::Weight::kNumReadPorts]; 
  spec::PE::Weight::Address   weight_write_addrs          [spec::PE::Weight::kNumWritePorts];
  bool                        weight_write_req_valid      [spec::PE::Weight::kNumWritePorts];
  spec::PE::Weight::WordType  weight_write_data           [spec::PE::Weight::kNumWritePorts];
  bool                        weight_read_ack             [spec::PE::Weight::kNumReadPorts]; 
  bool                        weight_write_ack            [spec::PE::Weight::kNumWritePorts];
  bool                        weight_read_ready           [spec::PE::Weight::kNumReadPorts];
  spec::PE::Weight::WordType  weight_port_read_out        [spec::PE::Weight::kNumReadPorts];
  bool                        weight_port_read_out_valid  [spec::PE::Weight::kNumReadPorts];             

  // Input Buffer signal      
  spec::PE::Input::Address    input_read_addrs            [spec::PE::Input::kNumReadPorts]; 
  bool                        input_read_req_valid        [spec::PE::Input::kNumReadPorts];     
  spec::PE::Input::Address    input_write_addrs           [spec::PE::Input::kNumWritePorts];
  bool                        input_write_req_valid       [spec::PE::Input::kNumWritePorts];
  spec::PE::Input::WordType   input_write_data            [spec::PE::Input::kNumWritePorts];
  bool                        input_read_ack              [spec::PE::Input::kNumReadPorts]; 
  bool                        input_write_ack             [spec::PE::Input::kNumWritePorts];
  bool                        input_read_ready            [spec::PE::Input::kNumReadPorts];
  spec::PE::Input::WordType   input_port_read_out         [spec::PE::Input::kNumReadPorts];
  bool                        input_port_read_out_valid   [spec::PE::Input::kNumReadPorts];  
    
  // Constructor
  PECore (sc_module_name nm)
      : match::Module(nm),
        start("start"),
        input_port("input_port"),
        rva_in("rva_in"),
        rva_out("rva_out"),
        act_port("act_port"),
        SC_SRAM_CONFIG("SRAM_CONFIG")
  {
    SC_THREAD(PECoreRun);
    sensitive << clk.pos();
    async_reset_signal_is(rst, false);
  }

// Reset famalies ///////////
  void Reset() {
    state = IDLE;
    is_start = 0; // bug fix 
    for (unsigned i = 0; i < spec::PE::kNumPEManagers; i++) {
      pe_manager[i].Reset();
    }
    pe_config.Reset();
    ResetAccum();
    ResetPorts();
  }

  void ResetPorts() {
    start.Reset();
    input_port.Reset();
    rva_in.Reset();
    rva_out.Reset();
    act_port.Reset();
  }


  void ResetAccum() {
    accum_vector = 0;
    act_port_reg = 0;
  }
  
  void ResetBufferInputs() {
    #pragma hls_unroll yes 
    for (unsigned i = 0; i < spec::PE::Weight::kNumReadPorts; i++) { 
      weight_read_addrs          [i] = 0;
      weight_read_req_valid      [i] = 0;   
      weight_read_ready          [i] = 0;
    }
    weight_write_addrs         [0] = 0;
    weight_write_req_valid     [0] = 0;
    weight_write_data          [0] = 0;

    input_read_addrs          [0] = 0; 
    input_read_req_valid      [0] = 0;  
    input_read_ready          [0] = 0;  
    input_write_addrs         [0] = 0;
    input_write_req_valid     [0] = 0;
    input_write_data          [0] = 0;
  
  }
/////////////////////////////

  void DecodeAxiWrite(const spec::Axi::SlaveToRVA::Write& rva_in_reg){
    NVUINT4     tmp = nvhls::get_slc<4>(rva_in_reg.addr, 20);
    NVUINT16    local_index = nvhls::get_slc<16>(rva_in_reg.addr, 4);
    
    switch (tmp) {
      case 0x4: {     // PEconfig
        switch (local_index) {
          case 0x1: {
            pe_config.PEConfigWrite(rva_in_reg.data);
            break;        
          }
          case 0x2: {     // manager 0 config
            pe_manager[0].PEManagerWrite(rva_in_reg.data);
            break; 
          }
          case 0x3: {     // manager 0 Cluster
            pe_manager[0].ClusterWrite(rva_in_reg.data);
            break; 
          }    
          case 0x4: {     // manager 1 config
            pe_manager[1].PEManagerWrite(rva_in_reg.data);
            break; 
          }     
          case 0x5: {     // manager 1 cluster
            pe_manager[1].ClusterWrite(rva_in_reg.data);
            break; 
          }          
          default: {
            break;
          }
        }
        break;
      }
      case 0x5: {     // Weight Buffer
        weight_write_addrs         [0] = local_index;
        weight_write_req_valid     [0] = 1;
        weight_write_data          [0] = rva_in_reg.data; 
        break;
      }
      case 0x6: {     // Input Buffer (lock datapath)
        input_write_addrs         [0] = local_index;
        input_write_req_valid     [0] = 1;
        input_write_data          [0] = rva_in_reg.data;       
        break;
      }
      //case 0xE: {
      //  if (local_index == 0xFFFF) {
      //    Reset();
      //  }
      //  break;
      //}
      default: {
        break;
      }
    } 
  }   
  
  
  void DecodeAxiRead(const spec::Axi::SlaveToRVA::Write& rva_in_reg) {
    NVUINT4 tmp = nvhls::get_slc<4>(rva_in_reg.addr, 20);
    NVUINT16    local_index = nvhls::get_slc<16>(rva_in_reg.addr, 4);
        
    // Set Push Response
    w_axi_rsp = 1;
    rva_out_reg.data = 0;
    switch (tmp) {
      case 0x3: { // Write implemented inside PEModule
        rva_out_reg.data = SC_SRAM_CONFIG.read();
        break;
      }    
      case 0x4: {     // PEconfig
        switch (local_index) {
          case 0x1: {
            pe_config.PEConfigRead(rva_out_reg.data);
            break;
          }
          case 0x2: {     // manager 0 config
            pe_manager[0].PEManagerRead(rva_out_reg.data);
            break; 
          }
          case 0x3: {     // manager 0 Cluster
            pe_manager[0].ClusterRead(rva_out_reg.data);
            break; 
          }    
          case 0x4: {     // manager 1 config
            pe_manager[1].PEManagerRead(rva_out_reg.data);
          
            break; 
          }     
          case 0x5: {     // manager 1 cluster
            pe_manager[1].ClusterRead(rva_out_reg.data);
            break; 
          }
          default: {
            break;
          }
        }
        break;
      }
      case 0x5: {     // Weight Buffer
        //w_axir_weight = 1;  
        weight_read_addrs          [0] = local_index;
        weight_read_req_valid      [0] = 1;   
        weight_read_ready          [0] = 1;
        break;
      }
      case 0x6: {     // Input Buffer (lock datapath)
        //w_axir_input = 1;
        input_read_addrs          [0] = local_index; 
        input_read_req_valid      [0] = 1;  
        input_read_ready          [0] = 1;    
        break;
      }
      default: {
        break;
      }
    }
  }


// for clusting only, split one Scalar in to two half scalars
  // Cannot use const on input
  void VectorSplitting(spec::VectorType in, spec::HalfVectorType& out_0, spec::HalfVectorType& out_1) {
    out_0 = nvhls::get_slc<spec::HalfVectorType::width>(in.to_rawbits(), 0);
    out_1 = nvhls::get_slc<spec::HalfVectorType::width>(in.to_rawbits(), spec::HalfVectorType::width);
  }
  
  void Initialize() {
    ResetBufferInputs();
    w_axi_rsp     = 0;
  }
  
  void CheckStart() {
    bool start_reg;
    if (start.PopNB(start_reg)) {
      is_start = pe_config.is_valid && start_reg;
      CDCOUT(sc_time_stamp()  << " PECore: " << name() << " Start" << endl, kDebugLevel);
    }
  }
  
  
  void DecodeAxi() {  
    spec::Axi::SlaveToRVA::Write rva_in_reg;
    if (rva_in.PopNB(rva_in_reg)) {
      //w_axi_req = 1;
      CDCOUT(sc_time_stamp()  << " PECore: " << name() << "RVA Pop " << endl, kDebugLevel);
      if(rva_in_reg.rw) {
        DecodeAxiWrite(rva_in_reg);
      }
      else {
        DecodeAxiRead(rva_in_reg);
      }
    }  
  }

  void RunFSM() {
    // Can do FSM only when and no Axi on input
    // Can only move forward to computation if is_start = 1
    // Can only pop message from GB buffer in IDLE state

    switch (state) {
      case IDLE: {
        spec::StreamType input_port_reg; 
        if (input_port.PopNB(input_port_reg)) {
          NVUINT4   m_index = input_port_reg.index;
          input_write_addrs         [0] = pe_manager[m_index].GetInputAddr(input_port_reg.logical_addr);
          input_write_req_valid     [0] = 1;
          input_write_data          [0] = input_port_reg.data;              
        }
        break;
      }
      case PRE: {
        break;      
      }
      case MAC: {
        NVUINT4   m_index = pe_config.ManagerIndex();
        // Do MAC (Datapath)
        // set weight SRAM read
        spec::PE::Weight::Address weight_base;
        // clustering mode
        if (pe_config.is_cluster) {
          weight_base = pe_manager[m_index].GetWeightAddr(pe_config.InputIndex(), pe_config.OutputIndex(), 1);
          #pragma hls_unroll yes
          for (int i = 0; i < 8; i ++) {
            weight_read_addrs          [i] = weight_base + i;
            weight_read_req_valid      [i] = 1;   
            weight_read_ready          [i] = 1;              
          }     
        }
        // non-clustering mode 
        else {
          weight_base = pe_manager[m_index].GetWeightAddr(pe_config.InputIndex(), pe_config.OutputIndex(), 0);        
          #pragma hls_unroll yes
          for (int i = 0; i < 16; i ++) {
            weight_read_addrs          [i] = weight_base + i;
            weight_read_req_valid      [i] = 1;   
            weight_read_ready          [i] = 1;              
          }             
        }
        
        // set input SRAM read
        input_read_ready[0] = 1;
        input_read_addrs[0] = pe_manager[m_index].GetInputAddr(pe_config.InputIndex());
        input_read_req_valid[0] = 1;
        
        break;  
      }
       
      case BIAS: {
        NVUINT4   m_index = pe_config.ManagerIndex();
        // Set Bias SRAM read (on input SRAM)
        if (pe_config.is_bias) {
          input_read_ready[0] = 1;
          input_read_addrs[0] = pe_manager[m_index].GetBiasAddr(pe_config.OutputIndex());
          input_read_req_valid[0] = 1;        
        }
        break;
      }
      case OUT: {
        break;
      }
       
      default: {
        break;  
      }
    }
  }  
  
  void BufferAccress() {
    weight_mem.run(
      weight_read_addrs          , 
      weight_read_req_valid      ,     
      weight_write_addrs         ,
      weight_write_req_valid     ,
      weight_write_data          ,
      weight_read_ack            ,
      weight_write_ack           ,
      weight_read_ready          ,
      weight_port_read_out       ,
      weight_port_read_out_valid 
    );
    input_mem.run(
      input_read_addrs          , 
      input_read_req_valid      ,     
      input_write_addrs         ,
      input_write_req_valid     ,
      input_write_data          ,
      input_read_ack            ,
      input_write_ack           ,
      input_read_ready          ,
      input_port_read_out       ,
      input_port_read_out_valid 
    );   
   
  }

  void RunMac() {
    if (state == MAC) {
      NVUINT4   m_index = pe_config.ManagerIndex();
      
      spec::VectorType dp_in0[spec::kNumVectorLanes];
      spec::VectorType dp_in1;
      spec::AccumVectorType dp_out; 
      if (pe_config.is_cluster) {
        // read only half ports 
        // LUT inference 256 lut should be performed simulaneously 
// XXX XXX IMPORTANT!!!!! make sure this part (and the ClusterLookup() ) is correctly synthesized
// PLEASE also try to figure out if clustering actually can save energy       
        #pragma hls_unroll yes
        for (int i = 0; i < 8; i++) {
          spec::HalfVectorType tmp_0, tmp_1;
          VectorSplitting(weight_port_read_out[i], tmp_0, tmp_1);
          dp_in0[2*i]   = pe_manager[m_index].ClusterLookup(tmp_0);
          dp_in0[2*i+1] = pe_manager[m_index].ClusterLookup(tmp_1);
        }
      }
      else {
        #pragma hls_unroll yes
        for (int i = 0; i < 16; i++) {
          dp_in0[i] = weight_port_read_out[i];
        }
      }
      dp_in1 = input_port_read_out[0];
      
      Datapath(dp_in0, dp_in1, dp_out);
      
      #pragma hls_unroll yes
      for (int i = 0; i < spec::kNumVectorLanes; i++) {
        accum_vector[i] += dp_out[i];
      }      
    }
  }
  
  void RunBias() {         
    if (state == BIAS) {
      NVUINT4           m_index = pe_config.ManagerIndex();
      
      
      // determine the shift amount
      // accum_vector fixed format num frac = -(2*spec::kAdpfloatOffset + adpbias_input + adpbias_weight - 2*spec::kAdpfloatManWidth) 
      //   i.e. = -(-18 + adpbias_input + adpbias_weight - 2*4) = 26 - (adpbias_input + adpbias_weight) >= 12, since bias range is 0~7 
      // 20190307 the spec::kActNumFrac is changed to 14, spec::kAdpfloatOffset unchanged
      // XXX: right_shift should work with negative value (20190307)
      // XXX 0310: 18+8 - 8 
      
      NVUINT5 right_shift = -2*spec::kAdpfloatOffset + 2*spec::kAdpfloatManWidth - spec::kActNumFrac
                            - pe_manager[m_index].adplfloat_bias_weight
                            - pe_manager[m_index].adplfloat_bias_input;
      spec::AccumVectorType accum_vector_out;
      
      #pragma hls_unroll yes
      for (int i = 0; i < spec::kNumVectorLanes; i++) {
        accum_vector_out[i] = accum_vector[i] >> right_shift;

        // Can skip appending bias if pe_config.is_bias == 0
        // MERGE BIAS bias_port -> input_port
        if (pe_config.is_bias) {
          AdpfloatType<spec::kAdpfloatWordWidth, spec::kAdpfloatExpWidth> 
              adpfloat_tmp(input_port_read_out[0][i]);
          spec::ActScalarType bias_tmp2 = 
              adpfloat_tmp.to_fixed<spec::kActWordWidth, spec::kActNumFrac>(pe_manager[m_index].adplfloat_bias_bias);
          accum_vector_out[i] += bias_tmp2;
        }
        
        // Do overflow checking and cutting 
        if (accum_vector_out[i] > spec::kActWordMax)
          accum_vector_out[i] = spec::kActWordMax;
        else if (accum_vector_out[i] < spec::kActWordMin) 
          accum_vector_out[i] = spec::kActWordMin;
        
        act_port_reg[i] = accum_vector_out[i];   
      }         
      
    }
  }

  void PushOutput() {
    if (state == OUT) {
      act_port.Push(act_port_reg);
    }
  }
  
  
  void PushAxiRsp() {
    if (w_axi_rsp) {
      // process read rsp from SRAM
      if (weight_port_read_out_valid[0]) {
        rva_out_reg.data = weight_port_read_out[0].to_rawbits();
      }
      // MERGE BIAS
      else if (input_port_read_out_valid[0]) {
        rva_out_reg.data = input_port_read_out[0].to_rawbits();
      }
      rva_out.Push(rva_out_reg);
    } 
    
  }
  
  // Update FSM State and PE_config counters
  void UpdateFSM() {
    FSM next_state;
    switch (state) {
      case IDLE: {
        if (is_start) {
          next_state = PRE;
        }
        else { 
          next_state = IDLE;
        }
        break;  
      }
      case PRE: {
        ResetAccum();
        NVUINT4 m_index = pe_config.ManagerIndex();
        if (pe_manager[m_index].zero_active && pe_config.is_zero_first) {
          // skip MAC
          next_state = BIAS;
        }
        else {
          next_state = MAC;
        }
        break;
      }
       
      case MAC: {
        NVUINT4 m_index = pe_config.ManagerIndex();
        bool is_input_end= 0;
        pe_config.UpdateInputCounter(pe_manager[m_index].num_input, is_input_end);
        if (is_input_end) {
          next_state = BIAS;
        }
        else {
          next_state = MAC;
        }
        break;  
      }
      case BIAS: {
        next_state = OUT;
        break;
      }
      
      case OUT: {
        // Check end condition  
        bool is_output_end = 0;   
        pe_config.UpdateManagerCounter(is_output_end);
        if (is_output_end) {
          next_state = IDLE;
          is_start = 0;
          CDCOUT(sc_time_stamp()  << " PECore: " << name() << " Finish" << endl, kDebugLevel);
        }
        else {
          next_state = PRE;
          CDCOUT(sc_time_stamp()  << " PECore: " << name() << "next state = " << next_state << endl, kDebugLevel);
        }
      }
      default: {
        next_state = IDLE; // Minor fix 02262019
        break;  
      }    
    }
    state = next_state;
  }
  
  void PECoreRun() {
    Reset();
    
    while (1) {
      Initialize();
      RunFSM();
      if (is_start == 0) {
        DecodeAxi(); 
      }
      BufferAccress(); 
      if (is_start == 1) {
        RunMac();
        RunBias();
        PushOutput();
      }
      else {
        PushAxiRsp();
        CheckStart();        
      }
      UpdateFSM();

     
      wait();
    }
  }
  
  
};
#endif

