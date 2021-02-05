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

#include <systemc.h>
#include <mc_scverify.h>
#include <testbench/nvhls_rand.h>
#include <nvhls_connections.h>
#include <map>
#include <vector>
#include <deque>
#include <utility>
#include <sstream>
#include <string>
#include <cstdlib>
#include <math.h> // testbench only
#include <queue>

#include "SM6Spec.h"
#include "AxiSpec.h"
#include "AdpfloatSpec.h"
#include "AdpfloatUtils.h"

#include "helper.h"
#include "GBSpec.h"
#include "Attention.h"

#include <iostream>
#include <sstream>
#include <iomanip>


#define NVHLS_VERIFY_BLOCKS (Attention)
#include <nvhls_verify.h>
#ifdef COV_ENABLE
   #pragma CTC SKIP
#endif

SC_MODULE(Source) {
  sc_in<bool> clk;
  sc_in<bool> rst;  
  Connections::Out<spec::Axi::SlaveToRVA::Write> rva_in;
  
  Connections::Out<bool> start;
  Connections::Out<spec::GB::Large::DataRsp<16>>    large_rsp;
  Connections::Out<spec::GB::Small::DataRsp>       small_rsp;
  std::vector<spec::Axi::SlaveToRVA::Write> src_vec;
  bool start_src; 
  
  spec::GB::Large::DataRsp<16> large_rsp_src;
  spec::GB::Small::DataRsp small_rsp_src;
  
  SC_CTOR(Source) {
    SC_THREAD(run);
    sensitive << clk.pos();
    async_reset_signal_is(rst, false);

  }

  void run(){
    spec::Axi::SlaveToRVA::Write  rva_in_src; 
    rva_in_src.rw = 1;
  //rva_in_src.data = set_bytes<16>("00_02_04_02_00_00_00_60_00_10_00_00_00_00_00_01");
    rva_in_src.data = set_bytes<16>("00_02_04_02_00_00_00_11_00_02_00_00_00_00_00_01"); //is_valid=1, mode=0, is_rnn=0, memory_index1=0, memory_index2=0, num_vector= 2, num_output_vector=0, num_timestep=16, num_timestep_padding=00, adpbias_1=2, adpbias_2=4, adpbias_3 = 2, adpbias_4=0 
    rva_in_src.addr = set_bytes<3>("B0_00_10");  // last 4 bits never used 
    rva_in.Push(rva_in_src);
    wait();

    start_src = 1;
    start.Push(start_src);
    wait(4);

    large_rsp_src.read_vector[0] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[1] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[2] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[3] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[4] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[5] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[6] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_40_11_01");
    large_rsp_src.read_vector[7] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[8] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_30_00_01");
    large_rsp_src.read_vector[9] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_20_00_01");
    large_rsp_src.read_vector[10] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[11] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[12] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[13] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[14] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[15] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp.Push(large_rsp_src);
    wait(4);

    small_rsp_src.read_data = set_bytes<16>("00_00_00_02_FF_00_00_A0_00_10_00_01_CC_00_00_01");
    small_rsp.Push(small_rsp_src);
    wait(4);

    large_rsp_src.read_vector[0] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[1] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[2] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[3] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[4] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[5] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[6] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_40_11_01");
    large_rsp_src.read_vector[7] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[8] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_30_00_01");
    large_rsp_src.read_vector[9] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_20_00_01");
    large_rsp_src.read_vector[10] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[11] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[12] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[13] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[14] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[15] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp.Push(large_rsp_src);
    wait(4);

    small_rsp_src.read_data = set_bytes<16>("00_00_00_02_FF_00_00_A0_00_10_00_01_CC_00_00_01");
    small_rsp.Push(small_rsp_src);
    wait(4);

    large_rsp_src.read_vector[0] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[1] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[2] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[3] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[4] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[5] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[6] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_40_11_01");
    large_rsp_src.read_vector[7] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[8] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_30_00_01");
    large_rsp_src.read_vector[9] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_20_00_01");
    large_rsp_src.read_vector[10] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[11] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[12] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[13] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[14] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[15] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp.Push(large_rsp_src);
    wait(4);

    small_rsp_src.read_data = set_bytes<16>("00_00_00_02_FF_00_00_A0_00_10_00_01_CC_00_00_01");
    small_rsp.Push(small_rsp_src);
    wait(4);

    large_rsp_src.read_vector[0] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[1] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[2] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[3] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[4] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[5] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[6] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_40_11_01");
    large_rsp_src.read_vector[7] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[8] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_30_00_01");
    large_rsp_src.read_vector[9] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_20_00_01");
    large_rsp_src.read_vector[10] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[11] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[12] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[13] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[14] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[15] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp.Push(large_rsp_src);
    wait(4);

    small_rsp_src.read_data = set_bytes<16>("00_00_00_02_FF_00_00_A0_00_10_00_01_CC_00_00_01");
    small_rsp.Push(small_rsp_src);
    wait(4);
 
    small_rsp_src.read_data = set_bytes<16>("00_00_00_02_FF_00_00_A0_00_10_00_01_CC_00_00_01");
    small_rsp.Push(small_rsp_src);
    wait(4);  

    small_rsp_src.read_data = set_bytes<16>("00_00_00_02_FF_00_00_A0_00_10_00_01_CC_00_00_01");
    small_rsp.Push(small_rsp_src);
    wait(4);   

    small_rsp_src.read_data = set_bytes<16>("00_00_00_02_FF_00_00_A0_00_10_00_01_CC_00_00_01");
    small_rsp.Push(small_rsp_src);
    wait(4);    

    small_rsp_src.read_data = set_bytes<16>("00_00_00_02_FF_00_00_A0_00_10_00_01_CC_00_00_01");
    small_rsp.Push(small_rsp_src);
    wait(4);  

    small_rsp_src.read_data = set_bytes<16>("00_00_00_02_FF_00_00_A0_00_10_00_01_CC_00_00_01");
    small_rsp.Push(small_rsp_src);
    wait(4);    

    small_rsp_src.read_data = set_bytes<16>("00_00_00_02_FF_00_00_A0_00_10_00_01_CC_00_00_01");
    small_rsp.Push(small_rsp_src);
    wait(4);  

    small_rsp_src.read_data = set_bytes<16>("00_00_00_02_FF_00_00_A0_00_10_00_01_CC_00_00_01");
    small_rsp.Push(small_rsp_src);
    wait(4);  

    small_rsp_src.read_data = set_bytes<16>("00_00_00_02_FF_00_00_A0_00_10_00_01_CC_00_00_01");
    small_rsp.Push(small_rsp_src);
    wait(4); 

    small_rsp_src.read_data = set_bytes<16>("00_00_00_02_FF_00_00_A0_00_10_00_01_CC_00_00_01");
    small_rsp.Push(small_rsp_src);
    wait(4);  

    small_rsp_src.read_data = set_bytes<16>("00_00_00_02_FF_00_00_A0_00_10_00_01_CC_00_00_01");
    small_rsp.Push(small_rsp_src);
    wait(4); 

    small_rsp_src.read_data = set_bytes<16>("00_00_00_02_FF_00_00_A0_00_10_00_01_CC_00_00_01");
    small_rsp.Push(small_rsp_src);
    wait(4); 



    small_rsp_src.read_data = set_bytes<16>("00_00_00_02_FF_00_00_A0_00_10_00_01_CC_00_00_01");
    small_rsp.Push(small_rsp_src);
    wait(4);  

    small_rsp_src.read_data = set_bytes<16>("00_00_00_02_FF_00_00_A0_00_10_00_01_CC_00_00_01");
    small_rsp.Push(small_rsp_src);
    wait(4);  

    small_rsp_src.read_data = set_bytes<16>("00_00_00_02_FF_00_00_A0_00_10_00_01_CC_00_00_01");
    small_rsp.Push(small_rsp_src);
    wait(4); 

    small_rsp_src.read_data = set_bytes<16>("00_00_00_02_FF_00_00_A0_00_10_00_01_CC_00_00_01");
    small_rsp.Push(small_rsp_src);
    wait(4);  

    small_rsp_src.read_data = set_bytes<16>("00_00_00_02_FF_00_00_A0_00_10_00_01_CC_00_00_01");
    small_rsp.Push(small_rsp_src);
    wait(4); 

    large_rsp_src.read_vector[0] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[1] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[2] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[3] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[4] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[5] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[6] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_40_11_01");
    large_rsp_src.read_vector[7] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[8] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_30_00_01");
    large_rsp_src.read_vector[9] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_20_00_01");
    large_rsp_src.read_vector[10] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[11] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[12] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[13] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[14] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[15] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp.Push(large_rsp_src);
    wait(4);

    small_rsp_src.read_data = set_bytes<16>("00_00_00_02_FF_00_00_A0_00_10_00_01_CC_00_00_01");
    small_rsp.Push(small_rsp_src);
    wait(4);

    large_rsp_src.read_vector[0] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[1] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[2] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[3] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[4] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[5] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[6] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_40_11_01");
    large_rsp_src.read_vector[7] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[8] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_30_00_01");
    large_rsp_src.read_vector[9] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_20_00_01");
    large_rsp_src.read_vector[10] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[11] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[12] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[13] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[14] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[15] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp.Push(large_rsp_src);
    wait(4); 

    small_rsp_src.read_data = set_bytes<16>("00_00_00_02_FF_00_00_A0_00_10_00_01_CC_00_00_01");
    small_rsp.Push(small_rsp_src);
    wait(4);   

    large_rsp_src.read_vector[0] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[1] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[2] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[3] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[4] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[5] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[6] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_40_11_01");
    large_rsp_src.read_vector[7] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[8] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_30_00_01");
    large_rsp_src.read_vector[9] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_20_00_01");
    large_rsp_src.read_vector[10] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[11] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[12] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[13] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[14] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[15] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp.Push(large_rsp_src);
    wait(4); 

    small_rsp_src.read_data = set_bytes<16>("00_00_00_02_FF_00_00_A0_00_10_00_01_CC_00_00_01");
    small_rsp.Push(small_rsp_src);
    wait(4); 

    large_rsp_src.read_vector[0] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[1] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[2] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[3] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[4] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[5] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[6] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_40_11_01");
    large_rsp_src.read_vector[7] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[8] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_30_00_01");
    large_rsp_src.read_vector[9] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_20_00_01");
    large_rsp_src.read_vector[10] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[11] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[12] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[13] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[14] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[15] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp.Push(large_rsp_src);
    wait(4);

    small_rsp_src.read_data = set_bytes<16>("00_00_00_02_FF_00_00_A0_00_10_00_01_CC_00_00_01");
    small_rsp.Push(small_rsp_src);
    wait(4);

  /*  large_rsp_src.read_vector[0] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[1] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[2] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[3] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[4] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[5] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[6] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_40_11_01");
    large_rsp_src.read_vector[7] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[8] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_30_00_01");
    large_rsp_src.read_vector[9] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_20_00_01");
    large_rsp_src.read_vector[10] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[11] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[12] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[13] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[14] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[15] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp.Push(large_rsp_src);
    //cout << " Attention received 2nd large_rsp" << endl;
    wait(4);

    small_rsp_src.read_data = set_bytes<16>("00_00_00_02_FF_00_00_A0_00_10_00_01_CC_00_00_01");
    small_rsp.Push(small_rsp_src);
    //cout << " Attention received 2nd small_rsp" << endl;
    wait(4);

    small_rsp_src.read_data = set_bytes<16>("00_00_00_02_FF_00_00_A0_00_10_00_01_CC_00_00_01");
    small_rsp.Push(small_rsp_src);
    wait(4);

    small_rsp_src.read_data = set_bytes<16>("00_00_00_02_FF_00_00_A0_00_10_00_01_CC_00_00_01");
    small_rsp.Push(small_rsp_src);
    wait(4);
 
    small_rsp_src.read_data = set_bytes<16>("00_00_00_02_FF_00_00_A0_00_10_00_01_CC_00_00_01");
    small_rsp.Push(small_rsp_src);
    wait(4);  

    small_rsp_src.read_data = set_bytes<16>("00_00_00_02_FF_00_00_A0_00_10_00_01_CC_00_00_01");
    small_rsp.Push(small_rsp_src);
    wait(4);   

    small_rsp_src.read_data = set_bytes<16>("00_00_00_02_FF_00_00_A0_00_10_00_01_CC_00_00_01");
    small_rsp.Push(small_rsp_src);
    wait(4);    

    small_rsp_src.read_data = set_bytes<16>("00_00_00_02_FF_00_00_A0_00_10_00_01_CC_00_00_01");
    small_rsp.Push(small_rsp_src);
    wait(4);  

    small_rsp_src.read_data = set_bytes<16>("00_00_00_02_FF_00_00_A0_00_10_00_01_CC_00_00_01");
    small_rsp.Push(small_rsp_src);
    wait(4);    

    small_rsp_src.read_data = set_bytes<16>("00_00_00_02_FF_00_00_A0_00_10_00_01_CC_00_00_01");
    small_rsp.Push(small_rsp_src);
    wait(4);  

    large_rsp_src.read_vector[0] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[1] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[2] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[3] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[4] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[5] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[6] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_40_11_01");
    large_rsp_src.read_vector[7] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[8] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_30_00_01");
    large_rsp_src.read_vector[9] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_20_00_01");
    large_rsp_src.read_vector[10] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[11] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[12] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[13] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[14] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[15] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp.Push(large_rsp_src);
    wait(4);

    small_rsp_src.read_data = set_bytes<16>("00_00_00_02_FF_00_00_A0_00_10_00_01_CC_00_00_01");
    small_rsp.Push(small_rsp_src);
    wait(4);

    large_rsp_src.read_vector[0] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[1] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[2] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[3] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[4] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[5] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[6] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_40_11_01");
    large_rsp_src.read_vector[7] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[8] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_30_00_01");
    large_rsp_src.read_vector[9] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_20_00_01");
    large_rsp_src.read_vector[10] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[11] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[12] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[13] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[14] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[15] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp.Push(large_rsp_src);
    wait(4); 

    small_rsp_src.read_data = set_bytes<16>("00_00_00_02_FF_00_00_A0_00_10_00_01_CC_00_00_01");
    small_rsp.Push(small_rsp_src);
    wait(4);   */

    wait();
  }  
};

