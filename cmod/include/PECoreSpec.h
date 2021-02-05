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

#ifndef __PECORESPEC__
#define __PECORESPEC__

#include <nvhls_int.h>
#include <nvhls_types.h>
#include "AxiSpec.h"
#include "AdpfloatSpec.h"

namespace spec {
  namespace PE {
    namespace Weight {  
      typedef VectorType WordType;
      const int kNumReadPorts = kNumVectorLanes; // spec::kNumVectorLanes = 16
      const int kNumWritePorts = 1;
      const int kNumBanks = kNumVectorLanes;
      const int kEntriesPerBank = 4096;       // need to configure
      const unsigned int kAddressWidth = nvhls::index_width<kNumBanks * kEntriesPerBank>::val;
      const unsigned int kBankIndexSize = nvhls::index_width<kNumBanks>::val;
      const unsigned int kLocalIndexSize = nvhls::index_width<kEntriesPerBank>::val;
      typedef NVUINTW(kAddressWidth) Address;
      typedef NVUINTW(kAddressWidth+1) AddressPlus1;
      typedef NVUINTW(kBankIndexSize) BankIndex;
      typedef NVUINTW(kLocalIndexSize) LocalIndex;
    }
    
    namespace Bias {
      typedef VectorType WordType;
      const int kNumReadPorts = 1; // spec::kNumVectorLanes
      const int kNumWritePorts = 1;
      const int kNumBanks = 1;
      const int kEntriesPerBank = 256;       // need to configure
      const unsigned int kAddressWidth = nvhls::index_width<kNumBanks * kEntriesPerBank>::val;
      const unsigned int kBankIndexSize = nvhls::index_width<kNumBanks>::val;
      const unsigned int kLocalIndexSize = nvhls::index_width<kEntriesPerBank>::val;
      typedef NVUINTW(kAddressWidth) Address;
      typedef NVUINTW(kAddressWidth+1) AddressPlus1;
      typedef NVUINTW(kBankIndexSize) BankIndex;
      typedef NVUINTW(kLocalIndexSize) LocalIndex;
    }
    
    namespace Input {
      typedef VectorType WordType;
      const int kNumReadPorts = 1; // spec::kNumVectorLanes
      const int kNumWritePorts = 1;
      const int kNumBanks = 1;
      const int kEntriesPerBank = 256;     // need to configure
      const unsigned int kAddressWidth = nvhls::index_width<kNumBanks * kEntriesPerBank>::val;
      const unsigned int kBankIndexSize = nvhls::index_width<kNumBanks>::val;
      const unsigned int kLocalIndexSize = nvhls::index_width<kEntriesPerBank>::val;
      typedef NVUINTW(kAddressWidth) Address;
      typedef NVUINTW(kAddressWidth+1) AddressPlus1;
      typedef NVUINTW(kBankIndexSize) BankIndex;
      typedef NVUINTW(kLocalIndexSize) LocalIndex;
    }
    
    const unsigned int kNumPEManagers = 2;
  }
}

// address width must be <= 16
// XXX: follow the format of weight address width (largest)
template <unsigned int kAddressWidth>
class PEManager {
  static const int kDebugLevel = 5;
  static const int write_width = 128;
 public:  
  typedef NVUINTW(kAddressWidth)        Address;
  typedef NVUINTW(kAddressWidth+1)      AddressPlus1;
  
  // FIXME: the naming adp"l"float_bias_weight contain typo
  
  NVUINT1   zero_active;                        // 8 (whether zero output functionality is activate)
  spec::AdpfloatBiasType adplfloat_bias_weight; // 8
  spec::AdpfloatBiasType adplfloat_bias_bias;   // 8
  spec::AdpfloatBiasType adplfloat_bias_input;  // 8
  NVUINT8   num_input;                          // 16
  Address   base_weight;                        // 16
  Address   base_bias;                          // 16
  Address   base_input;                         // 16
  
  
  spec::ClusterType cluster_lut;                // 128
  
  PEManager() {  
    Reset();
  }  
  
  void Reset() {
    zero_active = 0;                      
    adplfloat_bias_weight = 0;
    adplfloat_bias_bias = 0; 
    adplfloat_bias_input = 0; 
    num_input = 1;    // avoid error                           
    base_weight = 0;                       
    base_bias = 0;                         
    base_input = 0;
  }
  
  Address GetWeightAddr(Address input_index, Address output_index, bool is_cluster) const {
    if (is_cluster) { // read 8 banks (hard coded)
      return (output_index*num_input+input_index)*8 + base_weight;
    }
    else {
      return (output_index*num_input+input_index)*16 + base_weight;  
    }
  }
  
  Address GetBiasAddr(Address output_index) const {
    return output_index + base_bias;
  }
  
  Address GetInputAddr(Address input_index) const{
    return input_index + base_input;
  }
  
  void PEManagerWrite(const NVUINTW(write_width)& write_data) {
    zero_active             = nvhls::get_slc<1>(write_data, 0);
    adplfloat_bias_weight   = nvhls::get_slc<spec::kAdpfloatBiasWidth>(write_data, 8);
    adplfloat_bias_bias     = nvhls::get_slc<spec::kAdpfloatBiasWidth>(write_data, 16);
    adplfloat_bias_input    = nvhls::get_slc<spec::kAdpfloatBiasWidth>(write_data, 24);
    num_input               = nvhls::get_slc<8>(write_data, 32);  
    base_weight             = nvhls::get_slc<kAddressWidth>(write_data, 48);  
    base_bias               = nvhls::get_slc<kAddressWidth>(write_data, 64);  
    base_input              = nvhls::get_slc<kAddressWidth>(write_data, 80);  
  }

