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
#include "GBCore.h"

#include <iostream>
#include <sstream>
#include <iomanip>


#define NVHLS_VERIFY_BLOCKS (GBCore)
#include <nvhls_verify.h>
#ifdef COV_ENABLE
   #pragma CTC SKIP
#endif
/*
SC_MODULE(Source) {
  sc_in<bool> clk;
  sc_in<bool> rst;  
  Connections::Out<spec::Axi::SlaveToRVA::Write> rva_in;
  
  std::vector<spec::Axi::SlaveToRVA::Write> src_vec;
  
    
  SC_CTOR(Source) {
    SC_THREAD(run);
    sensitive << clk.pos();
    async_reset_signal_is(rst, false);

  }
  
  void run(){
    wait();
    // Test AXI
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


  std::vector<spec::Axi::SlaveToRVA::Read> dest_vec;


  SC_CTOR(Dest) {
    SC_THREAD(run);
    sensitive << clk.pos();
    async_reset_signal_is(rst, false);
  }
  
  void run(){
    wait();
    
    unsigned i = 0;
    while (1) {
      spec::Axi::SlaveToRVA::Read rva_out_dest;
      spec::StreamType output_port_dest;
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
  
  Connections::Combinational<spec::Axi::SlaveToRVA::Write> rva_;
  Connections::Combinational<spec::Axi::SlaveToRVA::Read> rva_out;


  NVHLS_DESIGN(GBCore) dut;
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
    
    source.clk(clk);
    source.rst(rst);
		source.rva_in(rva_in);
			      		
		dest.clk(clk);
		dest.rst(rst);
		dest.rva_out(rva_out);
				
    testset();
    		
    SC_THREAD(run);
  }
  
  void testset() {
    spec::Axi::SlaveToRVA::Write rva_write_tmp;
    spec::Axi::SlaveToRVA::Read  rva_read_tmp;    
    
    // AXI write 
    
    // PE config
    // 
    //  is_valid              = 1     nvhls::get_slc<1>(write_data, 0);
    //  adpfloat_bias_weight  = 3     nvhls::get_slc<spec::kAdpfloatBiasWidth>(write_data, 8);
    //  adpfloat_bias_input   = 3     nvhls::get_slc<spec::kAdpfloatBiasWidth>(write_data, 16);
    //  adpfloat_bias_soft    = 7     nvhls::get_slc<spec::kAdpfloatBiasWidth>(write_data, 24);      
    //  num_vcol              = 03    nvhls::get_slc<8>(write_data, 32);
    //  num_vrow              = 04   (50) nvhls::get_slc<16>(write_data, 40);
    //  base_weight           = 0     nvhls::get_slc<16>(write_data, 48);
    //  base_input            = 0     nvhls::get_slc<16>(write_data, 64);    
    //  base_soft             = 40     nvhls::get_slc<16>(write_data, 80);      
   

    rva_write_tmp.rw = 1;
    rva_write_tmp.data = set_bytes<16>("00_00_00_00_00_00_00_00_00_30_00_20_00_20_00_10");
    rva_write_tmp.addr = set_bytes<3>("40_00_10");  // last 4 bits never used 
    rva_read_tmp.data = rva_write_tmp.data; 
    source.src_vec.push_back(rva_write_tmp);
    dest.dest_vec.push_back(rva_read_tmp);    

    rva_write_tmp.rw = 1;
    rva_write_tmp.data = set_bytes<16>("00_00_00_13_12_11_10_09_08_07_06_05_04_03_02_01");
    rva_write_tmp.addr = set_bytes<3>("40_00_20");  
    rva_read_tmp.data = rva_write_tmp.data; 
    source.src_vec.push_back(rva_write_tmp);
    dest.dest_vec.push_back(rva_read_tmp);    
        
    
    
    // Large SRAM
    rva_write_tmp.rw = 1;
    rva_write_tmp.data = nvhls::get_rand<32>();
    rva_write_tmp.addr = set_bytes<3>("50_00_10"); 
    rva_read_tmp.data = rva_write_tmp.data;
    source.src_vec.push_back(rva_write_tmp);
    dest.dest_vec.push_back(rva_read_tmp);

    // Small SRAM
    rva_write_tmp.rw = 1;
    rva_write_tmp.data = nvhls::get_rand<32>();
    rva_write_tmp.addr = set_bytes<3>("60_00_30"); 
    rva_read_tmp.data = rva_write_tmp.data;
    source.src_vec.push_back(rva_write_tmp);
    dest.dest_vec.push_back(rva_read_tmp);        
    
    
    // AXI read
     
    rva_write_tmp.rw = 0;
    rva_write_tmp.addr = set_bytes<3>("40_00_20");
    source.src_vec.push_back(rva_write_tmp); 

    rva_write_tmp.rw = 0;
    rva_write_tmp.addr = set_bytes<3>("40_00_30");
    source.src_vec.push_back(rva_write_tmp); 
            
       
    rva_write_tmp.rw = 0;
    rva_write_tmp.addr = set_bytes<3>("50_00_10");
    source.src_vec.push_back(rva_write_tmp);    

    rva_write_tmp.rw = 0;
    rva_write_tmp.addr = set_bytes<3>("60_00_30");
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
*/

int sc_main(int argc, char *argv[]) {
  nvhls::set_random_seed();
  
  //testbench tb("tb");
  
  sc_report_handler::set_actions(SC_ERROR, SC_DISPLAY);
  sc_start();

  bool rc = (sc_report_handler::get_count(SC_ERROR) > 0);
  if (rc)
    DCOUT("TESTBENCH FAIL" << endl);
  else
    DCOUT("TESTBENCH PASS" << endl);
  return rc;
}


