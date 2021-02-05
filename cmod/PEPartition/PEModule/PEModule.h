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

#ifndef PE_MODULE_H
#define PE_MODULE_H

#include <systemc.h>
#include <nvhls_int.h>
#include <nvhls_types.h>
#include <nvhls_vector.h>
#include <nvhls_module.h>
#include "SM6Spec.h"
#include "AxiSpec.h"

#include "PECore/PECore.h"
#include "ActUnit/ActUnit.h"

class PERVA : public match::Module { 
  static const int kDebugLevel = 3;
  SC_HAS_PROCESS(PERVA);
 public: 
  Connections::In<spec::Axi::SlaveToRVA::Write> rva_in;
  Connections::Out<spec::Axi::SlaveToRVA::Read> rva_out;  
  Connections::In<bool> start; 
  
  // start trigger (streaming) is from GB -> Databus
  // Axi and also acted as start trigger (but please use streaming start )
  
  // 0: PEBlock Start
  Connections::Out<bool> pe_start;
  Connections::Out<bool> act_start;
  
  // 4, 5, 6
  Connections::Out<spec::Axi::SlaveToRVA::Write>    pe_rva_in;
  Connections::In<spec::Axi::SlaveToRVA::Read>      pe_rva_out;  
  // 8, 9
  Connections::Out<spec::Axi::SlaveToRVA::Write>    act_rva_in;
  Connections::In<spec::Axi::SlaveToRVA::Read>      act_rva_out;
  
  sc_out<NVUINT32> SC_SRAM_CONFIG;    
  
  // Constructor
  PERVA (sc_module_name nm)
      : match::Module(nm),
        rva_in("rva_in"),
        rva_out("rva_out"),
        start("start"),
        pe_start("pe_start"),
        act_start("act_start"), 
        pe_rva_in("pe_rva_in"),
        pe_rva_out("pe_rva_out"),
        act_rva_in("act_rva_in"),
        act_rva_out("act_rva_out"),
        SC_SRAM_CONFIG("SC_SRAM_CONFIG")
  {
    SC_THREAD(RVAInRun);
    sensitive << clk.pos();
    async_reset_signal_is(rst, false);
    
    SC_THREAD(RVAOutRun);
    sensitive << clk.pos();
    async_reset_signal_is(rst, false);
  }   

    void RVAInRun() {
    rva_in.Reset();
    start.Reset();
    pe_rva_in.Reset();
    act_rva_in.Reset();

    pe_start.Reset();
    act_start.Reset();
    SC_SRAM_CONFIG.write(0);
    #pragma hls_pipeline_init_interval 1
    while(1){
      // Axi input
      bool start_reg;
      bool is_start = 0;
      spec::Axi::SlaveToRVA::Write rva_in_reg;    
 
      if(rva_in.PopNB(rva_in_reg)) {
        NVUINT4 tmp = nvhls::get_slc<4>(rva_in_reg.addr, 20);
        switch (tmp) {
          case 0x0:
            is_start = 1;
            break;
          case 0x3: // read is implemented inside PECore
            if (rva_in_reg.rw) {
              SC_SRAM_CONFIG.write(nvhls::get_slc<32>(rva_in_reg.data, 0));
            }
            else {              
              pe_rva_in.Push(rva_in_reg);            
            }
            break;
          case 0x4:
          case 0x5:
          case 0x6:
            pe_rva_in.Push(rva_in_reg);
            break;
          case 0x8:
          case 0x9:
            act_rva_in.Push(rva_in_reg);
            break;
          case 0xE:
            pe_rva_in.Push(rva_in_reg);
            break;
          case 0xF:
            act_rva_in.Push(rva_in_reg);
            break;
          default:
            break;
        }      
      
      }
      else if (start.PopNB(start_reg)) {
        is_start = 1;
      }

      if (is_start) {
         pe_start.Push(1);
         act_start.Push(1);
      }
      
      wait();
    } //while
    } // RVAInRun

