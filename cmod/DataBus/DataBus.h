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

#ifndef __DATABUS__
#define __DATABUS__

#include <systemc.h>
#include <nvhls_int.h>
#include <nvhls_types.h>
#include <nvhls_vector.h>
#include <nvhls_module.h>
#include <arbitrated_crossbar.h>

#include "SM6Spec.h"
#include "AxiSpec.h"

// kNumPE = 8

SC_MODULE(PEStart) {
  static const int kDebugLevel = 6;
  static const int kSendDelay = spec::kGlobalTriggerDelay;
 public:
  sc_in<bool>  clk;
  sc_in<bool>  rst; 
  Connections::In<bool>           all_pe_start;
  Connections::OutBuffered<bool>  pe_start_array[spec::kNumPE];

  
  Connections::Combinational<bool> trigger;
      
  SC_HAS_PROCESS(PEStart);
  PEStart(sc_module_name name)
     : sc_module(name), 
     clk("clk"), 
     rst("rst")
  {
  
    SC_THREAD(RecvAllStart);
    sensitive << clk.pos();
    async_reset_signal_is(rst, false);
    
    SC_THREAD(SendPEStart);
    sensitive << clk.pos();
    async_reset_signal_is(rst, false);
  }      
  
  void RecvAllStart() {
    all_pe_start.Reset();
    trigger.ResetWrite();  

    #pragma hls_pipeline_init_interval 1
    while(1) {
      bool all_pe_start_reg;
      if (all_pe_start.PopNB(all_pe_start_reg)) {
        trigger.Push(1);
      }
      
      wait();
    }
  }
  void SendPEStart() {
    #pragma hls_unroll yes    
    for (int i = 0; i < spec::kNumPE; i++) {
      pe_start_array[i].Reset();
    }
    trigger.ResetRead();
    NVUINT6 state = 0;
    bool trigger_reg = 0;

    #pragma hls_pipeline_init_interval 1
    while(1) {
      #pragma hls_unroll yes    
      for (int i = 0; i < spec::kNumPE; i++) {
        pe_start_array[i].TransferNB();
      }   
   

      if (state >= kSendDelay && trigger_reg == 1) {
        trigger_reg = 0;
        state = 0;
        #pragma hls_unroll yes    
        for (int i = 0; i < spec::kNumPE; i++) {
          pe_start_array[i].Push(1);
        }           
      }
      else if (state >= 1) {      
        state += 1;
      }
      else if (trigger.PopNB(trigger_reg)) {
        state = 1;
      }
      
      wait();
    }
  }  
};

SC_MODULE(PEDone) {
  static const int kDebugLevel = 6;
  static const int kSendDelay = spec::kGlobalTriggerDelay;
 public:
  sc_in<bool>  clk;
  sc_in<bool>  rst; 
  Connections::In<bool>   pe_done_array[spec::kNumPE];
  Connections::Out<bool>  all_pe_done;
  
  Connections::Combinational<bool> trigger;
  
  
  NVUINTW(spec::kNumPE) done_indicator;
  
  SC_HAS_PROCESS(PEDone);
  PEDone(sc_module_name name)
     : sc_module(name), 
     clk("clk"), 
     rst("rst")
  {
  
    SC_THREAD(RecvPEDone);
    sensitive << clk.pos();
    async_reset_signal_is(rst, false);
    
    SC_THREAD(SendAllDone);
    sensitive << clk.pos();
    async_reset_signal_is(rst, false);
  }  
  
  void RecvPEDone() {
    #pragma hls_unroll yes    
    for (int i = 0; i < spec::kNumPE; i++) {
      pe_done_array[i].Reset();
    }
    trigger.ResetWrite();  
    done_indicator = 0;

    #pragma hls_pipeline_init_interval 1
    while(1) {
      #pragma hls_unroll yes
      for (int i = 0; i < spec::kNumPE; i++) {
        bool done_reg = 0;
        done_indicator[i] = done_indicator[i] | pe_done_array[i].PopNB(done_reg);
      }      
      
      if (done_indicator.and_reduce()) {
        done_indicator = 0;
        trigger.Push(1);
      }
      
      wait();
    }
  }
  void SendAllDone() {
    trigger.ResetRead();
    all_pe_done.Reset();

    #pragma hls_pipeline_init_interval 1    
    while(1) {
      trigger.Pop();
      #pragma hls_pipeline_init_interval 1      
      for (int i = 0; i < kSendDelay; i++) {
        wait();
      }
      all_pe_done.Push(1);
      wait();
    }
  }
};


