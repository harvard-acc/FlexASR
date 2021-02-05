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

#include "PECore.h"

//#include "BufferSpec.h"
//#include "BufferManager.h"
//#include "ControlSpec.h"

#include "SM6Spec.h"
#include "AxiSpec.h"
#include "AdpfloatSpec.h"
#include "AdpfloatUtils.h"

#include "helper.h"

#include <iostream>
#include <sstream>
#include <iomanip>


#define NVHLS_VERIFY_BLOCKS (PECore)
#include <nvhls_verify.h>

#ifdef COV_ENABLE
   #pragma CTC SKIP
#endif
SC_MODULE(Source) {
  sc_in<bool> clk;
  sc_in<bool> rst;  
  Connections::Out<bool> start;
  Connections::Out<spec::StreamType> input_port; 
  Connections::Out<spec::Axi::SlaveToRVA::Write> rva_in;

        
 
  std::vector<spec::Axi::SlaveToRVA::Write> src_vec;
  
  
  SC_CTOR(Source) {
    SC_THREAD(run);
    sensitive << clk.pos();
    async_reset_signal_is(rst, false);
  }
  
  void run(){
    spec::Axi::SlaveToRVA::Write  rva_in_src;
    
    wait();
    //cout << "size of src_vec: " << src_vec.size() << endl;
    for (unsigned i = 0; i < src_vec.size(); i++) {
      if (src_vec[i].rw == 1) {
        cout << hex << sc_time_stamp() << " Source rva write data " << src_vec[i].data << endl;
      }
    
      rva_in.Push(src_vec[i]);
      wait();
    }
  }
};
SC_MODULE(Dest) {
  sc_in<bool> clk;
  sc_in<bool> rst;
  Connections::In<spec::Axi::SlaveToRVA::Read> rva_out;
  Connections::In<spec::ActVectorType> act_port;

  std::vector<spec::Axi::SlaveToRVA::Read> dest_vec;

  spec::Axi::SlaveToRVA::Read rva_out_dest;

  SC_CTOR(Dest) {
    SC_THREAD(run);
    sensitive << clk.pos();
    async_reset_signal_is(rst, false);

  }
  
  void run(){
    wait();
    
    unsigned i = 0;
    while (1) {
      if (rva_out.PopNB(rva_out_dest)) {
        cout << hex << sc_time_stamp() << " Dest rva data = " << rva_out_dest.data << endl;
        assert(rva_out_dest.data == dest_vec[i].data);
        i++;
      }
      wait();    
    }
  }
};



