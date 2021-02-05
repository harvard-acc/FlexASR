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

#include "GBModule.h"
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
#include <iomanip>
#include <nvhls_int.h>
#include <nvhls_types.h>
#include <nvhls_vector.h>

#include "SM6Spec.h"
#include "AxiSpec.h"
#include "AdpfloatSpec.h"
#include "AdpfloatUtils.h"

#include "helper.h"

#include "../../testbench/libnpy/npy.hpp"

#define NVHLS_VERIFY_BLOCKS (GBModule)
#include <nvhls_verify.h>


#ifdef COV_ENABLE
   #pragma CTC SKIP
#endif

SC_MODULE(Source) {
  sc_in<bool> clk;
  sc_in<bool> rst;  
  Connections::Out<spec::StreamType> data_in;  
  Connections::Out<bool> pe_done;
  Connections::Out<spec::Axi::SlaveToRVA::Write> rva_in;

  std::vector<spec::Axi::SlaveToRVA::Write> src_vec;
 
  SC_CTOR(Source) {
    SC_THREAD(run);
    sensitive << clk.pos();
    async_reset_signal_is(rst, false);
  }

  void run() {
    spec::Axi::SlaveToRVA::Write  rva_in_src;

    wait();
    for (unsigned i = 0; i < src_vec.size(); i++) {
      if (src_vec[i].rw == 1) {
        cout << hex << sc_time_stamp() << " Source rva write data " << src_vec[i].data << endl;
      }
    
      rva_in.Push(src_vec[i]);
      wait(1);
    }
  } // run()
}; //SC MODULE Source

SC_MODULE(Dest) {
  sc_in<bool> clk;
  sc_in<bool> rst;
  Connections::In<spec::Axi::SlaveToRVA::Read> rva_out;
  Connections::In<spec::StreamType> data_out; 
  Connections::In<bool> done;  
  Connections::In<bool> pe_start;

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
        wait(10);
        cout << hex << sc_time_stamp() << " Dest rva data = " << rva_out_dest.data << endl;
        //assert(rva_out_dest.data == dest_vec[i].data);
        i++;
      }
      wait(1);    
    }
  }

}; //SC MODULE Dest


