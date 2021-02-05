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

#ifndef SRAMTOP_H
#define SRAMTOP_H

#include <systemc.h>
#include <nvhls_int.h>
#include <nvhls_types.h>
#include <nvhls_vector.h>
#include <nvhls_module.h>
#include <ArbitratedScratchpadDP.h>

#include "SM6Spec.h"
#include "AxiSpec.h"
#include "PECoreSpec.h"


class SRAMTop : public match::Module {
  static const int kDebugLevel = 4;
  SC_HAS_PROCESS(SRAMTop);
 public:
  Connections::In<spec::Axi::SlaveToRVA::Write> rva_in;
  Connections::Out<spec::Axi::SlaveToRVA::Read> rva_out;
  
  // single port input SRAM
  ArbitratedScratchpadDP<spec::PE::Input::kNumBanks,
                         spec::PE::Input::kNumReadPorts,
                         spec::PE::Input::kNumWritePorts,
                         spec::PE::Input::kEntriesPerBank, 
                         spec::PE::Input::WordType, 
                         false, true> input_mem;            
  
  
  // while loop control signal (including SRAM I/O)
  bool w_axi_rsp;
  spec::Axi::SlaveToRVA::Read rva_out_reg;  

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
  SRAMTop (sc_module_name nm)
      : match::Module(nm),
        rva_in("rva_in"),
        rva_out("rva_out")
  {
    SC_THREAD(SRAMTopRun);
    sensitive << clk.pos();
    async_reset_signal_is(rst, false);
  }

// Reset famalies ///////////
  void Reset() {
    ResetPorts();
  }

  void ResetPorts() {
    rva_in.Reset();
    rva_out.Reset();
  }

  void ResetBufferInputs() {
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
            break;        
          }
          case 0x2: {     // manager 0 config
            break; 
          }
          case 0x3: {     // manager 0 Cluster
            break; 
          }    
          case 0x4: {     // manager 1 config
            break; 
          }     
          case 0x5: {     // manager 1 cluster
            break; 
          }          
          default: {
            break;
          }
        }
        break;
      }
      case 0x5: {     // Weight Buffer
        break;
      }
      case 0x6: {     // Input Buffer (lock datapath)
        input_write_addrs         [0] = local_index;
        input_write_req_valid     [0] = 1;
        input_write_data          [0] = rva_in_reg.data;       
        break;
      }
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
        break;
      }    
      case 0x4: {     // PEconfig
        switch (local_index) {
          case 0x1: {
            break;
          }
          case 0x2: {     // manager 0 config
            break; 
          }
          case 0x3: {     // manager 0 Cluster
            break; 
          }    
          case 0x4: {     // manager 1 config
            break; 
          }     
          case 0x5: {     // manager 1 cluster
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

  void Initialize() {
    ResetBufferInputs();
    w_axi_rsp     = 0;
  }
  
  void DecodeAxi() {  
    spec::Axi::SlaveToRVA::Write rva_in_reg;
    if (rva_in.PopNB(rva_in_reg)) {
      CDCOUT(sc_time_stamp()  << " SRAMTop: " << name() << "RVA Pop " << endl, kDebugLevel);
      if(rva_in_reg.rw) {
        DecodeAxiWrite(rva_in_reg);
      }
      else {
        DecodeAxiRead(rva_in_reg);
      }
    }  
  }

  void BufferAccess() {
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

   
  void PushAxiRsp() {
    if (w_axi_rsp) {
      if (input_port_read_out_valid[0]) {
        rva_out_reg.data = input_port_read_out[0].to_rawbits();
      }
      rva_out.Push(rva_out_reg);
    } 
  }
  
  void SRAMTopRun() {
    Reset();
    
    while (1) {
      Initialize();
      DecodeAxi(); 
      BufferAccess(); 
      PushAxiRsp();

      wait();
    }
  }
  
};
#endif

