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

#ifndef __GBBUFFERSPEC__
#define __GBBUFFERSPEC__

#include <nvhls_int.h>
#include <nvhls_types.h>
#include "AxiSpec.h"
//#include "GBManager/GBManager.h"
//#include "GBSpec.h"
//Defines Configurations specific to Global Buffer partition 
#include "AdpfloatSpec.h"
#include <nvhls_message.h>
namespace spec {
  namespace GB {  
    namespace Large {
      // Parameters for Global Buffer 
      typedef VectorType WordType;
      const unsigned int kNumWritePorts = 1;          // 1 write port 
      const unsigned int kNumReadPorts = 16;          // need at most 16 read ports 
      const unsigned int kNumBanks = 16;            
      const unsigned int kEntriesPerBank = 4096;    // total global buffer size = 4096*8banks*16scalars*8bits = 8Mb = 1MB
      const unsigned int kAddressWidth = nvhls::index_width<kNumBanks * kEntriesPerBank>::val;
      const unsigned int kBankIndexSize = nvhls::index_width<kNumBanks>::val;
      const unsigned int kLocalIndexSize = nvhls::index_width<kEntriesPerBank>::val;
      typedef NVUINTW(kAddressWidth) Address;
      typedef NVUINTW(kAddressWidth+1) AddressPlus1;
      typedef NVUINTW(kBankIndexSize) BankIndex;
      typedef NVUINTW(kLocalIndexSize) LocalIndex;

      const int kMaxNumManagers = 4;
      // Parameters for COnfiguration 
      // const unsigned int kNumInstEntries = 16;
      class DataReq : public nvhls_message{
       public:
        NVUINT1     is_write;
        NVUINT2     memory_index;
        NVUINT8     vector_index;        
        NVUINT16    timestep_index;
        WordType    write_data;        
        
        static const unsigned int width = 1 + 2 + 8 + 16 + WordType::width;
        template <unsigned int Size>
        void Marshall(Marshaller<Size>& m) {
          m & is_write;
          m & memory_index;
          m & timestep_index;
          m & vector_index;        
          m & write_data;
        }
        DataReq() {
          Reset();
        }   
        void Reset() {
          is_write = 0;
          memory_index = 0;
          timestep_index = 0;
          vector_index = 0;
          write_data = 0;
        }
      };     
            
     
      template<unsigned N>
      class DataRsp : public nvhls_message{
       public:
        nvhls::nv_scvector<WordType, N> read_vector;
        
        static const unsigned int width = nvhls::nv_scvector<WordType, N>::width;
        template <unsigned int Size>
        void Marshall(Marshaller<Size>& m) {
          m & read_vector;
        }
        DataRsp() {
          Reset();
        }
        void Reset() {
          read_vector = 0;
        }
      };
     
    };  
    
    namespace Small {
      typedef VectorType WordType;
      const unsigned int kNumWritePorts = 1;         
      const unsigned int kNumReadPorts = 1;        
      const unsigned int kNumBanks = 1;         
      const unsigned int kEntriesPerBank = 1024;   // 16KB, we might need to store multiple set of results    
      const unsigned int kAddressWidth = nvhls::index_width<kNumBanks * kEntriesPerBank>::val;
      const unsigned int kBankIndexSize = nvhls::index_width<kNumBanks>::val;
      const unsigned int kLocalIndexSize = nvhls::index_width<kEntriesPerBank>::val;
      typedef NVUINTW(kAddressWidth) Address;
      typedef NVUINTW(kAddressWidth+1) AddressPlus1;
      typedef NVUINTW(kBankIndexSize) BankIndex;
      typedef NVUINTW(kLocalIndexSize) LocalIndex;    
    
      const int kMaxNumManagers = 8;    
      
      class DataReq : public nvhls_message {
       public:
        NVUINT1     is_write;           // 1: write
        NVUINT3     memory_index;
        NVUINT8     vector_index;        
        WordType    write_data;        
        
        static const unsigned int width = 1 + 3 + 8 + WordType::width;
        template <unsigned int Size>
        void Marshall(Marshaller<Size>& m) {
          m & is_write;
          m & memory_index;
          m & vector_index;        
          m & write_data;
        }
        DataReq() {
          Reset();
        }   
        void Reset() {
          is_write  = 0;
          memory_index = 0;
          vector_index = 0;
          write_data = 0;
        }
      };     
     
      class DataRsp : public nvhls_message {
       public:
        WordType read_data;
        
        static const unsigned int width = WordType::width;
        template <unsigned int Size>
        void Marshall(Marshaller<Size>& m) {
          m & read_data;
        }
        DataRsp() {
          Reset();
        }
        void Reset() {
          read_data = 0;
        }
      };
    }
  }
}

class GBControlConfig {
  static const int write_width = 128;
 public: 
  NVUINT1   is_valid;
  // Control      0: Unidirectional, 1: bi-forward, 2: bi-backward, 3: Decoder
  // LayerReduce  0: MaxPool, 1:MeanPool, 2: LayerAdd
  NVUINT3   mode;         
  NVUINT1   is_rnn;     // used to send collected RNN output back
  NVUINT3   memory_index_1; 
  NVUINT3   memory_index_2;
  NVUINT8   num_vector_1;
  NVUINT8   num_vector_2;
  NVUINT16  num_timestep_1;
  NVUINT16  num_timestep_2;
  spec::AdpfloatBiasType adpbias_1;
  spec::AdpfloatBiasType adpbias_2;
  spec::AdpfloatBiasType adpbias_3;
  spec::AdpfloatBiasType adpbias_4;
      
  
  NVUINT8   vector_counter;
  NVUINT16  timestep_counter;
  
