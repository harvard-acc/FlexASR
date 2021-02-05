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

#ifndef __LAYERNORM__
#define __LAYERNORM__

#include <systemc.h>
#include <nvhls_int.h>
#include <nvhls_types.h>
#include <nvhls_vector.h>
#include <nvhls_module.h>
#include "GBSpec.h"
#include "SM6Spec.h"
#include "AxiSpec.h"
#include "AdpfloatSpec.h"
#include "PEPartition/PEModule/ActUnit/PPU/PPU.h" 
// vector mul
// vector add (minus)
// vector divide
// also need sqrt 

// Initial Interval should be II = 2

class LayerNorm : public match::Module {
  static const int kDebugLevel = 4;
  static const int gamma_index = 5;
  static const int beta_index = 6;  
  SC_HAS_PROCESS(LayerNorm);
  
  spec::LayerNormSumType sum, sqsum;
  spec::ActScalarType mean, inv_std;
  spec::ActVectorType negmean_vector, inv_std_vector;
  spec::ActVectorType out_data;  
 public:
  Connections::In<spec::Axi::SlaveToRVA::Write> rva_in;
  Connections::Out<spec::Axi::SlaveToRVA::Read> rva_out;

  Connections::In<bool> start;
  Connections::Out<bool> done;
 
  Connections::Out<spec::GB::Large::DataReq>      large_req;
  Connections::In<spec::GB::Large::DataRsp<1>>    large_rsp;  
  
  Connections::Out<spec::GB::Small::DataReq>  small_req;
  Connections::In<spec::GB::Small::DataRsp>   small_rsp;  
  
  
  // The memory index of gramma = 6, beta = 7 
  
  // convert to fixed point before any computation 
  
  // Constructor
  LayerNorm (sc_module_name nm)
      : match::Module(nm),
        rva_in("rva_in"),
        rva_out("rva_out"),
        start("start"),
        done("done"),
        large_req("large_req"),
        large_rsp("large_rsp"),
        small_req("small_req"),
        small_rsp("small_rsp")
  {
    SC_THREAD(LayerNormRun);
    sensitive << clk.pos();
    async_reset_signal_is(rst, false);
  }
  bool is_start;
  GBControlConfig gbcontrol_config;
  
  bool w_axi_rsp;  
  spec::Axi::SlaveToRVA::Read rva_out_reg;  
  // A. FSM
  enum FSM {
    IDLE, MEAN, MEAN2, VAR, NORM, NORM2, GAMMA, GAMMA2, BETA, BETA2, BETA3, NEXT
  };
  FSM state;
  // Find mean first -> var -> norm


  void Reset() {
    state = IDLE;
    is_start = 0;
    gbcontrol_config.Reset();
    ResetPorts();
    ResetMeanVar();
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
  }
  
  void ResetMeanVar() {
    sum = 0;
    sqsum = 0;
    mean = 0;
    inv_std = 0;
  }
  
  void Initialize() {
    w_axi_rsp     = 0;
    //w_done        = 0;
  }

  void CheckStart() {
    bool start_reg;
    if (start.PopNB(start_reg)) {
      is_start = gbcontrol_config.is_valid && start_reg;
      CDCOUT(sc_time_stamp()  << name() << " LayerNorm Start !!!" << endl, kDebugLevel);
    }
  }
  