SC_MODULE(testbench) {
  SC_HAS_PROCESS(testbench);
	sc_clock clk;
  sc_signal<bool> rst;
  
  Connections::Combinational<bool> start;
  Connections::Combinational<spec::StreamType> input_port; 
  Connections::Combinational<spec::Axi::SlaveToRVA::Write> rva_in;
  Connections::Combinational<spec::Axi::SlaveToRVA::Read> rva_out;
  Connections::Combinational<spec::ActVectorType> act_port;
  


  NVHLS_DESIGN(PECore) dut;
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
    dut.start(start);
		dut.input_port(input_port);
		dut.rva_in(rva_in);
		dut.rva_out(rva_out);		
	  dut.act_port(act_port);
    
    source.clk(clk);
    source.rst(rst);
    source.start(start);
	  source.input_port(input_port);
		source.rva_in(rva_in);
			      		
		dest.clk(clk);
		dest.rst(rst);
		dest.rva_out(rva_out);
		dest.act_port(act_port);
		
    testset();
    
    		
    SC_THREAD(run);
  }
  
  void testset() {
    spec::Axi::SlaveToRVA::Write rva_write_tmp;
    spec::Axi::SlaveToRVA::Read  rva_read_tmp;

    std::vector<std::vector<double>>  x_ref, h_ref;
  
    std::vector<double>  bx_ref, bh_ref;
    std::vector<std::vector<double>> Wx_ref, Wh_ref;


    spec::VectorType weight_vec; 
    spec::AdpfloatBiasType adpbias = 2;
    AdpfloatType<8, 3> adpfloat_tmp;
    for (unsigned int i=0; i<spec::kNumVectorLanes; i++) {
        //cout << x_ref[0][i] << " \t " << endl;
        adpfloat_tmp.Reset();
        adpfloat_tmp.set_value(x_ref[0][i], adpbias);
        weight_vec[i] =adpfloat_tmp.to_rawbits();
        adpfloat_tmp.Reset();
        //cout << << endl; 
    }
    //cout << "weight_vec" << weight_vec << endl;
    //PrintVector(weight_vec);

    spec::ScalarType out_test;
    AdpfloatType<8, 3> adpfloat_a;
    adpfloat_a.set_value(-0.07569, adpbias);
    //adpfloat_a.to_rawbits();
    out_test = adpfloat_a.to_rawbits();

    AdpfloatType<8,3> tmp(out_test);
    cout << "check: " << tmp.to_float(2) << endl; 

 
    // AXI write 
    // PE config
    rva_write_tmp.rw = 1;
    rva_write_tmp.data = set_bytes<16>("00_00_00_00_00_00_00_00_00_00_20_02_01_01_01_01");
    rva_write_tmp.addr = set_bytes<3>("40_00_10");  // last 4 bits never used 
    rva_read_tmp.data = rva_write_tmp.data;
    source.src_vec.push_back(rva_write_tmp);
    dest.dest_vec.push_back(rva_read_tmp);    
    
    // Weight SRAM
    cout << "weight sram" << endl;
    rva_write_tmp.rw = 1;
    rva_write_tmp.data = weight_vec.to_rawbits();
    rva_write_tmp.addr = 0x500000 + 1*16 ;
    cout << "rva_write_tmp.addr: " << hex << rva_write_tmp.addr << endl;
    //rva_write_tmp.addr = set_bytes<3>("50_00_10");  // last 4 bits never used 
    rva_read_tmp.data = rva_write_tmp.data;
    source.src_vec.push_back(rva_write_tmp);
    dest.dest_vec.push_back(rva_read_tmp);
    
    // Input/Bias SRAM
    rva_write_tmp.rw = 1;
    rva_write_tmp.data = nvhls::get_rand<32>();
    rva_write_tmp.addr = set_bytes<3>("60_00_20");  // last 4 bits never used 
    rva_read_tmp.data = rva_write_tmp.data;
    source.src_vec.push_back(rva_write_tmp);    
    dest.dest_vec.push_back(rva_read_tmp);    
   
    
    
    // manager 1
    rva_write_tmp.rw = 1;
    rva_write_tmp.data = set_bytes<16>("00_00_00_00_00_00_33_00_22_04_00_0A_04_03_02_01");
    rva_write_tmp.addr = set_bytes<3>("40_00_20"); // last 4 bits never used 
    rva_read_tmp.data = rva_write_tmp.data;
    source.src_vec.push_back(rva_write_tmp);
    dest.dest_vec.push_back(rva_read_tmp);        
    
    // manager 1 cluster
    rva_write_tmp.rw = 1;
    rva_write_tmp.data = nvhls::get_rand<32>();
    rva_write_tmp.addr = set_bytes<3>("40_00_30"); // last 4 bits never used 
    rva_read_tmp.data = rva_write_tmp.data;
    source.src_vec.push_back(rva_write_tmp);
    dest.dest_vec.push_back(rva_read_tmp); 
       
    // manager 2
    rva_write_tmp.rw = 1;
    rva_write_tmp.data = set_bytes<16>("00_00_00_00_00_00_33_00_22_04_00_0A_02_03_04_01");
    rva_write_tmp.addr = set_bytes<3>("40_00_40"); // last 4 bits never used 
    rva_read_tmp.data = rva_write_tmp.data;
    source.src_vec.push_back(rva_write_tmp);
    dest.dest_vec.push_back(rva_read_tmp);    
    
    // manager 2 cluster
    rva_write_tmp.rw = 1;
    rva_write_tmp.data = nvhls::get_rand<32>();
    rva_write_tmp.addr = set_bytes<3>("40_00_50"); // last 4 bits never used 
    rva_read_tmp.data = rva_write_tmp.data;
    source.src_vec.push_back(rva_write_tmp);
    dest.dest_vec.push_back(rva_read_tmp);        
            
    // AXI read
    rva_write_tmp.rw = 0;
    rva_write_tmp.addr = set_bytes<3>("40_00_10");
    source.src_vec.push_back(rva_write_tmp); 

    cout << "weight sram" << endl;  
    rva_write_tmp.rw = 0;
    rva_write_tmp.addr = set_bytes<3>("50_00_10");
    source.src_vec.push_back(rva_write_tmp);    


    rva_write_tmp.rw = 0;
    rva_write_tmp.addr = set_bytes<3>("60_00_20");
    source.src_vec.push_back(rva_write_tmp);    
   
    rva_write_tmp.rw = 0;
    rva_write_tmp.addr = set_bytes<3>("40_00_20");
    source.src_vec.push_back(rva_write_tmp);
               
    rva_write_tmp.rw = 0;
    rva_write_tmp.addr = set_bytes<3>("40_00_30");
    source.src_vec.push_back(rva_write_tmp);    

    rva_write_tmp.rw = 0;
    rva_write_tmp.addr = set_bytes<3>("40_00_40");
    source.src_vec.push_back(rva_write_tmp);

    rva_write_tmp.rw = 0;
    rva_write_tmp.addr = set_bytes<3>("40_00_50");
    source.src_vec.push_back(rva_write_tmp);
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
