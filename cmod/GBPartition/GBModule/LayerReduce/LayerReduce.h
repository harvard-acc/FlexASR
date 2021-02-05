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

#ifndef __LAYERREDUCE__
#define __LAYERREDUCE__

#include <systemc.h>
#include <nvhls_int.h>
#include <nvhls_types.h>
#include <nvhls_vector.h>
#include <nvhls_module.h>
#include "SM6Spec.h"
#include "AxiSpec.h"
#include "GBSpec.h"
#include "AdpfloatSpec.h"
#include "AdpfloatUtils.h"

class LayerReduce : public match::Module {
  static const int kDebugLevel = 4;
  SC_HAS_PROCESS(LayerReduce);
 public:
  Connections::In<spec::Axi::SlaveToRVA::Write> rva_in;
  Connections::Out<spec::Axi::SlaveToRVA::Read> rva_out;

  Connections::In<bool> start;
  Connections::Out<bool> done;
 
  Connections::Out<spec::GB::Large::DataReq>      large_req;
  Connections::In<spec::GB::Large::DataRsp<2>>    large_rsp;  

  // Constructor
  LayerReduce (sc_module_name nm)
      : match::Module(nm),
        rva_in("rva_in"),
        rva_out("rva_out"),
        start("start"),
        done("done"),
        large_req("large_req"),
        large_rsp("large_rsp")
  {
    SC_THREAD(LayerReduceRun);
    sensitive << clk.pos();
    async_reset_signal_is(rst, false);
  } 
  
  // A. FSM
  enum FSM {
    IDLE, PRE, REDUCE, FIN
  };
  FSM state;                 
  
  // Handle done signal first, then handle Act from PE
  
  bool is_start;
  GBControlConfig gbcontrol_config;  
  bool w_axi_rsp; //w_done;
  spec::Axi::SlaveToRVA::Read rva_out_reg;  
  
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

  void DecodeAxiWrite(const spec::Axi::SlaveToRVA::Write& rva_in_reg){
    NVUINT4     tmp = nvhls::get_slc<4>(rva_in_reg.addr, 20);
    NVUINT16    local_index = nvhls::get_slc<16>(rva_in_reg.addr, 4);
    
    if (tmp == 0x8) {
      gbcontrol_config.ConfigWrite(local_index, rva_in_reg.data);
    }
  }   
  
  
  void DecodeAxiRead(const spec::Axi::SlaveToRVA::Write& rva_in_reg) {
    NVUINT4 tmp = nvhls::get_slc<4>(rva_in_reg.addr, 20);
    NVUINT16    local_index = nvhls::get_slc<16>(rva_in_reg.addr, 4);
    
    // Set Push Response
    w_axi_rsp = 1;
    if (tmp == 0x8) {
      gbcontrol_config.ConfigRead(local_index, rva_out_reg.data);
    }    
  }
  void Initialize() {
    w_axi_rsp     = 0;
    //w_done        = 0;
  }
    
  void CheckStart() {
    bool start_reg;
    if (start.PopNB(start_reg)) {
      is_start = gbcontrol_config.is_valid && start_reg;
      CDCOUT(sc_time_stamp()  << name() << " LayerReduce Start !!!" << endl, kDebugLevel);
    }
  }
  
