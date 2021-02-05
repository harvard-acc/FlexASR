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
#include "GBControl.h"

#include <iostream>
#include <sstream>
#include <iomanip>


#define NVHLS_VERIFY_BLOCKS (GBControl)
#include <nvhls_verify.h>
#ifdef COV_ENABLE
   #pragma CTC SKIP
#endif
SC_MODULE(Source) {
  sc_in<bool> clk;
  sc_in<bool> rst;  
  Connections::Out<spec::Axi::SlaveToRVA::Write> rva_in;
  
  Connections::Out<bool> start;
  Connections::Out<spec::GB::Large::DataRsp<1>>    large_rsp;   
  Connections::Out<spec::GB::Small::DataRsp>   small_rsp;  
  Connections::Out<spec::StreamType>  data_in;
  Connections::Out<bool> pe_done;

  std::vector<spec::Axi::SlaveToRVA::Write> src_vec;
  bool start_src; 
  bool pe_done_src;
  spec::GB::Large::DataRsp<1> large_rsp_src;
  spec::StreamType data_in_src;
  
  SC_CTOR(Source) {
    SC_THREAD(run);
    sensitive << clk.pos();
    async_reset_signal_is(rst, false);

  }
  
  void run(){
    spec::Axi::SlaveToRVA::Write  rva_in_src; 
    rva_in_src.rw = 1;
    rva_in_src.data = set_bytes<16>("01_00_00_02_00_00_00_02_02_02_00_00_00_01_00_01"); //is_valid=1, mode=0, sendback=1, memory_index=1, output_memory_index=0, num_vector= 2, num_output_vector=2, num_timestep=2, num_timestep_padding=0, adpbias_act=2, adpbias_atten=0, adpbias_beta = 0, adpbias_gamma=1
    rva_in_src.addr = set_bytes<3>("70_00_10");  // last 4 bits never used 
    rva_in.Push(rva_in_src);
    wait();

    start_src = 1;
    start.Push(start_src);
    wait(4);

    large_rsp_src.read_vector[0] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_01_00_00_00_01");
    large_rsp.Push(large_rsp_src);
    wait(4);

    large_rsp_src.read_vector[0] = set_bytes<16>("00_00_00_02_00_00_00_B0_00_11_00_C1_00_00_00_03");
    large_rsp.Push(large_rsp_src);
    wait(4);

    data_in_src.logical_addr = 0;
    data_in_src.data = set_bytes<16>("00_00_00_02_00_00_00_B0_00_11_00_D1_00_00_11_0F");
    data_in.Push(data_in_src);
    wait(4);

    data_in_src.logical_addr = 0;
    data_in_src.data = set_bytes<16>("00_00_00_02_00_00_00_B0_00_11_00_D2_00_00_11_00");
    data_in.Push(data_in_src);
    wait(4);

    pe_done.Push(1);
    wait(100);

    //2nd timesteps
    cout << sc_time_stamp() << "check for 2nd timestep" << endl;
    large_rsp_src.read_vector[0] = set_bytes<16>("00_00_00_00_00_00_00_01_00_00_00_00_00_00_00_01");
    large_rsp.Push(large_rsp_src);
    wait(4);

    large_rsp_src.read_vector[0] = set_bytes<16>("00_00_00_00_00_00_00_01_00_00_00_00_00_00_00_03");
    large_rsp.Push(large_rsp_src);
    wait(4);

    large_rsp_src.read_vector[0] = set_bytes<16>("00_00_00_00_00_00_00_01_00_00_00_00_00_00_00_03");
    large_rsp.Push(large_rsp_src);
    wait(4);

    large_rsp_src.read_vector[0] = set_bytes<16>("00_00_00_00_00_00_00_01_00_00_00_00_00_00_00_03");
    large_rsp.Push(large_rsp_src);
    wait(4);

    data_in_src.logical_addr = 0;
    data_in_src.data = set_bytes<16>("00_00_00_02_00_00_00_B0_00_11_00_D1_00_00_11_0F");
    data_in.Push(data_in_src);
    wait(4);

    data_in_src.logical_addr = 0;
    data_in_src.data = set_bytes<16>("00_00_00_02_00_00_00_B0_00_11_00_D2_00_00_11_00");
    data_in.Push(data_in_src);
    wait(4);

    pe_done.Push(1); 
    wait(100); 

    large_rsp_src.read_vector[0] = set_bytes<16>("00_00_00_00_00_00_00_01_00_00_00_00_00_00_00_01");
    large_rsp.Push(large_rsp_src);
    wait(4);

    large_rsp_src.read_vector[0] = set_bytes<16>("00_00_00_00_00_00_00_01_00_00_00_00_00_00_00_03");
    large_rsp.Push(large_rsp_src);
    wait(4); 
    // Test AXI
   /* for (unsigned i = 0; i < src_vec.size(); i++) {
      if (src_vec[i].rw == 1) {
        cout << hex << sc_time_stamp() << " Source rva write data " << src_vec[i].data << endl;
      }
      rva_in.Push(src_vec[i]);
      wait();
    } */
  }
};