  void Reset() {
    is_valid        = 0;
    mode            = 0;    
    is_rnn          = 0;
    memory_index_1  = 0;
    memory_index_2  = 0;
    num_vector_1    = 1;
    num_vector_2    = 1;    
    num_timestep_1  = 1;
    num_timestep_2  = 1;
    adpbias_1   = 0;
    adpbias_2   = 0;
    adpbias_3   = 0;
    adpbias_4   = 0;
    
    ResetCounter();
  }

  void ConfigWrite(const NVUINT8 write_index, const NVUINTW(write_width)& write_data) {
    if (write_index == 0x01) {
      is_valid      = nvhls::get_slc<1>(write_data, 0);    
      mode          = nvhls::get_slc<3>(write_data, 8);
      is_rnn        = nvhls::get_slc<1>(write_data, 16);
      memory_index_1  = nvhls::get_slc<3>(write_data, 32);
      memory_index_2  = nvhls::get_slc<3>(write_data, 40);
      num_vector_1    = nvhls::get_slc<8>(write_data, 48);
      num_vector_2    = nvhls::get_slc<8>(write_data, 56);
      num_timestep_1  = nvhls::get_slc<16>(write_data, 64);    
      num_timestep_2  = nvhls::get_slc<16>(write_data, 80); 
                   
      adpbias_1       = nvhls::get_slc<spec::kAdpfloatBiasWidth>(write_data, 96);  
      adpbias_2       = nvhls::get_slc<spec::kAdpfloatBiasWidth>(write_data, 104);              
      adpbias_3       = nvhls::get_slc<spec::kAdpfloatBiasWidth>(write_data, 112);        
      adpbias_4       = nvhls::get_slc<spec::kAdpfloatBiasWidth>(write_data, 120);        
    }
  }

  void ConfigRead(const NVUINT8 read_index, NVUINTW(write_width)& read_data) const {
    read_data = 0;
    if (read_index == 0x01) {
      read_data.set_slc<1>(0, is_valid);
      read_data.set_slc<3>(8, mode);
      read_data.set_slc<1>(16, is_rnn);
      read_data.set_slc<3>(32, memory_index_1);
      read_data.set_slc<3>(40, memory_index_2);
      read_data.set_slc<8>(48, num_vector_1);
      read_data.set_slc<8>(56, num_vector_2);
      read_data.set_slc<16>(64, num_timestep_1);    
      read_data.set_slc<16>(80, num_timestep_2);
      
      read_data.set_slc<spec::kAdpfloatBiasWidth>(96, adpbias_1);  
      read_data.set_slc<spec::kAdpfloatBiasWidth>(104, adpbias_2);            
      read_data.set_slc<spec::kAdpfloatBiasWidth>(112, adpbias_3);      
      read_data.set_slc<spec::kAdpfloatBiasWidth>(120, adpbias_4);      
    }
  }


  
  void ResetCounter() {
    vector_counter      = 0;
    timestep_counter    = 0;  
  }

  NVUINT8 GetVectorIndex() const {
    return vector_counter;
  }
  NVUINT16 GetTimestepIndex() const {
    return timestep_counter;
  }
  NVUINT16 GetTimestepIndexGBControl() const {
    NVUINT16 out; 
    switch (mode) {
    case 0: // Unidirectional 
      out = timestep_counter;
      break;
    case 1: // Bi-forward 
      out = timestep_counter << 1;
      break;
    case 2: // Bi-backward
      out = (num_timestep_1 - timestep_counter)*2 - 1;  
      break;
    default: // Decoder does not need timestep
      out = 0;
      break;
    }
    
    return out;
  }
  
  /*NVUINT16 GetTimestepIndexZeroPadding() const {
    return timestep_counter + num_timestep_1;
  }*/
  
  void UpdateVectorCounter(bool& is_end) {
    if (vector_counter >= (num_vector_1 - 1)) {
      is_end = 1;
      vector_counter = 0;
    }
    else {
      vector_counter += 1;
    }
  }
  
  // GBControl
  void UpdateVectorCounter(NVUINT1 sel, bool& is_end) {
    is_end = 0;
    NVUINT8 num_vector_tmp;
    if (sel == 0) {
      num_vector_tmp = num_vector_1;
    }
    else {
      num_vector_tmp = num_vector_2;    
    }
    
    if (vector_counter >= (num_vector_tmp - 1)) {
      is_end = 1;
      vector_counter = 0;
    }
    else {
      vector_counter += 1;
    }
  }  
  
  void UpdateTimestepCounter(bool& is_end) {
    is_end = 0;
    if (timestep_counter >= (num_timestep_1 - 1)) {
      is_end = 1;
      timestep_counter = 0;
    }
    else {
      timestep_counter += 1;
    }
  }
    
  void UpdateTimestepCounterByTwo(bool& is_end) {
    is_end = 0;
    if (timestep_counter >= (num_timestep_1 - 2)) {
      is_end = 1;
      timestep_counter = 0;
    }
    else {
      timestep_counter += 2;
    }
  } 
  
  void UpdateTimestepCounterBySixteen(bool& is_end) {
    is_end = 0;
    if (timestep_counter >= (num_timestep_1 - 16)) {
      is_end = 1;
      timestep_counter = 0;
    }
    else {
      timestep_counter += 16;
    }
  }
  
  
  void UpdateTimestepCounterZeroPadding(bool& is_end) {
    is_end = 0;
    if (timestep_counter >= num_timestep_2 - 1) {
      is_end = 1;
      timestep_counter = 0;
    }
    else {
      timestep_counter += 1;
    }
    
  }    
  
  
};
#endif
