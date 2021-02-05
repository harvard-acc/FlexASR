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

#ifndef __ZEROPADDING__
#define __ZEROPADDING__

#include <systemc.h>
#include <nvhls_int.h>
#include <nvhls_types.h>
#include <nvhls_vector.h>
#include <nvhls_module.h>
#include "GBSpec.h"
#include "SM6Spec.h"
#include "AxiSpec.h"

// count from numtimestep_1 to numtimestep_2 -1
class ZeroPadding : public match::Module {
  static const int kDebugLevel = 4;
  SC_HAS_PROCESS(ZeroPadding);
 public:
  Connections::In<spec::Axi::SlaveToRVA::Write> rva_in;
  Connections::Out<spec::Axi::SlaveToRVA::Read> rva_out;

  Connections::In<bool> start;
  Connections::Out<bool> done;
 
  Connections::Out<spec::GB::Large::DataReq>      large_req;
  Connections::In<spec::GB::Large::DataRsp<1>>    large_rsp;  
  
  // Constructor
  ZeroPadding (sc_module_name nm)
      : match::Module(nm),
        rva_in("rva_in"),
        rva_out("rva_out"),
        start("start"),
        done("done"),
        large_req("large_req"),
        large_rsp("large_rsp")
  {
    SC_THREAD(ZeroPaddingRun);
    sensitive << clk.pos();
    async_reset_signal_is(rst, false);
  }
  bool is_start;
  GBControlConfig gbcontrol_config;
  
  bool w_axi_rsp;  
  spec::Axi::SlaveToRVA::Read rva_out_reg;   
  enum FSM {
    IDLE, ZERO, NEXT
  };
  FSM state; 
  
  void Reset() {
    state = IDLE;
    is_start = 0;
    gbcontrol_config.Reset();
    ResetPorts();
  }
  
  void ResetPorts() { 
    rva_in.Reset();
    rva_out.Reset();
    start.Reset();
    done.Reset();
    large_req.Reset();
    large_rsp.Reset();
  }
  
  void Initialize() {
    w_axi_rsp     = 0;
  }  

  void CheckStart() {
    bool start_reg;
    if (start.PopNB(start_reg)) {
      is_start = gbcontrol_config.is_valid && start_reg;
      CDCOUT(sc_time_stamp()  << name() << " ZeroPadding Start !!!" << endl, kDebugLevel);
    }
  }
  
  void DecodeAxiWrite(const spec::Axi::SlaveToRVA::Write& rva_in_reg){
    NVUINT4     tmp = nvhls::get_slc<4>(rva_in_reg.addr, 20);
    NVUINT16    local_index = nvhls::get_slc<16>(rva_in_reg.addr, 4);
    
    if (tmp == 0xA) {
      gbcontrol_config.ConfigWrite(local_index, rva_in_reg.data);
    }
  }   
  
  
  void DecodeAxiRead(const spec::Axi::SlaveToRVA::Write& rva_in_reg) {
    NVUINT4 tmp = nvhls::get_slc<4>(rva_in_reg.addr, 20);
    NVUINT16    local_index = nvhls::get_slc<16>(rva_in_reg.addr, 4);
    
    // Set Push Response
    w_axi_rsp = 1;
    if (tmp == 0xA) {
      gbcontrol_config.ConfigRead(local_index, rva_out_reg.data);
    }    
  }
  
  void DecodeAxi() {  
    spec::Axi::SlaveToRVA::Write rva_in_reg;
    if (rva_in.PopNB(rva_in_reg)) {
      CDCOUT(sc_time_stamp() << name() << "RVA Pop " << endl, kDebugLevel);
      if(rva_in_reg.rw) {
        DecodeAxiWrite(rva_in_reg);
      }
      else {
        DecodeAxiRead(rva_in_reg);
      }
    }  
  }

  void PushAxiRsp() {
    if (w_axi_rsp) {
      rva_out.Push(rva_out_reg);
    } 
  } 

  void RunFSM() {
    switch (state) {
      case IDLE: {
        break;
      }
      case ZERO: {
        // Send Zero write Request write_data = 0
        NVUINT3   memory_index = gbcontrol_config.memory_index_1;
        NVUINT8   vector_index = gbcontrol_config.GetVectorIndex();
        NVUINT16  timestep_index = gbcontrol_config.GetTimestepIndex();

        spec::GB::Large::DataReq large_req_reg;
        large_req_reg.is_write = 1;
        large_req_reg.memory_index = memory_index;
        large_req_reg.vector_index = vector_index;
        large_req_reg.timestep_index = timestep_index;
        large_req_reg.write_data = 0;        
        large_req.Push(large_req_reg);
        break;
      }
      case NEXT: {

        break;
      }
      default: {
        break;
      }
    }
  }
  
  void UpdateFSM() {
    FSM next_state;
    switch (state) {
      case IDLE: {
        // Wait for start signal (Axi config)
        if (is_start) {
          gbcontrol_config.ResetCounter();
          gbcontrol_config.timestep_counter = gbcontrol_config.num_timestep_1;
          next_state = ZERO;
        }
        else {
          next_state = IDLE;
        }
        break;
      }
      case ZERO: {
        bool is_end = 0;
        gbcontrol_config.UpdateVectorCounter(is_end);
        if (is_end) {
          next_state = NEXT;
        }
        else {
          next_state = ZERO;
        }
        break;
      }
      case NEXT: {
        bool is_end = 0;
        gbcontrol_config.UpdateTimestepCounterZeroPadding(is_end);
        if (is_end) {
          // Pushdone 
          is_start = 0;
          next_state = IDLE;
          CDCOUT(sc_time_stamp()  <<  name() << " ZeroPadding Finish" << endl, kDebugLevel);
          done.Push(1);    
        }
        else {
          next_state = ZERO;
        }
        break;
      }
      default: {
        next_state = IDLE;
        break;
      }
      
    }      
    state = next_state;
  }
  
  void ZeroPaddingRun() {
    Reset();
    #pragma hls_pipeline_init_interval 1
    while(1) {
      Initialize();
      RunFSM();
      if (is_start == 0) {
        DecodeAxi();
        PushAxiRsp();
        CheckStart();
      }
      UpdateFSM();
      wait();
    }
  }
};

#endif