   void RVAOutRun() {
    rva_out.Reset();
    pe_rva_out.Reset();        
    act_rva_out.Reset();        	

    #pragma hls_pipeline_init_interval 1
    while(1){
      spec::Axi::SlaveToRVA::Read rva_out_reg;
      bool is_valid = 0;
      if (pe_rva_out.PopNB(rva_out_reg)) {
        is_valid = 1;
      }
      else if (act_rva_out.PopNB(rva_out_reg)) {
        is_valid = 1;
      }
      
      if (is_valid) {
        rva_out.Push(rva_out_reg);
      }
      wait();
    }		
	}  
};

class PEModule : public match::Module { 
  SC_HAS_PROCESS(PEModule);
 public:
  Connections::In<bool> start;  
  Connections::Out<bool> done;
  Connections::In<spec::Axi::SlaveToRVA::Write> rva_in;  
  Connections::Out<spec::Axi::SlaveToRVA::Read> rva_out;
  Connections::In<spec::StreamType> input_port;     
  Connections::Out<spec::StreamType> output_port; 
  
  Connections::Combinational<bool> pe_start;
  Connections::Combinational<spec::Axi::SlaveToRVA::Write> pe_rva_in;
  Connections::Combinational<spec::Axi::SlaveToRVA::Read> pe_rva_out;
  Connections::Combinational<bool> act_start;    
  Connections::Combinational<spec::Axi::SlaveToRVA::Write> act_rva_in;
  Connections::Combinational<spec::Axi::SlaveToRVA::Read> act_rva_out;
  Connections::Combinational<spec::ActVectorType> act_port;

  sc_signal<NVUINT32> SC_SRAM_CONFIG;

  PERVA perva_inst;
  PECore  pecore_inst;
  ActUnit act_inst; 
  
  // Constructor
  PEModule(sc_module_name nm)
      : match::Module(nm),
        start("start"),
        done("done"),
        rva_in("rva_in"),
        rva_out("rva_out"),
        input_port("input_port"),
        output_port("output_port"),
        pe_start("pe_start"),
        pe_rva_in("pe_rva_in"),
        pe_rva_out("pe_rva_out"),
        act_start("act_start"),
        act_rva_in("act_rva_in"),
        act_rva_out("act_rva_out"),
        act_port("act_port"),
        SC_SRAM_CONFIG("SC_SRAM_CONFIG"),
        perva_inst("perva_inst"),
        pecore_inst("pecore_inst"),
        act_inst("act_inst")
  {
    perva_inst.clk(clk);
    perva_inst.rst(rst);    
    perva_inst.rva_in(rva_in);
    perva_inst.rva_out(rva_out);
    perva_inst.start(start);
    perva_inst.pe_start(pe_start);
    perva_inst.pe_rva_in(pe_rva_in);
    perva_inst.pe_rva_out(pe_rva_out);
    perva_inst.act_start(act_start);
    perva_inst.act_rva_in(act_rva_in);
    perva_inst.act_rva_out(act_rva_out);
    perva_inst.SC_SRAM_CONFIG(SC_SRAM_CONFIG);
    
    pecore_inst.clk(clk);
    pecore_inst.rst(rst);
    pecore_inst.act_port(act_port);
    pecore_inst.input_port(input_port);
    pecore_inst.start(pe_start);
    pecore_inst.rva_in(pe_rva_in);
    pecore_inst.rva_out(pe_rva_out);
    pecore_inst.SC_SRAM_CONFIG(SC_SRAM_CONFIG);


    act_inst.clk(clk);
    act_inst.rst(rst);    
    act_inst.act_port(act_port);
    act_inst.start(act_start);
    act_inst.rva_in(act_rva_in);
    act_inst.rva_out(act_rva_out);
    act_inst.output_port(output_port);
    act_inst.done(done);
  }
  
  
};


#endif
