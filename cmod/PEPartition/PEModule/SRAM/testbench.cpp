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

#include "SRAMTop.h"
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
#include "helper.h"

#define NVHLS_VERIFY_BLOCKS (SRAMTop)
#include <nvhls_verify.h>

#ifdef COV_ENABLE
   #pragma CTC SKIP
#endif

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

}; //SC MODULE Source

SC_MODULE(Dest) {
  sc_in<bool> clk;
  sc_in<bool> rst;
  Connections::In<spec::Axi::SlaveToRVA::Read> rva_out;

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

  Connections::Combinational<spec::Axi::SlaveToRVA::Write> rva_in;
  Connections::Combinational<spec::Axi::SlaveToRVA::Read> rva_out;

  NVHLS_DESIGN(SRAMTop) dut;
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

    rva_write_tmp.rw = 1;
    rva_write_tmp.data = set_bytes<16>("00_00_00_00_00_00_00_00_00_00_20_02_01_01_01_01");
    rva_write_tmp.addr = set_bytes<3>("60_00_20");  // last 4 bits never used 
    rva_read_tmp.data = rva_write_tmp.data;
    source.src_vec.push_back(rva_write_tmp);
    dest.dest_vec.push_back(rva_read_tmp);

    rva_write_tmp.rw = 1;
    rva_write_tmp.data = set_bytes<16>("00_00_00_00_00_00_00_00_00_00_33_02_01_33_01_01");
    rva_write_tmp.addr = set_bytes<3>("60_00_40");  // last 4 bits never used 
    rva_read_tmp.data = rva_write_tmp.data;
    source.src_vec.push_back(rva_write_tmp);
    dest.dest_vec.push_back(rva_read_tmp);

    rva_write_tmp.rw = 1;
    rva_write_tmp.data = set_bytes<16>("00_00_00_00_00_00_00_00_00_00_20_02_44_01_77_FF");
    rva_write_tmp.addr = set_bytes<3>("60_00_70");  // last 4 bits never used 
    rva_read_tmp.data = rva_write_tmp.data;
    source.src_vec.push_back(rva_write_tmp);
    dest.dest_vec.push_back(rva_read_tmp);

    rva_write_tmp.rw = 1;
    rva_write_tmp.data = set_bytes<16>("00_00_00_00_00_00_00_00_00_00_20_02_01_01_01_33");
    rva_write_tmp.addr = set_bytes<3>("60_01_20");  // last 4 bits never used 
    rva_read_tmp.data = rva_write_tmp.data;
    source.src_vec.push_back(rva_write_tmp);
    dest.dest_vec.push_back(rva_read_tmp);

    rva_write_tmp.rw = 0;
    rva_write_tmp.addr = set_bytes<3>("60_00_20");
    source.src_vec.push_back(rva_write_tmp);

    rva_write_tmp.rw = 0;
    rva_write_tmp.addr = set_bytes<3>("60_00_40");
    source.src_vec.push_back(rva_write_tmp);

    rva_write_tmp.rw = 0;
    rva_write_tmp.addr = set_bytes<3>("60_00_70");
    source.src_vec.push_back(rva_write_tmp);

    rva_write_tmp.rw = 0;
    rva_write_tmp.addr = set_bytes<3>("60_01_20");
    source.src_vec.push_back(rva_write_tmp);
  }

  void run(){
    wait(2, SC_NS );
    std::cout << "@" << sc_time_stamp() <<" Asserting reset" << std::endl;
    rst.write(false);
    wait(2, SC_NS );
    rst.write(true);
    std::cout << "@" << sc_time_stamp() <<" De-Asserting reset" << std::endl;
    wait(100, SC_NS );
    std::cout << "@" << sc_time_stamp() <<" sc_stop" << std::endl;
    sc_stop();
  }  
}; //SC MODULE testbench


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
