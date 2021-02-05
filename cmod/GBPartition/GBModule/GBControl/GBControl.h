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

#ifndef GBCONTROL_H
#define GBCONTROL_H

#include <systemc.h>
#include <nvhls_int.h>
#include <nvhls_types.h>
#include <nvhls_vector.h>
#include <nvhls_module.h>
#include "GBSpec.h"
#include "SM6Spec.h"
#include "AxiSpec.h"
/*
  XXX: The current coding style of Push, Pop might not work for II = 1
  Might need another thread for sending read request
  Maybe use wait() between Push Pop will work
*/

  // GB 
  // 1. start 
  // 2. done 
  // 3. Large Data Req
  // 4. Large Data Rsp
  
  // 5. Small Data Req
  // 6. Small Data Rsp
  
  // GB <-> PE
  // 1. Output Activation To PE
  // 2. Input Result from PE
  // 3. Output PEStart 
  // 4. Input PEDone 

// spec::GB::Large::DataRsp<1> read one scalar at a time 

class GBControl : public match::Module {
  static const int kDebugLevel = 4;
  static const int x_index = 0;
  static const int h_index = 1;    
  
  SC_HAS_PROCESS(GBControl);
 public:
  Connections::In<spec::Axi::SlaveToRVA::Write> rva_in;
  Connections::Out<spec::Axi::SlaveToRVA::Read> rva_out;

  Connections::In<bool> start;
  Connections::Out<bool> done;
 
  Connections::Out<spec::GB::Large::DataReq>      large_req;
  Connections::In<spec::GB::Large::DataRsp<1>>    large_rsp;  

  Connections::Out<spec::GB::Small::DataReq>  small_req;
  Connections::In<spec::GB::Small::DataRsp>   small_rsp;
  
  Connections::Out<spec::StreamType> data_out;
  Connections::In<spec::StreamType>  data_in;
  
  Connections::Out<bool> pe_start;
  Connections::In<bool>  pe_done;

  // Constructor
  GBControl (sc_module_name nm)
      : match::Module(nm),
        rva_in("rva_in"),
        rva_out("rva_out"),
        start("start"),
        done("done"),
        large_req("large_req"),
        large_rsp("large_rsp"),
        small_req("small_req"),
        small_rsp("small_rsp"),
        data_out("data_out"),
        data_in("data_in"),
        pe_start("pe_start"),
        pe_done("pe_done")
  {
    SC_THREAD(GBControlRun);
    sensitive << clk.pos();
    async_reset_signal_is(rst, false);
  } 

  // A. FSM
  enum FSM {
    IDLE, SEND, SEND2, START, RECV, SENDBACK, SENDBACK2, NEXT
  };
  FSM state;                 
  
  // Handle done signal first, then handle Act from PE
  
  bool is_start;
  GBControlConfig gbcontrol_config;
  