SC_MODULE(testbench) {
  SC_HAS_PROCESS(testbench);
  sc_clock clk;
  sc_signal<bool> rst;

  Connections::Combinational<spec::StreamType> data_in;     
  Connections::Combinational<bool> pe_start;  
  Connections::Combinational<bool> pe_done;
  Connections::Combinational<spec::Axi::SlaveToRVA::Write> rva_in;
  Connections::Combinational<spec::Axi::SlaveToRVA::Read> rva_out;
  Connections::Combinational<spec::StreamType> data_out; 
  Connections::Combinational<bool> done;  

  NVHLS_DESIGN(GBModule) dut;
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
     dut.data_in(data_in);
     dut.pe_start(pe_start);
     dut.pe_done(pe_done);
     dut.rva_in(rva_in);
     dut.rva_out(rva_out);
     dut.data_out(data_out);
     dut.done(done);

     source.clk(clk);
     source.rst(rst);
     source.data_in(data_in);
     source.pe_done(pe_done);
     source.rva_in(rva_in);

     dest.clk(clk);
     dest.rst(rst);
     dest.rva_out(rva_out);
     dest.data_out(data_out);
     dest.done(done);
     dest.pe_start(pe_start);

     testset();

     SC_THREAD(run);
   }

  void testset() {
    spec::Axi::SlaveToRVA::Write rva_write_tmp;
    spec::Axi::SlaveToRVA::Read  rva_read_tmp;
 
    // AXI write 
    // GB SRAM config
    rva_write_tmp.rw = 1;
    rva_write_tmp.data = nvhls::get_rand<32>();
    rva_write_tmp.addr = set_bytes<3>("30_00_00");  // last 4 bits never used 
    rva_read_tmp.data = rva_write_tmp.data;
    source.src_vec.push_back(rva_write_tmp);
    dest.dest_vec.push_back(rva_read_tmp); 

    // GB Core large buffer config
    rva_write_tmp.rw = 1;
    rva_write_tmp.data = set_bytes<16>("00_00_00_00_00_00_00_00_00_00_00_00_00_00_00_10");
    rva_write_tmp.addr = set_bytes<3>("40_00_10");  // last 4 bits never used 
    rva_read_tmp.data = rva_write_tmp.data;
    source.src_vec.push_back(rva_write_tmp);
    dest.dest_vec.push_back(rva_read_tmp);   
    
    // Large Buffer SRAM
    rva_write_tmp.rw = 1;
    rva_write_tmp.data = nvhls::get_rand<32>();
    rva_write_tmp.addr = set_bytes<3>("50_00_00");  // last 4 bits never used 
    rva_read_tmp.data = rva_write_tmp.data;
    source.src_vec.push_back(rva_write_tmp);
    dest.dest_vec.push_back(rva_read_tmp); 

    // GB Core small buffer config
    rva_write_tmp.rw = 1;
    rva_write_tmp.data = set_bytes<16>("00_10_00_00_00_00_00_00_00_00_00_00_00_00_00_00");
    rva_write_tmp.addr = set_bytes<3>("40_00_20");  // last 4 bits never used 
    rva_read_tmp.data = rva_write_tmp.data;
    source.src_vec.push_back(rva_write_tmp);
    dest.dest_vec.push_back(rva_read_tmp);  

    
    // Small Buffer SRAM
    rva_write_tmp.rw = 1;
    rva_write_tmp.data = nvhls::get_rand<32>();
    rva_write_tmp.addr = set_bytes<3>("60_00_00");  // last 4 bits never used 
    rva_read_tmp.data = rva_write_tmp.data;
    source.src_vec.push_back(rva_write_tmp);    
    dest.dest_vec.push_back(rva_read_tmp); 
   
    // GBControl config
    rva_write_tmp.rw = 1;
    rva_write_tmp.data = set_bytes<16>("00_00_00_00_00_00_00_00_00_10_00_00_00_00_00_01");
    rva_write_tmp.addr = set_bytes<3>("70_00_10");  // last 4 bits never used 
    rva_read_tmp.data = rva_write_tmp.data;
    source.src_vec.push_back(rva_write_tmp);
    dest.dest_vec.push_back(rva_read_tmp); 

    // LayerReduce config
    rva_write_tmp.rw = 1;
    rva_write_tmp.data = set_bytes<16>("00_00_00_02_00_00_00_B0_00_10_00_00_00_00_00_01");
    rva_write_tmp.addr = set_bytes<3>("80_00_10");  // last 4 bits never used 
    rva_read_tmp.data = rva_write_tmp.data;
    source.src_vec.push_back(rva_write_tmp);
    dest.dest_vec.push_back(rva_read_tmp);  
    
    // LayerNorm config
    rva_write_tmp.rw = 1;
    rva_write_tmp.data = set_bytes<16>("01_00_00_02_00_00_01_40_00_10_00_00_00_00_00_01");
    rva_write_tmp.addr = set_bytes<3>("90_00_10");  // last 4 bits never used 
    rva_read_tmp.data = rva_write_tmp.data;
    source.src_vec.push_back(rva_write_tmp);
    dest.dest_vec.push_back(rva_read_tmp);  


    // ZeroPadding config
    rva_write_tmp.rw = 1;
    rva_write_tmp.data = set_bytes<16>("00_02_04_02_00_64_00_60_00_10_00_00_00_00_00_01");
    rva_write_tmp.addr = set_bytes<3>("A0_00_10");  // last 4 bits never used 
    rva_read_tmp.data = rva_write_tmp.data;
    source.src_vec.push_back(rva_write_tmp);
    dest.dest_vec.push_back(rva_read_tmp);  

    // Attention config
    rva_write_tmp.rw = 1;
    rva_write_tmp.data = set_bytes<16>("00_02_04_02_00_00_00_60_00_10_00_00_00_00_00_01");
    rva_write_tmp.addr = set_bytes<3>("B0_00_10");  // last 4 bits never used 
    rva_read_tmp.data = rva_write_tmp.data;
    source.src_vec.push_back(rva_write_tmp);
    dest.dest_vec.push_back(rva_read_tmp);   
            
    // AXI read
    rva_write_tmp.rw = 0;
    rva_write_tmp.addr = set_bytes<3>("30_00_00");
    source.src_vec.push_back(rva_write_tmp); 

    rva_write_tmp.rw = 0;
    rva_write_tmp.addr = set_bytes<3>("40_00_10");
    source.src_vec.push_back(rva_write_tmp); 

    rva_write_tmp.rw = 0;
    rva_write_tmp.addr = set_bytes<3>("50_00_00");
    source.src_vec.push_back(rva_write_tmp);   

    rva_write_tmp.rw = 0;
    rva_write_tmp.addr = set_bytes<3>("40_00_20");
    source.src_vec.push_back(rva_write_tmp); 

    rva_write_tmp.rw = 0;
    rva_write_tmp.addr = set_bytes<3>("60_00_00");
    source.src_vec.push_back(rva_write_tmp); 
   
    rva_write_tmp.rw = 0;
    rva_write_tmp.addr = set_bytes<3>("70_00_10");
    source.src_vec.push_back(rva_write_tmp);
               
    rva_write_tmp.rw = 0;
    rva_write_tmp.addr = set_bytes<3>("80_00_10");
    source.src_vec.push_back(rva_write_tmp);    

    rva_write_tmp.rw = 0;
    rva_write_tmp.addr = set_bytes<3>("90_00_10");
    source.src_vec.push_back(rva_write_tmp); 

    rva_write_tmp.rw = 0;
    rva_write_tmp.addr = set_bytes<3>("A0_00_10");
    source.src_vec.push_back(rva_write_tmp);

    rva_write_tmp.rw = 0;
    rva_write_tmp.addr = set_bytes<3>("B0_00_10");
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

}; //SC MODULE testbench


int sc_main(int argc, char *argv[]) {
  nvhls::set_random_seed();
  NVINT8 test = 14;
  cout << fixed2float<8, 3>(test) << endl;

  
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