SC_MODULE(Dest) {
  sc_in<bool> clk;
  sc_in<bool> rst;
  Connections::In<spec::Axi::SlaveToRVA::Read> rva_out;
  Connections::In<bool> done;
  Connections::In<spec::GB::Large::DataReq>      large_req;
  Connections::In<spec::GB::Small::DataReq>  small_req;  
  Connections::In<spec::StreamType> data_out;
  Connections::In<bool> pe_start;
  
  
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
      spec::StreamType data_out_dest;
      bool pe_start_dest;
      bool done_dest;
      spec::GB::Large::DataReq large_req_dest;

      if (large_req.PopNB(large_req_dest)) {
         cout << sc_time_stamp() << "large buffer request sent: " << " -large buffer request wr: " << large_req_dest.is_write << " - mem index: " << large_req_dest.memory_index << " - vector index: " << large_req_dest.vector_index << " - timestep index: " << large_req_dest.timestep_index << endl;
      }

      if (rva_out.PopNB(rva_out_dest)) {
        cout << hex << sc_time_stamp() << " Dest rva data = " << rva_out_dest.data << endl;
        //assert(rva_out_dest.data == dest_vec[i].data);
        //i++;
      }
      if (data_out.PopNB(data_out_dest)) {
        //cout << hex << sc_time_stamp() << " data_out data = " << data_out_dest.data << endl;
        cout << sc_time_stamp() << " Design data_out result" << " \t " << endl;
        for (int i = 0; i < spec::kNumVectorLanes; i++) {
          AdpfloatType<8,3> tmp(data_out_dest.data[i]);
          cout << tmp.to_float(2) << endl; //XXX check adativefloat bias value 
        }
      }
      if (pe_start.PopNB(pe_start_dest)) {
        cout << sc_time_stamp() << " PE_start signal issued!!!" << endl;
      }
      if (done.PopNB(done_dest)) {
        cout << sc_time_stamp() << " GBControl TB done !!!" << endl;
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
  Connections::Combinational<spec::GB::Large::DataRsp<1>>    large_rsp;  

  Connections::Combinational<spec::GB::Small::DataReq>  small_req;
  Connections::Combinational<spec::GB::Small::DataRsp>   small_rsp;   
  
  Connections::Combinational<spec::StreamType> data_out;
  Connections::Combinational<spec::StreamType>  data_in;  
  
  Connections::Combinational<bool> pe_start;
  Connections::Combinational<bool> pe_done;

  NVHLS_DESIGN(GBControl) dut;
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
    dut.data_out(data_out);
    dut.data_in(data_in);
    dut.pe_start(pe_start);
    dut.pe_done(pe_done);
    
    source.clk(clk);
    source.rst(rst);
		source.rva_in(rva_in);
		source.start(start);
		source.large_rsp(large_rsp);
		source.small_rsp(small_rsp);
		source.data_in(data_in);
		source.pe_done(pe_done);
			      		
		dest.clk(clk);
		dest.rst(rst);
		dest.rva_out(rva_out);
	  dest.done(done);
	  dest.large_req(large_req);
	  dest.small_req(small_req);
	  dest.data_out(data_out);
	  dest.pe_start(pe_start);
    //testset();
    		
    SC_THREAD(run);
  }
  
/*  void testset() {
    spec::Axi::SlaveToRVA::Write rva_write_tmp;
    spec::Axi::SlaveToRVA::Read  rva_read_tmp;    
    
    // AXI write 
    
    // PE config

    // Congig write
    rva_write_tmp.rw = 1;
    rva_write_tmp.data = set_bytes<16>("01_00_00_02_00_00_01_40_00_10_00_00_00_00_00_01"); //is_valid=1, mode=0, sendback=0, memory_index=0, output_memory_index=0, num_vector= 16, num_output_vector=0, num_timestep=320, num_timestep_padding=0, adpbias_act=2, adpbias_atten=0, adpbias_beta = 0, adpbias_gamma=1
    rva_write_tmp.addr = set_bytes<3>("70_00_10");  // last 4 bits never used 
    rva_read_tmp.data = rva_write_tmp.data; 
    source.src_vec.push_back(rva_write_tmp);
    dest.dest_vec.push_back(rva_read_tmp);    
    
    // AXI read
    rva_write_tmp.rw = 0;
    rva_write_tmp.addr = set_bytes<3>("70_00_10");
    source.src_vec.push_back(rva_write_tmp); 
     
  } */
  
  void run(){
	  wait(2, SC_NS );
    std::cout << "@" << sc_time_stamp() <<" Asserting reset" << std::endl;
    rst.write(false);
    wait(2, SC_NS );
    rst.write(true);
    std::cout << "@" << sc_time_stamp() <<" De-Asserting reset" << std::endl;
    wait(1000, SC_NS );
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


