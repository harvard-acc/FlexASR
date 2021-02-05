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

#ifndef __ATTENTION__
#define __ATTENTION__

#include <systemc.h>
#include <nvhls_int.h>
#include <nvhls_types.h>
#include <nvhls_vector.h>
#include <nvhls_module.h>
#include "SM6Spec.h"
#include "AxiSpec.h"
#include "GBSpec.h"
#include "PEPartition/PEModule/PECore/Datapath/Datapath.h" 
 
void Exponential (const spec::AttentionVectorType in, spec::AttentionVectorType& out) {
  spec::AttentionVectorType out_tmp;       
  #pragma hls_unroll yes
  for (int i = 0; i < 4; i++) {
    ac_fixed<spec::kAttentionWordWidth, spec::kAttentionNumInt, true, AC_TRN, AC_WRAP> in_ac; 
    ac_fixed<spec::kAttentionWordWidth, spec::kAttentionNumInt, false, AC_TRN, AC_WRAP> out_ac;
      
    in_ac.set_slc(0, in[i]);

    out_ac = ac_math::ac_exp_pwl
              <ac_fixed<spec::kAttentionWordWidth, spec::kAttentionNumInt, false, AC_TRN, AC_WRAP> >(in_ac);

    out_tmp[i].set_slc(0, nvhls::get_slc<spec::kAttentionWordWidth>(out_ac, 0));
  }
  out = out_tmp;  
}
  
  
// Now only for Attention.h (division by a scalar)
//void EDiv (const spec::AttentionVectorType in_1, const spec::AttentionVectorType& in_2, spec::AttentionVectorType& out){
void EDiv (const spec::AttentionVectorType in_1, const spec::AttentionVectorType in_2, spec::AttentionVectorType& out){
  spec::AttentionVectorType out_tmp;       
  #pragma hls_unroll yes
  for (int i = 0; i < 4; i++) {
    ac_fixed<spec::kAttentionWordWidth, spec::kAttentionNumInt, true, AC_TRN, AC_WRAP> in_1_ac, in_2_ac; 
    ac_fixed<spec::kAttentionWordWidth, spec::kAttentionNumInt, true, AC_TRN, AC_WRAP> out_ac;  
    
    ac_fixed<6, 2, false, AC_TRN, AC_WRAP> in_1_reduce, in_2_reduce;
    ac_fixed<10, 2, false, AC_TRN, AC_WRAP> out_reduce;
    

    in_1_ac.set_slc(0, in_1[i]);
    in_2_ac.set_slc(0, in_2[i]);
    
    in_1_reduce = in_1_ac;
    in_2_reduce = in_2_ac;
    
    ac_math::ac_div(in_1_reduce, in_2_reduce, out_reduce);

    out_ac = out_reduce;
    out_tmp[i].set_slc(0, nvhls::get_slc<spec::kAttentionWordWidth>(out_ac, 0));
  }  
  out = out_tmp;    
}
  
  
  
class Attention  : public match::Module {
  static const int softmax_index = 7;
  static const int kDebugLevel = 4;
  SC_HAS_PROCESS(Attention);
 public:
  Connections::In<bool> start;
  Connections::Out<bool> done;
  Connections::In<spec::Axi::SlaveToRVA::Write>   rva_in;
  Connections::Out<spec::Axi::SlaveToRVA::Read>   rva_out;  
  Connections::Out<spec::GB::Large::DataReq>      large_req;
  Connections::In<spec::GB::Large::DataRsp<16>>   large_rsp;

  Connections::Out<spec::GB::Small::DataReq>      small_req;
  Connections::In<spec::GB::Small::DataRsp>       small_rsp;
 
  // Constructor
  Attention (sc_module_name nm)
      : match::Module(nm),
        start("start"),
        done("done"),
        rva_in("rva_in"),
        rva_out("rva_out"),
        large_req("large_req"),
        large_rsp("large_rsp"),
        small_req("small_req"),
        small_rsp("small_rsp")       
  {
    SC_THREAD(AttentionRun);
    sensitive << clk.pos();
    async_reset_signal_is(rst, false);
  }  
 
  // A. FSM
  enum FSM {
    IDLE, 
    PRE, 
    BMM,
    BMM1a,
    BMM1b,
    BMM2, 
    NEXT,   // 1st bmm
    OUT,    // 2nd bmm
    SFM1,   // Find Sum[exp]
    SFM1b,
    SFM2,   // exp/sum[exp]
    SFM2b,
    SFM3,   // Write Back
    FIN
  };
  FSM state;    
  
