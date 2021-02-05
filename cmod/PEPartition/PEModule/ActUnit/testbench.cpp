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

#include "ActUnit.h"
#include "SM6Spec.h"
#include "AxiSpec.h"
//#include "BufferSpec.h"
//#include "BufferManager.h"
//#include "ControlSpec.h"
#include "AdpfloatSpec.h"
#include "AdpfloatUtils.h"

#include "helper.h"

#include <iostream>
#include <sstream>
#include <iomanip>


#define NVHLS_VERIFY_BLOCKS (ActUnit)
#include <nvhls_verify.h>

#ifdef COV_ENABLE
   #pragma CTC SKIP
#endif
SC_MODULE(Source) {
  sc_in<bool> clk;
  sc_in<bool> rst;  
  Connections::Out<spec::ActVectorType> act_port;
  Connections::Out<spec::Axi::SlaveToRVA::Write> rva_in;

  Connections::Out<bool> start;    

  bool start_src;
  spec::ActVectorType act_port_src;
  spec::ActVectorType test_in[16];
 
  SC_CTOR(Source) {
    SC_THREAD(run);
    sensitive << clk.pos();
    async_reset_signal_is(rst, false);
  }
  
  void run(){
    spec::Axi::SlaveToRVA::Write  rva_in_src;

    float ref_in[16][spec::kNumVectorLanes];
    for (int i = 0; i < 16; i++) {
      for (int j = 0; j < spec::kNumVectorLanes; j++) {
        test_in[i][j] = nvhls::get_rand<spec::kActWordWidth>();
        //cout << test_in[i][j] << " ";
        ref_in[i][j] = test_in[i][j];
        ref_in[i][j] = ref_in[i][j] / (1 << spec::kActNumFrac);
      }
    }
    float ref_out[16][spec::kNumVectorLanes];

    wait(); 

    //Axi Config 0x01
    rva_in_src.rw = 1;
    rva_in_src.data = set_bytes<16>("00_00_00_00_00_00_00_00_00_00_01_01_15_04_00_01"); //is_valid=1, is_zero_first=0, adpfloat_bias=4, num_inst=10, num_output=257, addr_base=0
    rva_in_src.addr = set_bytes<3>("80_00_10");  // last 4 bits never used 
    rva_in.Push(rva_in_src);
    wait(); 

    //inpe inst_reg[00] --> tanh actregs[00] --> and output_port --> inpe inst_reg[01] --> sigmoid actregs[01] --> output_port --> EADD actregs[01] --> output_port --> EMUL actregs [01] --> output_port
    //onex actregs[01] --> output_port --> copy actregs[00] to actregs[01] --> output_port
    rva_in_src.rw = 1;
    rva_in_src.data = set_bytes<16>("44_C4_44_74_44_D4_44_94_44_84_44_A4_34_40_B0_30");
    rva_in_src.addr = set_bytes<3>("80_00_20");  // last 4 bits never used 
    rva_in.Push(rva_in_src);
    wait();

    rva_in_src.rw = 1;
    rva_in_src.data = set_bytes<16>("00_00_00_00_00_00_00_00_00_00_00_4C_1C_24_44_D4");
    rva_in_src.addr = set_bytes<3>("80_00_30");  // last 4 bits never used 
    rva_in.Push(rva_in_src);
    wait();

    //start signal Poped
    start_src = 1;
    start.Push(start_src);
    wait();

    cout << "\nTest Tanh" << endl;     
    cout << "ref_in" << " \t " << "ref_out" << endl;
    //cout << "ref_in1" << " \t " << "ref_in2" << " \t " << "ref_out" << " \t " << endl; 
    for (int i = 0; i < spec::kNumVectorLanes; i++) {
      ref_out[0][i] = tanh(ref_in[0][i]);
      cout << ref_in[0][i] << " \t " << ref_out[0][i] << " \t " << endl; 
    }    

    //push actport for act_regs[00]
    act_port_src = test_in[0];
    act_port.Push(act_port_src);
    //wait();    
    wait(5);

    cout << "\nTest Sigmoid" << endl;     
    cout << "ref_in" << " \t " << "ref_out" << endl;
    //cout << "ref_in1" << " \t " << "ref_in2" << " \t " << "ref_out" << " \t " << endl; 
    for (int i = 0; i < spec::kNumVectorLanes; i++) {
      ref_out[1][i] = sigmoid(ref_in[1][i]);
      cout << ref_in[1][i] << " \t " << ref_out[1][i] << " \t " << endl; 
    }   

    //push actport for act_regs[01]
    act_port_src = test_in[1];
    act_port.Push(act_port_src);
    wait(5); 

    cout << "\nTest EADD" << endl;     
    cout << "ref_in1" << " \t " << "ref_in2" << " \t " << "ref_out" << " \t " << endl; 
    for (int i = 0; i < spec::kNumVectorLanes; i++) {
      ref_out[2][i] = ref_out[0][i] + ref_out[1][i];
      cout << ref_out[0][i] << " \t " << ref_out[1][i] << " \t " << ref_out[2][i] << " \t " << endl; 
    } 
    wait(2);

    cout << "\nTest EMUL" << endl;     
    cout << "ref_in1" << " \t " << "ref_in2" << " \t " << "ref_out" << " \t " << endl; 
    for (int i = 0; i < spec::kNumVectorLanes; i++) {
      ref_out[3][i] = ref_out[0][i] * ref_out[2][i];
      cout << ref_out[0][i] << " \t " << ref_out[2][i] << " \t " << ref_out[3][i] << " \t " << endl; 
    } 
    wait(2);

    cout << "\nTest OneX" << endl;     
    cout << "ref_in" << " \t " << "ref_out" << " \t " << endl; 
    for (int i = 0; i < spec::kNumVectorLanes; i++) {
      ref_out[4][i] = 1 - ref_out[3][i];
      cout << ref_out[3][i] << " \t " << ref_out[4][i] << " \t " << endl; 
    } 
    wait(2);     

    cout << "\nTest Copy" << endl;     
    cout << "ref_in" << " \t " << "ref_out" << " \t " << endl; 
    for (int i = 0; i < spec::kNumVectorLanes; i++) {
      ref_out[5][i] = ref_out[0][i];
      cout << ref_out[0][i] << " \t " << ref_out[5][i] << " \t " << endl; 
    } 
    wait(2); 

    cout << "\nTest Relu" << endl;     
    cout << "ref_in" << " \t " << "ref_out" << " \t " << endl; 
    for (int i = 0; i < spec::kNumVectorLanes; i++) {
      if (ref_out[5][i] < 0) ref_out[6][i] = 0;
          else ref_out[6][i] = ref_out[5][i];
      cout << ref_out[5][i] << " \t " << ref_out[6][i] << " \t " << endl; 
    } 
    wait(2);

    cout << "\nTest OneX" << endl;     
    cout << "ref_in" << " \t " << "ref_out" << " \t " << endl; 
    for (int i = 0; i < spec::kNumVectorLanes; i++) {
      ref_out[7][i] = 1 - ref_out[6][i];
      cout << ref_out[6][i] << " \t " << ref_out[7][i] << " \t " << endl; 
    } 
    wait(2); 
   
  }// void run()

};

