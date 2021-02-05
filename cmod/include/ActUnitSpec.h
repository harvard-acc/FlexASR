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

#ifndef __ACTUNITSPEC__
#define __ACTUNITSPEC__
#include <nvhls_int.h>
#include <nvhls_types.h>
#include <nvhls_assert.h>
#include "SM6Spec.h"

/*** New set of instrunction set and config only for act unit ***/
/*** Main goal is to simplify the PECore thread (previously L1Datapath) operations***/
namespace spec {
  namespace Act {
    typedef VectorType WordType;
    const int kNumReadPorts = 1;
    const int kNumWritePorts = 1;
    const int kNumBanks = 1;
    const int kEntriesPerBank = 32;
    const unsigned int kAddressWidth = nvhls::index_width<kNumBanks * kEntriesPerBank>::val;
    const unsigned int kBankIndexSize = nvhls::index_width<kNumBanks>::val;
    const unsigned int kLocalIndexSize = nvhls::index_width<kEntriesPerBank>::val;
    typedef NVUINTW(kAddressWidth) Address;
    typedef NVUINTW(kAddressWidth+1) AddressPlus1;
    typedef NVUINTW(kBankIndexSize) BankIndex;
    typedef NVUINTW(kLocalIndexSize) LocalIndex;

    const unsigned int kNumInstEntries = 32;
  }
}

/* New version Mini instruction (only tries to support a minimum number of operations)
 OP (4-bit) A2 (2-bit, dest) A1 (1-bit, src)

  OP list
  0: NOP
  1: LOAD:  use output counter to locate (to A2)
  2: STORE: use output counter to locate (from A2)
  3: INPE:  wait data from PE and store  (to A2)
  4: OUTGB: Output to output port        (to A2)
  
  7: COPY: A1 -> A2
  8: EADD: A2+A1 => A2
  9: EMUL: A2*A1 => A2
  A: SIGM: 
  B: TANH:
  C: RELU:
  D: ONEX:
  E: SIGN: (XXX: not support now)
  F: COMP: (XXX: not support now)
*/


class ActConfig {
  static const int write_width = 128;  

 public:
  NVUINT1                 is_valid;
  NVUINT1                 is_zero_first;
  spec::AdpfloatBiasType  adpfloat_bias;
  NVUINT6                 num_inst;
  NVUINT8                 num_output; // maximum is much larger than the required
  spec::Act::Address      buffer_addr_base;
  NVUINT8                 output_addr_base;
  
  NVUINT8                 inst_regs[spec::Act::kNumInstEntries];
  // internal state 
  NVUINT5   inst_counter;
  NVUINT8   output_counter;
  
  
  ActConfig() {  
    Reset();
  }
  
  NVUINT8 InstFetch(){
    return inst_regs[inst_counter];
  }
  
  bool InstIncr() {
    bool is_end = 0;
    if (inst_counter == (num_inst-1)) {
      inst_counter = 0;
      if (output_counter == (num_output-1)) {    
        output_counter = 0;
        is_zero_first = 0; // disactivate is_zero_first
        is_end = 1;
      }
      else {
        output_counter += 1;
      }
    }
    else {
      inst_counter += 1;
    }
    return is_end;
  }

  void Reset() {
    ResetCounter();
    is_valid        = 0;
    is_zero_first   = 0;
    adpfloat_bias   = 0;
    num_inst        = 1;    // should be initialize to 1 to avoid error
    num_output      = 1;    // should be initialize to 1 to avoid error
    buffer_addr_base = 0;
    output_addr_base = 0;

  }
  void ResetCounter(){
    inst_counter    = 0;
    output_counter  = 0;  
  }
  
  
  void ActConfigWrite(const NVUINT8 write_index, const NVUINTW(write_width) write_data) {
    ResetCounter();
    if (write_index == 0x01) {
      is_valid              = nvhls::get_slc<1>(write_data, 0);
      is_zero_first         = nvhls::get_slc<1>(write_data, 8);
      adpfloat_bias         = nvhls::get_slc<spec::kAdpfloatBiasWidth>(write_data, 16);  // spec::kAdpfloatBiasWidth = 4
      num_inst              = nvhls::get_slc<6>(write_data, 24);
      num_output            = nvhls::get_slc<8>(write_data, 32);
      buffer_addr_base      = nvhls::get_slc<spec::Act::kAddressWidth>(write_data, 48);
      output_addr_base      = nvhls::get_slc<8>(write_data, 64);
      
    }
    else if (write_index == 0x02) { // first 16 instructions
      #pragma hls_unroll yes
      for (int i = 0; i < 16; i++) {
        inst_regs[i] = nvhls::get_slc<8>(write_data, 8*i);
      }
    }
    else if (write_index == 0x03) { // second 16 instructions
      #pragma hls_unroll yes
      for (int i = 0; i < 16; i++) {
        inst_regs[i+16] = nvhls::get_slc<8>(write_data, 8*i);
      }
    }
  }
  
  void ActConfigRead(const NVUINT8 read_index, NVUINTW(write_width)& read_data) const {
    read_data = 0;
    if (read_index == 0x01) {
      read_data.set_slc<1>(0, is_valid);
      read_data.set_slc<1>(8, is_zero_first);
      read_data.set_slc<spec::kAdpfloatBiasWidth>(16, adpfloat_bias);
      read_data.set_slc<6>(24, num_inst);
      read_data.set_slc<8>(32, num_output);
      read_data.set_slc<spec::Act::kAddressWidth>(48, buffer_addr_base);
      read_data.set_slc<8>(64, output_addr_base);
      
    }
    else if (read_index == 0x02) { // first 16 instructions
      #pragma hls_unroll yes
      for (int i = 0; i < 16; i++) {
        read_data.set_slc<8>(8*i, inst_regs[i]);
      }
    }
    else if (read_index == 0x03) { // second 16 instructions
      #pragma hls_unroll yes
      for (int i = 0; i < 16; i++) {
        read_data.set_slc<8>(8*i, inst_regs[i+16]);
      }
    }
  }
};



#endif 
