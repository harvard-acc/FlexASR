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
#include "LayerReduce.h"

#include <iostream>
#include <sstream>
#include <iomanip>


#define NVHLS_VERIFY_BLOCKS (LayerReduce)
#include <nvhls_verify.h>
#ifdef COV_ENABLE
   #pragma CTC SKIP
#endif
SC_MODULE(Source) {
  sc_in<bool> clk;
  sc_in<bool> rst;  
  Connections::Out<spec::Axi::SlaveToRVA::Write> rva_in;
  
  Connections::Out<bool> start;
  Connections::Out<spec::GB::Large::DataRsp<2>>    large_rsp;

  std::vector<spec::Axi::SlaveToRVA::Write> src_vec;
  bool start_src; 
  
  spec::GB::Large::DataRsp<2> large_rsp_src;  
  
  SC_CTOR(Source) {
    SC_THREAD(run);
    sensitive << clk.pos();
    async_reset_signal_is(rst, false);

  }
  
  void run(){
    spec::Axi::SlaveToRVA::Write  rva_in_src; 
    rva_in_src.rw = 1;
    rva_in_src.data = set_bytes<16>("00_00_00_02_00_00_00_01_00_03_00_01_00_00_00_01"); //is_valid=1, mode=0, is_rnn=0, memory_index1=1, memory_index2=0, num_vector= 3, num_output_vector=0, num_timestep=1, num_timestep_padding=00, adpbias_1=2, adpbias_2=0, adpbias_3 = 0, adpbias_4=0
    rva_in_src.addr = set_bytes<3>("80_00_10");  // last 4 bits never used 
    rva_in.Push(rva_in_src);
    wait();

    start_src = 1;
    start.Push(start_src);
    wait(4);

    large_rsp_src.read_vector[0] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[1] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp.Push(large_rsp_src);
    wait(4);

    large_rsp_src.read_vector[0] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[1] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp.Push(large_rsp_src);
    wait(4);

    large_rsp_src.read_vector[0] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp_src.read_vector[1] = set_bytes<16>("00_00_00_02_00_00_00_A0_00_10_00_01_00_00_00_01");
    large_rsp.Push(large_rsp_src);

    wait();
  }
};

SC_MODULE(Dest) {
  sc_in<bool> clk;
  sc_in<bool> rst;
  Connections::In<spec::Axi::SlaveToRVA::Read> rva_out;
  Connections::In<bool> done;
  Connections::In<spec::GB::Large::DataReq>      large_req;
  
  
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
      bool done_dest;

      if (large_req.PopNB(large_req_dest)) {
         cout << "large buffer request sent: " << large_req_dest.memory_index << large_req_dest.vector_index << large_req_dest.timestep_index << endl;
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
  Connections::Combinational<spec::GB::Large::DataRsp<2>>    large_rsp;  

  NVHLS_DESIGN(LayerReduce) dut;
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
    
    source.clk(clk);
    source.rst(rst);
		source.rva_in(rva_in);
		source.start(start);
		source.large_rsp(large_rsp);
			      		
		dest.clk(clk);
		dest.rst(rst);
		dest.rva_out(rva_out);
	  dest.done(done);
	  dest.large_req(large_req);
	  
    //testset();
    		
    SC_THREAD(run);
  }
  
  /*void testset() {
    spec::Axi::SlaveToRVA::Write rva_write_tmp;
    spec::Axi::SlaveToRVA::Read  rva_read_tmp;    
    
    // AXI write 
    
    // PE config

    // Congig write
    rva_write_tmp.rw = 1;
    rva_write_tmp.data = set_bytes<16>("00_00_00_00_00_00_00_00_00_00_00_00_00_00_03_01");
    rva_write_tmp.addr = set_bytes<3>("80_00_10");  // last 4 bits never used 
    rva_read_tmp.data = rva_write_tmp.data; 
    source.src_vec.push_back(rva_write_tmp);
    dest.dest_vec.push_back(rva_read_tmp);    
    
    // AXI read
    rva_write_tmp.rw = 0;
    rva_write_tmp.addr = set_bytes<3>("80_00_10");
    source.src_vec.push_back(rva_write_tmp); 
     
  } */
  
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