SC_MODULE(Dest) {
  sc_in<bool> clk;
  sc_in<bool> rst;
  Connections::In<spec::Axi::SlaveToRVA::Read> rva_out;
  Connections::In<spec::StreamType> output_port; 
  Connections::In<bool> done;

  spec::StreamType output_port_dest;
  spec::Axi::SlaveToRVA::Read rva_out_dest;

  SC_CTOR(Dest) {
    SC_THREAD(Pop_rva_out);
    sensitive << clk.pos();
    async_reset_signal_is(rst, false);
    SC_THREAD(PopOutport);
    sensitive << clk.pos();
    async_reset_signal_is(rst, false);
  }
  
  void Pop_rva_out(){
    wait();
    
    while (1) {
      if (rva_out.PopNB(rva_out_dest)) {
        //cout << hex << sc_time_stamp() << " Dest rva data = " << rva_out_dest.data << endl;
      }
      wait();    
    } // while
  } //Pop_rva_out

  void PopOutport() {
   wait();
 
   while (1) {
     if (output_port.PopNB(output_port_dest)) {
        //cout << hex << sc_time_stamp() << " output_port data = " << output_port_dest.data << endl;
        cout << "Design Output" << " \t " << endl;
        for (int i = 0; i < spec::kNumVectorLanes; i++) {
          AdpfloatType<8,3> tmp(output_port_dest.data[i]);
          cout << tmp.to_float(4) << endl; 
        }
     }
     wait(); 
   } // while
   
  } //PopOutputport

}; //SC_MODULE Dest


SC_MODULE(testbench) {
  SC_HAS_PROCESS(testbench);
	sc_clock clk;
  sc_signal<bool> rst;

  Connections::Combinational<spec::ActVectorType> act_port;
  Connections::Combinational<spec::Axi::SlaveToRVA::Write> rva_in;
  Connections::Combinational<spec::Axi::SlaveToRVA::Read> rva_out;
  Connections::Combinational<spec::StreamType> output_port; 
  
  Connections::Combinational<bool> start;
  Connections::Combinational<bool> done;

  NVHLS_DESIGN(ActUnit) dut;
  //ActUnit dut;
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
	  dut.act_port(act_port);
		dut.rva_in(rva_in);
		dut.rva_out(rva_out);		
		dut.output_port(output_port);
    dut.start(start);
    dut.done(done);		
    
    source.clk(clk);
    source.rst(rst); 
	  source.act_port(act_port);
		source.rva_in(rva_in);
    source.start(start);
    		
		dest.clk(clk);
		dest.rst(rst);
		dest.rva_out(rva_out);
		dest.output_port(output_port);
    dest.done(done);		
    
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