  void PEManagerRead(NVUINTW(write_width)& read_data) const {
    read_data.set_slc<1>(0, zero_active);
    read_data.set_slc<spec::kAdpfloatBiasWidth>(8, adplfloat_bias_weight);
    read_data.set_slc<spec::kAdpfloatBiasWidth>(16, adplfloat_bias_bias);
    read_data.set_slc<spec::kAdpfloatBiasWidth>(24, adplfloat_bias_input);
    read_data.set_slc<8>(32, num_input);
    read_data.set_slc<kAddressWidth>(48, base_weight);
    read_data.set_slc<kAddressWidth>(64, base_bias);
    read_data.set_slc<kAddressWidth>(80, base_input);
  }

  void ClusterWrite(const NVUINTW(write_width)& write_data) {
    cluster_lut = write_data;
  }

  
  void ClusterRead(NVUINTW(write_width)& read_data) const {
    spec::ClusterType tmp = cluster_lut;
    read_data = tmp.to_rawbits();
  }
  
 
/*** XXX XXX IMPORTANT!!!!! make sure this part (and the ClusterLookup() ) is correctly synthesized ***/
/*** XXX XXX PLEASE also try to figure out if clustering actually can save energy                   ***/
  spec::VectorType ClusterLookup(const spec::HalfVectorType indices) const {
    spec::VectorType out;

    #pragma hls_unroll yes
    for (int i = 0; i < spec::kNumVectorLanes; i++) {
      out[i] = cluster_lut[indices[i]];
    
    }
    return out;
  }
};

class PEConfig {
  static const int write_width = 128;

 public:
  NVUINT1   is_valid;
  //NVUINT1   active_idx;
  NVUINT1   is_zero_first;
  NVUINT1   is_cluster;
  NVUINT1   is_bias;
  NVUINT4   num_manager;      // number of matrix-vector mul (1 or 2)
  NVUINT8   num_output;       // number of output vector per matrix vector mul (For LSTM it should be 4*num_output in act unit) 
  
  // Counters 
 protected:
  NVUINT4   manager_counter;
  NVUINT8   input_counter;
  NVUINT8   output_counter;
 
 public: 
  PEConfig() {  
    Reset();
  }  
  
  NVUINT4 ManagerIndex() const {
    return manager_counter;
  }
  
  NVUINT8 InputIndex() const {
    return input_counter;
  }  
  
  NVUINT8 OutputIndex() const {
    return output_counter;
  }  
  
  void Reset() {
    is_valid      = 0;
    //active_idx    = 1;    // preload on double_buffer[0], and after preload (please write this to 0)
    is_zero_first = 0;
    is_cluster    = 0;
    is_bias       = 0;
    num_manager    = 1;   // should be initialize to 1 to avoid error
    num_output    = 1;    // should be initialize to 1 to avoid error
    
    ResetCounter();
  }
  
  void ResetCounter() {
    manager_counter = 0;
    input_counter  = 0;
    output_counter = 0;  
  }
  
  // note that since num_input is in PEManager, needs a const parameter input
  
  
  // Used After a Datapath (16*16 matrix X 16*1 vector) operation  
  // void UpdateInputCounter(const NVUINT16 num_input, bool& is_input_end, bool& is_output_end) {
  void UpdateInputCounter(const NVUINT16 num_input, bool& is_input_end) {
    is_input_end = 0;
    //is_output_end = 0;
    // 1. update input counter
    if (input_counter == (num_input - 1)) {
      // ready to add bias and move to next row_vector
      input_counter = 0;
      is_input_end  = 1;
      //UpdateManagerCounter(is_output_end);
    }
    else {
      input_counter += 1;
    }
  }
  
  // used after bias appending (a vector row of mul is done)
  void UpdateManagerCounter(bool& is_output_end) {
    is_output_end = 0;
    // 2. update manager counter
    if (manager_counter == (num_manager - 1)) {
        manager_counter = 0;
      // 3. update output counter
      if (output_counter == (num_output - 1)) {
        output_counter = 0;
        // ready for next timestep
        is_zero_first = 0;
        is_output_end = 1;
      }
      else {
        output_counter += 1;
      }
    }
    else {
      manager_counter += 1;
    }
  }
  
  void PEConfigWrite(const NVUINTW(write_width)& write_data) {
    ResetCounter();
    is_valid              = nvhls::get_slc<1>(write_data, 0);
    is_zero_first         = nvhls::get_slc<1>(write_data, 8);
    //active_idx            = nvhls::get_slc<1>(write_data, 16);
    is_cluster            = nvhls::get_slc<1>(write_data, 16);
    is_bias               = nvhls::get_slc<1>(write_data, 24);
    num_manager           = nvhls::get_slc<4>(write_data, 32);
    num_output            = nvhls::get_slc<8>(write_data, 40);
  }

  void PEConfigRead(NVUINTW(write_width)& read_data) const {
    read_data = 0;
    read_data.set_slc<1>(0, is_valid);
    read_data.set_slc<1>(8, is_zero_first);
    //read_data.set_slc<1>(16, active_idx);    
    read_data.set_slc<1>(16, is_cluster);
    read_data.set_slc<1>(24, is_bias);
    read_data.set_slc<4>(32, num_manager);
    read_data.set_slc<8>(40, num_output); 
  }
};

#endif