  bool is_start;

  GBControlConfig gbcontrol_config;  
  bool w_axi_rsp;
  spec::Axi::SlaveToRVA::Read rva_out_reg;    
  spec::GB::Large::DataReq large_req_reg;
  spec::GB::Small::DataReq small_req_reg;
  spec::GB::Large::DataRsp<16> large_rsp_reg;
  spec::GB::Small::DataRsp small_rsp_reg;

  NVUINT1 bmm_counter;  
  NVUINT2 softmax_counter; // counter from 0~4
  
  
  spec::AccumVectorType     accum_vector;    
  spec::AttentionScalarType sum_exp, maximum_value;
  spec::VectorType softmax_result;
  
  void Reset() {
    state = IDLE;
    is_start      = 0;
    bmm_counter   = 0;
    softmax_counter  = 0;
    gbcontrol_config.Reset();
    ResetPorts();
    ResetAccum();
    ResetSoftmax();
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
  
  void ResetAccum() {
    accum_vector  = 0;
  }
  
  void ResetSoftmax() {
    sum_exp = 0;
    maximum_value = spec::kAttentionWordMin;
  }
     
  void DecodeAxiWrite(const spec::Axi::SlaveToRVA::Write& rva_in_reg){
    NVUINT4     tmp = nvhls::get_slc<4>(rva_in_reg.addr, 20);
    NVUINT16    local_index = nvhls::get_slc<16>(rva_in_reg.addr, 4);
    
    if (tmp == 0xB) {
      gbcontrol_config.ConfigWrite(local_index, rva_in_reg.data);
    }
  }   
  
  
  void DecodeAxiRead(const spec::Axi::SlaveToRVA::Write& rva_in_reg) {
    NVUINT4 tmp = nvhls::get_slc<4>(rva_in_reg.addr, 20);
    NVUINT16    local_index = nvhls::get_slc<16>(rva_in_reg.addr, 4);
    
    // Set Push Response
    w_axi_rsp = 1;
    if (tmp == 0xB) {
      gbcontrol_config.ConfigRead(local_index, rva_out_reg.data);
    }    
  }  
  
  void Initialize() {
    w_axi_rsp     = 0;
  }

  void CheckStart() {
    bool start_reg;
    if (start.PopNB(start_reg)) {
      is_start = gbcontrol_config.is_valid && start_reg;
      CDCOUT(sc_time_stamp()  << name() << " Attention Start !!!" << endl, kDebugLevel);
    }
  }
  
  void DecodeAxi() {  
    spec::Axi::SlaveToRVA::Write rva_in_reg;
    if (rva_in.PopNB(rva_in_reg)) {
      CDCOUT(sc_time_stamp() << name() << "Attention RVA Pop " << endl, kDebugLevel);
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
  
  // To make it work for the 
  // This function is working on array (passing by pointers)
  void MatrixTranspose(spec::VectorType in_out[spec::kNumVectorLanes]) const {
    spec::VectorType out_tmp[spec::kNumVectorLanes];
    
    #pragma hls_unroll yes
    for (int i = 0; i < spec::kNumVectorLanes; i++) {
      #pragma hls_unroll yes      
      for (int j = 0; j < spec::kNumVectorLanes; j++) {
        out_tmp[i][j] = in_out[j][i];
      }
    }
    #pragma hls_unroll yes
    for (int i = 0; i < 16; i++) {
      in_out[i] = out_tmp[i];
    }
  }

  bool IsVectorZero(spec::VectorType vec) {
    NVUINTW(spec::kVectorSize) is_scalar_zero;
    #pragma hls_unroll yes
    for (int i = 0; i < spec::kVectorSize; i++)
      is_scalar_zero[i] = (vec[i] == 0);
    bool is_vec_zero  = is_scalar_zero.and_reduce();
    return is_vec_zero;
  }
  

  void UpdateMax(const spec::AttentionVectorType attention_vector) {
    spec::AttentionScalarType new_max = spec::kAttentionWordMin;    // should not be a reg

    #pragma hls_unroll yes 
    for (int i=0; i< 4; i++){
      if (attention_vector[i] > new_max) {
        new_max = attention_vector[i]; 
      }
    }
       
    if (new_max > maximum_value) {
      maximum_value = new_max;
    }
  }

  void UpdateSoftmaxCounter(bool & is_end) {
    if (softmax_counter == 3) { 
      is_end = 1;
      softmax_counter = 0;
    }
    else {
      is_end = 0;
      softmax_counter++;    
    }
  }
  
  void RunFSM(){
    // 4 types of biases
    // adpbias_1: bias for matrix 
    // adpbias_2: bias for input of bmm1
    // adpbias_3: bias for softmax output
    // adpbias_4: bias for output   
    spec::AdpfloatBiasType adpbias_matrix  = gbcontrol_config.adpbias_1;
    spec::AdpfloatBiasType adpbias_input   = gbcontrol_config.adpbias_2;
    spec::AdpfloatBiasType adpbias_softmax = gbcontrol_config.adpbias_3; 
    spec::AdpfloatBiasType adpbias_output  = gbcontrol_config.adpbias_4;
  
    switch (state) {
      case IDLE: {
        ResetSoftmax();
        break;
      } 
      case PRE: {
        ResetAccum();
        break;
      }
      case BMM: {
        // Read Small, Large
        // Make sure the cycle behavior is correct, and no deadlock
        NVUINT3   memory_index_encoder = gbcontrol_config.memory_index_1;
        //NVUINT3   memory_index_decoder = gbcontrol_config.memory_index_2;
        //NVUINT3   memory_index_softmax = softmax_index; // 7
        NVUINT8   vector_index = gbcontrol_config.GetVectorIndex();
        NVUINT16  timestep_index = gbcontrol_config.GetTimestepIndex();
        
        // Send Req
        large_req_reg.is_write = 0;
        large_req_reg.memory_index = memory_index_encoder;
        large_req_reg.vector_index = vector_index;
        large_req_reg.timestep_index = timestep_index;
        large_req.Push(large_req_reg);
        break;
      }
      case BMM1a: {
        large_rsp_reg = large_rsp.Pop();
        break;
      }
      case BMM1b: { 
        NVUINT3   memory_index_decoder = gbcontrol_config.memory_index_2;
        NVUINT3   memory_index_softmax = softmax_index; // 7
        NVUINT8   vector_index = gbcontrol_config.GetVectorIndex();
        NVUINT16  timestep_index = gbcontrol_config.GetTimestepIndex();
        small_req_reg.is_write = 0;
        if (bmm_counter == 0) {
          small_req_reg.memory_index = memory_index_decoder;
          small_req_reg.vector_index = vector_index; 
        }
        else {
          small_req_reg.memory_index = memory_index_softmax;
          small_req_reg.vector_index = timestep_index >> 4;     
        }
        small_req.Push(small_req_reg);
        break;
      }
      case BMM2: { 
        //large_rsp_reg = large_rsp.Pop();        
        small_rsp_reg = small_rsp.Pop(); 
        
        spec::VectorType dp_in0[spec::kNumVectorLanes];
        spec::VectorType dp_in1;
        spec::AccumVectorType dp_out;        
        
        #pragma hls_unroll yes
        for (int i = 0; i < 16; i++) {
          dp_in0[i] = large_rsp_reg.read_vector[i];
        }
        dp_in1 = small_rsp_reg.read_data;

      
        // DO transpose a matrix "dp_in0" for 2nd BMM;
        if (bmm_counter == 1) {
          MatrixTranspose(dp_in0);
        }        
        
        // ZERO Skipping 
        bool is_zero = IsVectorZero(dp_in1);
        if (is_zero == 0) {
          Datapath(dp_in0, dp_in1, dp_out);
          #pragma hls_unroll yes
          for (int i = 0; i < spec::kNumVectorLanes; i++) { // spec::kNumVectorLanes = 16
            accum_vector[i] += dp_out[i];
          }
        }
        
        break;
      }      
      case NEXT: {
        NVUINT16 timestep_index = gbcontrol_config.GetTimestepIndex();

        // matrix, input 
        NVINT6 shift_amount = -2*spec::kAdpfloatOffset + 2*spec::kAdpfloatManWidth - spec::kAttentionNumFrac
                              - adpbias_matrix - adpbias_input; 
        
        
        spec::AttentionVectorType attention_vector(nvhls::get_slc<128>(accum_vector.to_rawbits(), 128*softmax_counter));
        
        // XXX Important Note //
        // The output of first BMM is zero for all zeropadded timesteps (we call it them zeropadded outputs)
        // the softmax computation will be affected by above the zeropadded outputs. 
        // We assume that the output of first BMM is extremely likely not to be zero if it is not the zeropadded outputs
        // Therefore, to solve the problem of zeropadded outputs, we could set them to a very high negative value
        
        #pragma hls_unroll yes          
        for (int j = 0; j < 4; j++) {
          // XXX changes based on the above reasoning, we set the zero outputs to -64
          if (attention_vector[j] == 0) {
            attention_vector[j] = -64*(1<<spec::kAttentionNumFrac);
          }
          else {
            attention_vector[j] = attention_vector[j] >> shift_amount;         
          }
        }
        UpdateMax(attention_vector);
          
        // Output follows the timestep_index with basic unit 16 
        spec::VectorType tmp_data(attention_vector.to_rawbits());
          
        small_req_reg.is_write = 1;        
        small_req_reg.memory_index = softmax_index;
        small_req_reg.vector_index = (timestep_index >> 2) + softmax_counter;                                 
        small_req_reg.write_data = tmp_data;
        small_req.Push(small_req_reg);
        
        break;
      }
      case OUT: {
        // Final Output after BMM 2
        NVUINT3   memory_index_decoder = gbcontrol_config.memory_index_2;        
        NVUINT8   vector_index = gbcontrol_config.GetVectorIndex();            
        
        // matrix, softmax
        NVINT6 shift_amount = -2*spec::kAdpfloatOffset + 2*spec::kAdpfloatManWidth - spec::kAttentionNumFrac
                              - adpbias_matrix - adpbias_softmax;
        
        spec::VectorType write_data = 0;
        #pragma hls_unroll yes          
        for (int i = 0; i < 16; i++) {
          NVINT32 tmp_out = accum_vector[i];
          tmp_out = tmp_out >> shift_amount;
          AdpfloatType<8,3> tmp;
          NVINTW(26) reduce = tmp_out;        
          // uses output
          tmp.set_value_fixed<26, spec::kAttentionNumFrac>(reduce, adpbias_output);
          write_data[i] = tmp.to_rawbits();
        }
        
        
        small_req_reg.is_write = 1; 
        small_req_reg.write_data = write_data;
        small_req_reg.memory_index = memory_index_decoder;
        small_req_reg.vector_index = vector_index;
        small_req.Push(small_req_reg);       
        
        break;
      }
      case SFM1: {
        NVUINT16  timestep_index = gbcontrol_config.GetTimestepIndex();
        //spec::AttentionVectorType maximum_vector;
        //spec::AttentionScalarType tmp_sum = 0;
        
        small_req_reg.is_write = 0;        
        small_req_reg.memory_index = softmax_index;        
        small_req_reg.vector_index = (timestep_index >> 2) + softmax_counter;        
        small_req.Push(small_req_reg);                
        break;
      }
      case SFM1b: {
        spec::AttentionScalarType tmp_sum = 0;
        small_rsp_reg = small_rsp.Pop();         
        
        spec::AttentionVectorType exp_vector(small_rsp_reg.read_data.to_rawbits());
                
        #pragma hls_unroll yes
        for (int i = 0; i < 4; i++) {
          exp_vector[i] -= maximum_value;
        }        
        Exponential(exp_vector, exp_vector);
        
        #pragma hls_unroll yes
        for (int i = 0; i < 4; i++) {
          tmp_sum += exp_vector[i];
        }             
        sum_exp += tmp_sum;
        break;
      }
      case SFM2: {
        NVUINT16  timestep_index = gbcontrol_config.GetTimestepIndex();
        //spec::AttentionVectorType maximum_vector;
        //spec::AttentionVectorType sum_exp_vector;
        
        small_req_reg.is_write = 0;        
        small_req_reg.memory_index = softmax_index;        
        small_req_reg.vector_index = (timestep_index >> 2) + softmax_counter;        
        small_req.Push(small_req_reg);                
        break;
      }
      case SFM2b: {
        spec::AttentionVectorType sum_exp_vector; 
        small_rsp_reg = small_rsp.Pop();         
        
        spec::AttentionVectorType exp_vector(small_rsp_reg.read_data.to_rawbits());
                
        #pragma hls_unroll yes
        for (int i = 0; i < 4; i++) {
          exp_vector[i] -= maximum_value;
          sum_exp_vector[i] = sum_exp;
        }        
        Exponential(exp_vector, exp_vector); 
        
                       
        EDiv(exp_vector, sum_exp_vector, exp_vector);
        
        #pragma hls_unroll yes        
        for (int i = 0; i < 4; i++) {
          AdpfloatType<8,3> tmp;
          NVINTW(26) reduce = exp_vector[i];
          // compress softmax output
          tmp.set_value_fixed<26, spec::kAttentionNumFrac>(reduce, adpbias_softmax);
          softmax_result[i + 4*softmax_counter] = tmp.to_rawbits();
        }
        break;
      }
      case SFM3: {
        NVUINT16  timestep_index = gbcontrol_config.GetTimestepIndex();      
        small_req_reg.is_write = 1;        
        small_req_reg.write_data = softmax_result;

        small_req_reg.memory_index = softmax_index;        
        small_req_reg.vector_index = (timestep_index >> 4);        
        small_req.Push(small_req_reg);                
        
        break;
      }      
      case FIN: {
        done.Push(1);
        CDCOUT(sc_time_stamp() << name() << " Attention Finish " << endl, kDebugLevel);
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
        if (is_start) {
          next_state = PRE;
        }
        else {
          next_state = IDLE;
        }
        break;
      } 
      case PRE: {
        next_state = BMM;
        break;
      }
      case BMM: {
        next_state = BMM1a;
        break;
      }
      case BMM1a: {
        next_state = BMM1b;
        break;
      }
      case BMM1b: {
        next_state = BMM2;
        break;
      }
      case BMM2: {
        bool is_end = 0;
        if (bmm_counter == 0) {
          gbcontrol_config.UpdateVectorCounter(is_end);
          if (is_end) {
            next_state = NEXT;
          }
          else {
            next_state = BMM;
          }
        }
        else {
          gbcontrol_config.UpdateTimestepCounterBySixteen(is_end);
          if (is_end) {
            next_state = OUT;
          }
          else {
            next_state = BMM;
          }          
        }
      
        // if first BMM update vector_counter -> NEXT
        // else update timestep_counter by 16 -> OUT
        break;
      }      
      case NEXT: {
        // work only for first BMM
        // softmax_counter
        bool is_end1 = 0, is_end2 = 0;
        UpdateSoftmaxCounter(is_end1);
        if (is_end1) {
          gbcontrol_config.UpdateTimestepCounterBySixteen(is_end2);
          if (is_end2) {
            next_state = SFM1;
            bmm_counter = 1;
          }
          else {
            next_state = PRE;         
          }
        }
        else {
          next_state = NEXT;
        }
        
        // update timestep_counter by 16 (4 writes)
        break;
      }
      case OUT: {
        // work only for second BMM
        // update vector_counter
        // push output 
        bool is_end = 0;
        gbcontrol_config.UpdateVectorCounter(is_end);
        if (is_end) {
          next_state = FIN;
          bmm_counter = 0;
        }
        else {
          next_state = PRE;
        }           
        break;
      }
      case SFM1: {
        next_state = SFM1b;
        break;
      }
      case SFM1b: {
        // FIND sum[exp()]
        // update timestep_counter by 4 (1 read)
        bool is_end1 = 0, is_end2 = 0;
        UpdateSoftmaxCounter(is_end1);
        if (is_end1) {
          gbcontrol_config.UpdateTimestepCounterBySixteen(is_end2);
          if (is_end2) {
            next_state = SFM2;
          }
          else {
            next_state = SFM1;         
          }
        }
        else {
          next_state = SFM1;
        }        
        
        
        break;
      }
      case SFM2: {
        next_state = SFM2b;
        break;
      }
      case SFM2b: {
        // FIND softmax output    
        // update timestep_counter by 16 (4 reads 1 write)        
        bool is_end = 0;
        UpdateSoftmaxCounter(is_end);
        if (is_end) {
          next_state = SFM3;
        }
        else {
          next_state = SFM2;
        }       
        break;
      }
      case SFM3: {
        // Write SFM output to SRAM
        // update timestep_counter by 16 (4 reads 1 write)        
        bool is_end = 0;        
        gbcontrol_config.UpdateTimestepCounterBySixteen(is_end);
        if (is_end) {
          bmm_counter = 1;        
          next_state = PRE;
        }
        else {
          next_state = SFM2;         
        }        
        break;
      }      
      case FIN: {
        is_start = 0;
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
  
  void AttentionRun(){
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