SC_MODULE(Dest) {
  sc_in<bool> clk;
  sc_in<bool> rst;
  Connections::In<spec::Axi::SlaveToRVA::Read> rva_out;
  Connections::In<bool> done;
  Connections::In<spec::GB::Large::DataReq>      large_req;
  Connections::In<spec::GB::Small::DataReq>      small_req;
  
  std::vector<spec::Axi::SlaveToRVA::Read> dest_vec;


  SC_CTOR(Dest) {
    SC_THREAD(run);
    sensitive << clk.pos();
    async_reset_signal_is(rst, false);
  }
  
  void run(){
    wait();
    
    while (1) {
      spec::Axi::SlaveToRVA::Read rva_out_dest;
      spec::StreamType output_port_dest;
      spec::GB::Large::DataReq large_req_dest;
      spec::GB::Small::DataReq small_req_dest;
      bool done_dest;

      if (large_req.PopNB(large_req_dest)) {
         cout << sc_time_stamp() << " - large buffer request sent: " << " - is_write: "  << large_req_dest.is_write << large_req_dest.memory_index << large_req_dest.vector_index << large_req_dest.timestep_index << endl;
      }
      if (small_req.PopNB(small_req_dest)) {
         cout << sc_time_stamp() << " - small buffer request sent: " << " - is_write: " << small_req_dest.is_write << small_req_dest.memory_index << small_req_dest.vector_index << endl;
      }
      if (rva_out.PopNB(rva_out_dest)) {
        cout << hex << sc_time_stamp() << " Dest rva data = " << rva_out_dest.data << endl;
      }

      if (done.PopNB(done_dest)) {
        cout << hex << sc_time_stamp() << " Done signal issued !!!!" << endl;
      }
      
      wait();    
    }
  }
};