  void DecodeAxiWrite(const spec::Axi::SlaveToRVA::Write& rva_in_reg){
    NVUINT4     tmp = nvhls::get_slc<4>(rva_in_reg.addr, 20);
    NVUINT16    local_index = nvhls::get_slc<16>(rva_in_reg.addr, 4);
    
    if (tmp == 0x9) {
      gbcontrol_config.ConfigWrite(local_index, rva_in_reg.data);
    }
  }   
  
  
  void DecodeAxiRead(const spec::Axi::SlaveToRVA::Write& rva_in_reg) {
    NVUINT4 tmp = nvhls::get_slc<4>(rva_in_reg.addr, 20);
    NVUINT16    local_index = nvhls::get_slc<16>(rva_in_reg.addr, 4);
    
    // Set Push Response
    w_axi_rsp = 1;
    if (tmp == 0x9) {
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
      case MEAN: {
        CDCOUT(sc_time_stamp()  << name() << " case MEAN" << endl, kDebugLevel);
        // one pass of Large Buffer read to set mean amd sqmean
        // E[x^2] - [E[x]]^2
        NVUINT3   memory_index = gbcontrol_config.memory_index_1;
        NVUINT8   vector_index = gbcontrol_config.GetVectorIndex();
        NVUINT16  timestep_index = gbcontrol_config.GetTimestepIndex();
        //spec::AdpfloatBiasType adpbias_enc = gbcontrol_config.adpbias_1;
        // Send Req
        spec::GB::Large::DataReq large_req_reg;
        large_req_reg.is_write = 0;
        large_req_reg.memory_index = memory_index;
        large_req_reg.vector_index = vector_index;
        large_req_reg.timestep_index = timestep_index;
        large_req.Push(large_req_reg);
        break;
      }
      case MEAN2: {
        CDCOUT(sc_time_stamp()  << name() << " case MEAN2" << endl, kDebugLevel);
        // Receive Rsp        
        spec::AdpfloatBiasType adpbias_enc = gbcontrol_config.adpbias_1;
        spec::GB::Large::DataRsp<1> large_rsp_reg;
        large_rsp_reg = large_rsp.Pop();

        spec::ActVectorType x_vector;
        spec::ActVectorType sq_vector;
        spec::ActScalarType tmp_sum, tmp_sqsum;
        
        // Get activation vector in ActScalarType
        Adpfloat2Fixed(large_rsp_reg.read_vector[0],  x_vector, adpbias_enc);       
        // Get x^2
        EMul(x_vector, x_vector, sq_vector);
        // sum[x]
        VSum(x_vector, tmp_sum);
        // sum[x^2]
        VSum(sq_vector, tmp_sqsum);        
        
        sum += tmp_sum;
        sqsum += tmp_sqsum;
        break;
      }
      case VAR: {
        CDCOUT(sc_time_stamp()  << name() << " case VAR" << endl, kDebugLevel);
        // calculate the mean of  sum[X]/N, sum[X^2]/N, and E[X]^2
        NVUINT8 num_vector = gbcontrol_config.num_vector_1;
        spec::ActScalarType sqmean, meansq;
        NVINTW(2*spec::kActWordWidth) meansq_tmp;
        NVUINT14 dividsor;
        dividsor = num_vector; 
        dividsor = dividsor << nvhls::log2_floor<spec::kVectorSize>::val; // this val == 4
        // E[X], E[X^2]
        mean = sum / dividsor;
        sqmean = sqsum/ dividsor;
        
        // E[X]^2 
        meansq_tmp = mean * mean;
        meansq_tmp = meansq_tmp >> spec::kActNumFrac;
        meansq = meansq_tmp;
        
        // VAR[X] = E[X^2] - E[X]^2
        spec::ActScalarType var = sqmean - meansq;
        // We use inv_std = 1/sqrt(VAR[X])
        SInvSqrt(var, inv_std);
        break;
      }
      case NORM: {
        CDCOUT(sc_time_stamp()  << name() << " case NORM" << endl, kDebugLevel);
        NVUINT3   memory_index = gbcontrol_config.memory_index_1;
        NVUINT8   vector_index = gbcontrol_config.GetVectorIndex();
        NVUINT16  timestep_index = gbcontrol_config.GetTimestepIndex();
        //spec::AdpfloatBiasType adpbias_enc = gbcontrol_config.adpbias_1;
        //out_data = 0;
        //spec::ActVectorType negmean_vector, inv_std_vector; 
        
        #pragma hls_unroll
        for (int i = 0; i < spec::kVectorSize; i++) {
          negmean_vector[i] = -mean; // minus mean
          inv_std_vector[i] = inv_std;
        }
        
        // Send Req
        spec::GB::Large::DataReq large_req_reg;
        large_req_reg.is_write = 0;
        large_req_reg.memory_index = memory_index;
        large_req_reg.vector_index = vector_index;
        large_req_reg.timestep_index = timestep_index;
        large_req.Push(large_req_reg);
        break;
      }
      case NORM2: {  
        CDCOUT(sc_time_stamp()  << name() << " case NORM2" << endl, kDebugLevel);
        // Receive Rsp  
        spec::AdpfloatBiasType adpbias_enc = gbcontrol_config.adpbias_1;   
        out_data = 0;
        //spec::ActVectorType negmean_vector, inv_std_vector;
        spec::GB::Large::DataRsp<1> large_rsp_reg;
        large_rsp_reg = large_rsp.Pop();
       
        spec::ActVectorType x_vector;
        Adpfloat2Fixed(large_rsp_reg.read_vector[0],  x_vector, adpbias_enc);
        
        // Normalize = (X - E[X])*(Inverse Stdev)
        EAdd (x_vector, negmean_vector, x_vector);
        EMul (x_vector, inv_std_vector, x_vector);
        
        // out_data temporary storage
        out_data = x_vector;
        break;
      }
      case GAMMA: {
        CDCOUT(sc_time_stamp()  << name() << " case GAMMA" << endl, kDebugLevel);
        NVUINT8   vector_index = gbcontrol_config.GetVectorIndex();
        //spec::AdpfloatBiasType adpbias_gamma = gbcontrol_config.adpbias_4;
        // Get gamma vector
        spec::GB::Small::DataReq small_req_reg;          
        small_req_reg.is_write = 0;
        small_req_reg.memory_index = gamma_index;   // Use 5 
        small_req_reg.vector_index = vector_index;
        small_req.Push(small_req_reg); 
        break;
      }
      case GAMMA2: { 
        CDCOUT(sc_time_stamp()  << name() << " case GAMMA2" << endl, kDebugLevel);
        spec::AdpfloatBiasType adpbias_gamma = gbcontrol_config.adpbias_4;
        spec::GB::Small::DataRsp small_rsp_reg;
        small_rsp_reg = small_rsp.Pop();        
        
        spec::ActVectorType gamma_vector, vtmp;
        Adpfloat2Fixed(small_rsp_reg.read_data,  gamma_vector, adpbias_gamma);       
      
        // Mul gamma
        EMul (out_data, gamma_vector, vtmp);        
        out_data = vtmp;
        break;
      }
      case BETA: {
        CDCOUT(sc_time_stamp()  << name() << " case BETA" << endl, kDebugLevel);
        //NVUINT3   memory_index = gbcontrol_config.memory_index_1;
        NVUINT8   vector_index = gbcontrol_config.GetVectorIndex();
        //NVUINT16  timestep_index = gbcontrol_config.GetTimestepIndex();
        //spec::AdpfloatBiasType adpbias_beta = gbcontrol_config.adpbias_3;
        // Get Beta vector       
        spec::GB::Small::DataReq small_req_reg;          
        small_req_reg.is_write = 0;
        small_req_reg.memory_index = beta_index;    // Use 6
        small_req_reg.vector_index = vector_index;
        
        small_req.Push(small_req_reg); 
        break;
      } 
      case BETA2: {
        CDCOUT(sc_time_stamp()  << name() << " case BETA2" << endl, kDebugLevel);
        spec::AdpfloatBiasType adpbias_beta = gbcontrol_config.adpbias_3;
        spec::GB::Small::DataRsp small_rsp_reg;
        small_rsp_reg = small_rsp.Pop();              

        spec::ActVectorType beta_vector, vtmp;
        Adpfloat2Fixed(small_rsp_reg.read_data,  beta_vector, adpbias_beta);
       
        // Add Beta
        EAdd (out_data, beta_vector, vtmp);  
        out_data = vtmp;
        break;
     }
     case BETA3: {
        CDCOUT(sc_time_stamp()  << name() << " case BETA3" << endl, kDebugLevel);
        spec::AdpfloatBiasType adpbias_enc = gbcontrol_config.adpbias_1;        
        NVUINT3   memory_index = gbcontrol_config.memory_index_1;
        NVUINT8   vector_index = gbcontrol_config.GetVectorIndex();
        NVUINT16  timestep_index = gbcontrol_config.GetTimestepIndex();
        
        spec::GB::Large::DataReq large_req_reg;
        large_req_reg.is_write = 1;
        large_req_reg.memory_index = memory_index;
        large_req_reg.vector_index = vector_index;
        large_req_reg.timestep_index = timestep_index;
        Fixed2Adpfloat (out_data, large_req_reg.write_data, adpbias_enc);
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
          ResetMeanVar();
          next_state = MEAN;
        }
        else {
          next_state = IDLE;
        }
        break;
      }
      case MEAN: {
        next_state = MEAN2;
        break;
      }
      case MEAN2: { // first data read
        bool is_end = 0;
        gbcontrol_config.UpdateVectorCounter(is_end);
        if (is_end) {
          next_state = VAR;
        }
        else {
          next_state = MEAN;
        }
        break;
      }
      case VAR: {
        next_state = NORM;
        break;        
      }
      case NORM: { // second data read NORM, GAMMA BETA
        next_state = NORM2;
        break;
      }
      case NORM2: { 
        next_state = GAMMA;
        break;
      }
      case GAMMA: {
        next_state = GAMMA2;
        break;
      }
      case GAMMA2: {
        next_state = BETA; 
        break;
      }
      case BETA: {
        next_state = BETA2;
        break;
      }
      case BETA2: {
        next_state = BETA3;
        break;
      }
      case BETA3: { 
        bool is_end = 0;
        gbcontrol_config.UpdateVectorCounter(is_end);
        if (is_end) {
          next_state = NEXT;
        }
        else {
          next_state = NORM;
        }
        break;
      }
      case NEXT: {
        // Reset Temporary state during calculation 
        ResetMeanVar();
        // Move to next timestep
        bool is_end = 0;
        gbcontrol_config.UpdateTimestepCounter(is_end);
        if (is_end) {
          // Pushdone 
          is_start = 0;
          next_state = IDLE;
          CDCOUT(sc_time_stamp()  <<  name() << " LayerNorm Finish" << endl, kDebugLevel);
          done.Push(1);    
        }
        else {
          next_state = MEAN;
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
  

  void LayerNormRun() {
    Reset();

    #pragma hls_pipeline_init_interval 4 
    while(1) {
      Initialize();
      RunFSM();
      if (is_start == 0) {
        DecodeAxi();
        PushAxiRsp();
        CheckStart();
      }
      UpdateFSM();
      //CheckStart();
      wait();
    }
  }
  
};
#endif