SC_MODULE(GBSend) { 
  static const int kDebugLevel = 6;
 public:
  sc_in<bool>  clk;
  sc_in<bool>  rst; 
  
  Connections::In<spec::StreamType>   gb_output;   
  Connections::OutBuffered<spec::StreamType>  pe_inputs[spec::kNumPE];
 
  // note: does not give the name for I/O connections
  SC_HAS_PROCESS(GBSend);
  GBSend(sc_module_name name)
     : sc_module(name), 
     clk("clk"), 
     rst("rst")
  {
  
    SC_THREAD(Run);
    sensitive << clk.pos();
    async_reset_signal_is(rst, false);
  }
  
  
  void Run() {
    gb_output.Reset();
    #pragma hls_unroll yes    
    for (int i = 0; i < spec::kNumPE; i++) {
      pe_inputs[i].Reset();
    }
    
    #pragma hls_pipeline_init_interval 2
    while (1) {
      // TransferNB
      #pragma hls_unroll yes    
      for (int i = 0; i < spec::kNumPE; i++) {
        pe_inputs[i].TransferNB();
      }
      NVUINTW(spec::kNumPE) is_full_array = 0;       
      #pragma hls_unroll yes
      for (int i = 0; i < spec::kNumPE; i++) {
        is_full_array[i] = pe_inputs[i].Full();      
      }
      if (!is_full_array.or_reduce()) {
        spec::StreamType gb_output_reg;
        if (gb_output.PopNB(gb_output_reg)) {
          #pragma hls_unroll yes    
          for (int i = 0; i < spec::kNumPE; i++) {
            pe_inputs[i].Push(gb_output_reg);
          }
        }
      }
      wait();
    }
  }
};



// ArbitratedCrossbar
// copied and modified from matchlib, only support 1 output 
// data_in: pe_outputs:
// data_out: gb_input:
class GBRecv : public match::Module {
  static const int NumInputs       = spec::kNumPE;
  static const int NumOutputs      = 1;
  static const int LenInputBuffer  = 4; //FIXME: we might need larger input buffer 
  static const int LenOutputBuffer = 1;
  typedef spec::StreamType DataType;
  typedef NVUINTW(nvhls::index_width<NumOutputs>::val) OutIdxType;
  typedef NVUINTW(Wrapped<OutIdxType>::width + Wrapped<DataType>::width)  DataDestType;

  typedef DataDestType DataDestInArray[NumInputs];
  typedef DataType     DataInArray[NumInputs];
  typedef OutIdxType   OutIdxArray[NumInputs];
  typedef bool         ValidInArray[NumInputs];
  typedef DataType     DataOutArray[NumOutputs];
  typedef bool         ValidOutArray[NumOutputs];
  typedef bool         ReadyArray[NumInputs];

 public:
  Connections::In<DataType>     data_in[NumInputs];
  Connections::Out<DataType>    data_out[NumOutputs];

  ArbitratedCrossbar<DataType, NumInputs, NumOutputs, LenInputBuffer, LenOutputBuffer> arbxbar;

  SC_HAS_PROCESS(GBRecv);
  GBRecv(sc_module_name name_)
      : match::Module(name_) {
    
    SC_THREAD(Run);
    sensitive << clk.pos();
    async_reset_signal_is(rst, false);
    
    // Disable Trace
    this->SetTraceLevel(0);
  }
  
  void Run() { 
    #pragma hls_unroll yes
    for(int inp_lane=0; inp_lane<NumInputs; inp_lane++) {
      data_in[inp_lane].Reset();
    }

    #pragma hls_unroll yes
    for(int out_lane=0; out_lane<NumOutputs; out_lane++) {
      data_out[out_lane].Reset();
    }

    #pragma hls_pipeline_init_interval 1
    while(1) {
      wait();
      T(1) << "##### Entered DUT #####" << EndT;

      //DataDestInArray data_dest_in_reg;
      DataInArray     data_in_reg;
      OutIdxArray     dest_in_reg;
      ValidInArray    valid_in_reg;
      #pragma hls_unroll yes
      for(int inp_lane=0; inp_lane<NumInputs; inp_lane++) {
	dest_in_reg[inp_lane]  = 0;
        if(!arbxbar.isInputFull(inp_lane) && LenInputBuffer > 0) {
	        valid_in_reg[inp_lane] = data_in[inp_lane].PopNB(data_in_reg[inp_lane]);
	        //data_in_reg[inp_lane]  = static_cast<DataType>   (data_dest_in_reg[inp_lane]);
	        // only 1 output: idx = 0 	        
	        //dest_in_reg[inp_lane]  = 0;
	        //static_cast<OutIdxType> (data_dest_in_reg[inp_lane] >> Wrapped<DataType>::width);
        } else {
          valid_in_reg[inp_lane] = false;
        }
          T(2) << "data_in["   << inp_lane << "] = " << data_in_reg[inp_lane]
               << " dest_in["  << inp_lane << "] = " << dest_in_reg[inp_lane]
               << " valid_in[" << inp_lane << "] = " << valid_in_reg[inp_lane] << EndT;
      }

      DataOutArray  data_out_reg;
      ValidOutArray valid_out_reg;
      ReadyArray    ready_reg;
      arbxbar.run(data_in_reg, dest_in_reg, valid_in_reg, data_out_reg, valid_out_reg, ready_reg);

      if(LenOutputBuffer > 0) {
        arbxbar.pop_all_lanes(valid_out_reg);
      }

      // Push only the valid outputs.
      #pragma hls_unroll yes
      for(int out_lane=0; out_lane<NumOutputs; out_lane++) {
        if(valid_out_reg[out_lane]) {
          data_out[out_lane].Push(data_out_reg[out_lane]);
          T(2) << "data_out[" << out_lane << "] = " << data_out_reg[out_lane] << EndT;
        }
      }
    }
  }  
};







#endif