SC_MODULE(testbench) {
  SC_HAS_PROCESS(testbench);
  sc_clock clk;
  sc_signal<bool> rst;
  
  Connections::Combinational<spec::Axi::SlaveToRVA::Write> rva_in;
  Connections::Combinational<spec::Axi::SlaveToRVA::Read> rva_out;
  Connections::Combinational<bool> start;
  Connections::Combinational<bool> done;
 
  Connections::Combinational<spec::GB::Large::DataReq>      large_req;
  Connections::Combinational<spec::GB::Large::DataRsp<16>>    large_rsp;  
  Connections::Combinational<spec::GB::Small::DataReq>      small_req;
  Connections::Combinational<spec::GB::Small::DataRsp>       small_rsp;

  NVHLS_DESIGN(Attention) dut;
  Source  source;
  Dest    dest;
  
  testbench(sc_module_name name)
  : sc_module(name),
    clk("clk", 1.0, SC_NS, 0.5, 0, SC_NS, true),
    rst("rst"),
    dut("dut"),
    source("source"),
    dest("dest")
  {
    dut.clk(clk);
    dut.rst(rst);
    dut.rva_in(rva_in);
    dut.rva_out(rva_out);
    dut.start(start);
    dut.done(done);
    dut.large_req(large_req);
    dut.large_rsp(large_rsp);
    dut.small_req(small_req);
    dut.small_rsp(small_rsp);
    
    source.clk(clk);
    source.rst(rst);
    source.rva_in(rva_in);
    source.start(start);
    source.large_rsp(large_rsp);
    source.small_rsp(small_rsp);
			      		
    dest.clk(clk);
    dest.rst(rst);
    dest.rva_out(rva_out);
    dest.done(done);
    dest.large_req(large_req);
    dest.small_req(small_req);
	  
    SC_THREAD(run);
  }
  
   
  void run(){
	  wait(2, SC_NS );
    std::cout << "@" << sc_time_stamp() <<" Asserting reset" << std::endl;
    rst.write(false);
    wait(2, SC_NS );
    rst.write(true);
    std::cout << "@" << sc_time_stamp() <<" De-Asserting reset" << std::endl;
    wait(10000, SC_NS );
    std::cout << "@" << sc_time_stamp() <<" sc_stop" << std::endl;
    sc_stop();
  }
};


int sc_main(int argc, char *argv[]) {
  nvhls::set_random_seed();
  
  testbench tb("tb");
  
  sc_report_handler::set_actions(SC_ERROR, SC_DISPLAY);
  sc_start();

  bool rc = (sc_report_handler::get_count(SC_ERROR) > 0);
  if (rc)
    DCOUT("TESTBENCH FAIL" << endl);
  else
    DCOUT("TESTBENCH PASS" << endl);
  return rc;
}