  void DecodeAxi() {  
    spec::Axi::SlaveToRVA::Write rva_in_reg;
    if (rva_in.PopNB(rva_in_reg)) {
      CDCOUT(sc_time_stamp() << name() << " LayerReduce RVA Pop " << endl, kDebugLevel);
      if(rva_in_reg.rw) {
        DecodeAxiWrite(rva_in_reg);
      }
      else {
        DecodeAxiRead(rva_in_reg);
      }
    }  
  }
  
  
  void RunFSM(){
    switch (state) {
      case IDLE: {
        
        break;
      }
      case PRE: {
        NVUINT3   memory_index = gbcontrol_config.memory_index_1;
        NVUINT8   vector_index = gbcontrol_config.GetVectorIndex();
        NVUINT16  timestep_index = gbcontrol_config.GetTimestepIndex();
        //NVUINT3   mode = gbcontrol_config.mode;

        spec::GB::Large::DataReq large_req_reg;
        large_req_reg.is_write = 0;
        large_req_reg.memory_index = memory_index;
        large_req_reg.vector_index = vector_index;
        large_req_reg.timestep_index = timestep_index;
        //large_req_reg.write_data = 0;
        large_req.Push(large_req_reg);
        break;
      }
        // it should receive two data from adjecent timestep
        // large_rsp_reg.read_vector[0], large_rsp_reg.read_vector[1];
      case REDUCE: {
        NVUINT3   memory_index = gbcontrol_config.memory_index_1;
        NVUINT8   vector_index = gbcontrol_config.GetVectorIndex();
        NVUINT16  timestep_index = gbcontrol_config.GetTimestepIndex();
        NVUINT3   mode = gbcontrol_config.mode;
        spec::GB::Large::DataReq large_req_reg;

        spec::GB::Large::DataRsp<2> large_rsp_reg;
        large_rsp_reg = large_rsp.Pop();
        
        spec::VectorType write_data;
        if (mode == 0) { // max pool
          #pragma hls_unroll yes
          for (int i = 0; i < spec::kVectorSize; i++) {
            AdpfloatType<spec::kAdpfloatWordWidth,spec::kAdpfloatExpWidth> in_a(large_rsp_reg.read_vector[0][i]);
            AdpfloatType<spec::kAdpfloatWordWidth,spec::kAdpfloatExpWidth> in_b(large_rsp_reg.read_vector[1][i]);  
            AdpfloatType<spec::kAdpfloatWordWidth,spec::kAdpfloatExpWidth> out_tmp;
            adpfloat_max(in_a, in_b, out_tmp);
            write_data[i] = out_tmp.to_rawbits();
          }
        }
        else if (mode == 1) { // mean pool
          #pragma hls_unroll yes
          for (int i = 0; i < spec::kVectorSize; i++) {
            AdpfloatType<spec::kAdpfloatWordWidth,spec::kAdpfloatExpWidth> in_a(large_rsp_reg.read_vector[0][i]);
            AdpfloatType<spec::kAdpfloatWordWidth,spec::kAdpfloatExpWidth> in_b(large_rsp_reg.read_vector[1][i]);  
            AdpfloatType<spec::kAdpfloatWordWidth,spec::kAdpfloatExpWidth> out_tmp;
            adpfloat_mean(in_a, in_b, out_tmp);
            write_data[i] = out_tmp.to_rawbits();            
          }          
        }
        else { // layer add
          #pragma hls_unroll yes
          for (int i = 0; i < spec::kVectorSize; i++) {
            AdpfloatType<spec::kAdpfloatWordWidth, spec::kAdpfloatExpWidth> in_a(large_rsp_reg.read_vector[0][i]);
            AdpfloatType<spec::kAdpfloatWordWidth, spec::kAdpfloatExpWidth> in_b(large_rsp_reg.read_vector[1][i]);  
            AdpfloatType<spec::kAdpfloatWordWidth, spec::kAdpfloatExpWidth> out_tmp;
            adpfloat_add(in_a, in_b, out_tmp);
            write_data[i] = out_tmp.to_rawbits();  
          }        
        }
        // when write, please divide the timestep index by one
        large_req_reg.is_write = 1;
        large_req_reg.memory_index = memory_index;
        large_req_reg.vector_index = vector_index;
        large_req_reg.timestep_index = (timestep_index >> 1);
        large_req_reg.write_data = write_data;
        large_req.Push(large_req_reg);
        break;
      }
      case FIN: {
        break;
      }    
      default: {
        break;
      }
    }
  }
  
  void UpdateFSM(){
    FSM next_state;
    switch (state) {
      case IDLE: {
        // Wait for start signal (Axi config)
        if (is_start) {
          gbcontrol_config.ResetCounter();
          next_state = PRE;
        }
        else {
          next_state = IDLE;
        }
        break;
      }
      case PRE: {
        next_state = REDUCE;
        break;
      }
      case REDUCE: {
        // Send Data from GB to PE
        bool is_end_0 = 0, is_end_1 = 0;
        gbcontrol_config.UpdateVectorCounter(is_end_0);
        if (is_end_0) { 
          // update by 2 for layer reduce operation
          gbcontrol_config.UpdateTimestepCounterByTwo(is_end_1);
          if (is_end_1) {
            next_state = FIN; 
          }
          else {
            next_state = PRE;          
          }         
        }
        else {
          next_state = PRE;
        }
        break;
        
      }
      case FIN: {
        is_start = 0;
        done.Push(1);
        CDCOUT(sc_time_stamp() << name() << " LayerReduce Finish" << endl, kDebugLevel);
        next_state = IDLE;
        break;
      }
      default: {
        next_state = IDLE;
        break;
      }
    }  
    state = next_state;        
  }

  void PushAxiRsp() {
    if (w_axi_rsp) {
      rva_out.Push(rva_out_reg);
    } 
  } 
  
  void LayerReduceRun(){
    Reset();
    #pragma hls_pipeline_init_interval 2 
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