  bool w_axi_rsp, w_done;
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
    small_req.Reset();
    small_rsp.Reset();
    data_out.Reset();
    data_in.Reset();
    pe_start.Reset();
    pe_done.Reset();  
  }

  void DecodeAxiWrite(const spec::Axi::SlaveToRVA::Write& rva_in_reg){
    NVUINT4     tmp = nvhls::get_slc<4>(rva_in_reg.addr, 20);
    NVUINT16    local_index = nvhls::get_slc<16>(rva_in_reg.addr, 4);
    
    if (tmp == 0x7) {
      gbcontrol_config.ConfigWrite(local_index, rva_in_reg.data);
    }
  }   
  
  
  void DecodeAxiRead(const spec::Axi::SlaveToRVA::Write& rva_in_reg) {
    NVUINT4 tmp = nvhls::get_slc<4>(rva_in_reg.addr, 20);
    NVUINT16    local_index = nvhls::get_slc<16>(rva_in_reg.addr, 4);
    
    // Set Push Response
    w_axi_rsp = 1;
    if (tmp == 0x7) {
      gbcontrol_config.ConfigRead(local_index, rva_out_reg.data);
    }    
  }
  void Initialize() {
    w_axi_rsp     = 0;
    w_done        = 0;
  }
    
  void CheckStart() {
    bool start_reg;
    if (start.PopNB(start_reg)) {
      is_start = gbcontrol_config.is_valid && start_reg;
      CDCOUT(sc_time_stamp()  << name() << " GBControl Start !!!" << endl, kDebugLevel);
    }
  }
  
  void DecodeAxi() {  
    spec::Axi::SlaveToRVA::Write rva_in_reg;
    if (rva_in.PopNB(rva_in_reg)) {
      CDCOUT(sc_time_stamp() << name() << " GBControl RVA Pop " << endl, kDebugLevel);
      if(rva_in_reg.rw) {
        DecodeAxiWrite(rva_in_reg);
      }
      else {
        DecodeAxiRead(rva_in_reg);
      }
    }  
  }
  
  void RunFSM() {
    switch (state) {
      case IDLE: {
        break;
      }
      case SEND: {
        // Send X From GB to PE
        //spec::StreamType data_out_reg;
        NVUINT3  memory_index = gbcontrol_config.memory_index_1;
        NVUINT8  vector_index = gbcontrol_config.GetVectorIndex();
        
        NVUINT16 timestep_index = gbcontrol_config.GetTimestepIndexGBControl();

        // The timestep index here corresponds to hidden state (output) timestep index
        //   for mode == 0: hidden state timestep index equals to input timestep index 
        //   for mode == 1 or 2: hidden state timestep index needs right shift to match input timestep index
        if (gbcontrol_config.mode == 1 || gbcontrol_config.mode == 2) timestep_index = timestep_index >> 1;
        
        //XXX: use GB control version of GetTimestepIndex, the func is controlled by config.mode
        if (gbcontrol_config.mode != 3) { // Non- Decoder mode
          spec::GB::Large::DataReq large_req_reg;
          large_req_reg.is_write = 0;
          large_req_reg.memory_index = memory_index;
          large_req_reg.vector_index = vector_index;
          large_req_reg.timestep_index = timestep_index;
          //cout << "large_req_reg.vector_index: " << large_req_reg.vector_index << "\t large_req_reg.timestep_index: " << large_req_reg.timestep_index << endl;
          large_req.Push(large_req_reg);
        }
        else {
          spec::GB::Small::DataReq small_req_reg;          
          small_req_reg.is_write = 0;
          small_req_reg.memory_index = memory_index;
          small_req_reg.vector_index = vector_index;
          
          small_req.Push(small_req_reg);  
        }
        break;
      }
      case SEND2: {
        spec::StreamType data_out_reg;
        NVUINT8  vector_index = gbcontrol_config.GetVectorIndex();
        if (gbcontrol_config.mode != 3) { // Non- Decoder mode
          spec::GB::Large::DataRsp<1> large_rsp_reg;
          large_rsp_reg = large_rsp.Pop();
          
          data_out_reg.data = large_rsp_reg.read_vector[0];
          data_out_reg.index = x_index;
          data_out_reg.logical_addr = vector_index;
        }
        else {
          spec::GB::Small::DataRsp small_rsp_reg;
          small_rsp_reg = small_rsp.Pop();
          
          data_out_reg.data = small_rsp_reg.read_data;
          data_out_reg.index = x_index;
          data_out_reg.logical_addr = vector_index;  
        }
        // Send The data to PE (Streaming index = 0 => data x)
        data_out.Push(data_out_reg);
        break;
      }
      case START: {
        // send PE start 
        pe_start.Push(1);
        break;
      }
      case RECV: {
        // wait for Done while recieving data from PE and forward it to GB, memory_index_2;
        spec::StreamType data_in_reg;        
        if (data_in.PopNB(data_in_reg)) {
          NVUINT3  memory_index = gbcontrol_config.memory_index_2;
          NVUINT16 timestep_index = gbcontrol_config.GetTimestepIndexGBControl();
          if (gbcontrol_config.mode != 3) { // Non-Decoder mode            
            spec::GB::Large::DataReq large_req_reg;
            large_req_reg.is_write = 1;
            large_req_reg.memory_index = memory_index;
            large_req_reg.vector_index = data_in_reg.logical_addr;
            large_req_reg.timestep_index = timestep_index;
            large_req_reg.write_data = data_in_reg.data;
            large_req.Push(large_req_reg);
            CDCOUT(sc_time_stamp() << name() << " CASE RECV " << endl, kDebugLevel);
          }
          else {
            spec::GB::Small::DataReq small_req_reg;          
            small_req_reg.is_write = 1;
            small_req_reg.memory_index = memory_index;
            small_req_reg.vector_index = data_in_reg.logical_addr;        
            small_req_reg.write_data = data_in_reg.data;
            small_req.Push(small_req_reg);            
          }
        }
        break;
      }
      case SENDBACK: { // data_out_reg.index = 1 for hidden state logical memory in PECore
        // If needed (e.g. RNN), broadcast activation (h) back to PE
        CDCOUT(sc_time_stamp() << name() << " CASE SENDBACK " << endl, kDebugLevel);
        //spec::StreamType data_out_reg;
        NVUINT3  memory_index = gbcontrol_config.memory_index_2;
        NVUINT8  vector_index = gbcontrol_config.GetVectorIndex();
        NVUINT16 timestep_index = gbcontrol_config.GetTimestepIndexGBControl();
        
        if (gbcontrol_config.mode != 3) { // Non- Decoder mode
          spec::GB::Large::DataReq large_req_reg;
          large_req_reg.is_write = 0;
          large_req_reg.memory_index = memory_index;
          large_req_reg.vector_index = vector_index;
          large_req_reg.timestep_index = timestep_index;
          large_req.Push(large_req_reg);
        }
        else { 
          spec::GB::Small::DataReq small_req_reg;          
          small_req_reg.is_write = 0;
          small_req_reg.memory_index = memory_index;
          small_req_reg.vector_index = vector_index;
          
          small_req.Push(small_req_reg); 
        }
        break;
      }
      case SENDBACK2: {
        spec::StreamType data_out_reg;
        NVUINT8  vector_index = gbcontrol_config.GetVectorIndex();
        if (gbcontrol_config.mode != 3) { // Non- Decoder mode
          spec::GB::Large::DataRsp<1> large_rsp_reg;
          large_rsp_reg = large_rsp.Pop();
          
          data_out_reg.data = large_rsp_reg.read_vector[0];
          data_out_reg.index = h_index;
          data_out_reg.logical_addr = vector_index;
        }
        else {
          spec::GB::Small::DataRsp small_rsp_reg;
          small_rsp_reg = small_rsp.Pop();
          
          data_out_reg.data = small_rsp_reg.read_data;
          data_out_reg.index = h_index;
          data_out_reg.logical_addr = vector_index;                   
        }
        // Send The data to PE (Streaming index = 0 => data x)
        data_out.Push(data_out_reg);
        break;        
      }
      case NEXT: {
        CDCOUT(sc_time_stamp() << name() << " CASE NEXT " << endl, kDebugLevel);
        break;
      }
      default: {
        break;
      }
    }
  }
  
  void PushAxiRsp() {
    if (w_axi_rsp) {
      rva_out.Push(rva_out_reg);
    } 
  } 
  
  void UpdateFSM() {
    FSM next_state;
    switch (state) {
      case IDLE: {
        // Wait for start signal (Axi config)
        if (is_start) {
          gbcontrol_config.ResetCounter();
          next_state = SEND;
        }
        else {
          next_state = IDLE;
        }
        break;
      }
      case SEND: {
        next_state = SEND2;
        break;
      }
      case SEND2: {
        // Send Data from GB to PE
        bool is_end = 0;
        gbcontrol_config.UpdateVectorCounter(0, is_end);
        if (is_end) {
          next_state = START;
        }
        else {
          next_state = SEND;
        }
        break;
      }
      case START: {
        // send PE start 
        //cout << "GB send PE Start" << endl;
        next_state = RECV;
        break;
      }
      case RECV: {
        // wait for Done while recieving data from PE and forward it to GB
        bool pe_done_reg;
        if (pe_done.PopNB(pe_done_reg)) {
          if (gbcontrol_config.is_rnn) {
            next_state = SENDBACK;
          }
          else {
            next_state = NEXT;
          }
        }
        else {
          next_state = RECV;
        }
        break;
      }
      case SENDBACK: {
        next_state = SENDBACK2;
        break;
      }
      case SENDBACK2: {
        // If needed (e.g. RNN), broadcast activation back to PE
        bool is_end = 0;
        gbcontrol_config.UpdateVectorCounter(1, is_end);
        if (is_end) {
          next_state = NEXT;
        }
        else {
          next_state = SENDBACK;
        }
        break;
      }
      case NEXT: {
        // Move to next timestep
        bool is_end = 0;
        gbcontrol_config.UpdateTimestepCounter(is_end);
        if (is_end) {
          // Pushdone 
          is_start = 0;
          next_state = IDLE;
          CDCOUT(sc_time_stamp()  << " GBControl: " << name() << " Finish" << endl, kDebugLevel);
          done.Push(1);    
        }
        else {
          next_state = SEND;
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
  
  void GBControlRun() {
  
    Reset();  
    #pragma hls_pipeline_init_interval 1 
    while(1){
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
