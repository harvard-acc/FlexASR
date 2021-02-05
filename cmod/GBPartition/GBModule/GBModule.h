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

#ifndef __GBMODULE__
#define __GBMODULE__

#include <systemc.h>
#include <nvhls_int.h>
#include <nvhls_types.h>
#include <nvhls_vector.h>
#include <nvhls_module.h>
#include <ArbitratedScratchpadDP.h>
#include "SM6Spec.h"
#include "AxiSpec.h"

#include "GBCore/GBCore.h"
#include "GBControl/GBControl.h"
#include "LayerReduce/LayerReduce.h"
#include "LayerNorm/LayerNorm.h"
#include "ZeroPadding/ZeroPadding.h"
#include "Attention/Attention.h"
class GBRVA : public match::Module { 
  static const int kDebugLevel = 3;
  SC_HAS_PROCESS(GBRVA);
 public: 
  Connections::In<spec::Axi::SlaveToRVA::Write> rva_in;
  Connections::Out<spec::Axi::SlaveToRVA::Read> rva_out;  
  
  // 0: GBBLock Start 
  Connections::Out<bool> gbcontrol_start;
  Connections::Out<bool> layerreduce_start;
  Connections::Out<bool> layernorm_start;
  Connections::Out<bool> zeropadding_start;
  Connections::Out<bool> attention_start; 
  // 4, 5, 6
  Connections::Out<spec::Axi::SlaveToRVA::Write>    gbcore_large_rva_in;
  Connections::In<spec::Axi::SlaveToRVA::Read>      gbcore_large_rva_out; 
   
  Connections::Out<spec::Axi::SlaveToRVA::Write>    gbcore_small_rva_in;
  Connections::In<spec::Axi::SlaveToRVA::Read>      gbcore_small_rva_out;    
  
  // 7
  Connections::Out<spec::Axi::SlaveToRVA::Write>    gbcontrol_rva_in;
  Connections::In<spec::Axi::SlaveToRVA::Read>      gbcontrol_rva_out;  
  // 8
  Connections::Out<spec::Axi::SlaveToRVA::Write>    layerreduce_rva_in;
  Connections::In<spec::Axi::SlaveToRVA::Read>      layerreduce_rva_out;  
  // 9
  Connections::Out<spec::Axi::SlaveToRVA::Write>    layernorm_rva_in;
  Connections::In<spec::Axi::SlaveToRVA::Read>      layernorm_rva_out; 
  // A
  Connections::Out<spec::Axi::SlaveToRVA::Write>    zeropadding_rva_in;
  Connections::In<spec::Axi::SlaveToRVA::Read>      zeropadding_rva_out;    
  // B TODO: For Attention module
  Connections::Out<spec::Axi::SlaveToRVA::Write>    attention_rva_in;
  Connections::In<spec::Axi::SlaveToRVA::Read>      attention_rva_out;   
    
  sc_out<NVUINT32> SC_SRAM_CONFIG;  
  
  // Constructor
  GBRVA (sc_module_name nm)
      : match::Module(nm),
        rva_in("rva_in"),
        rva_out("rva_out"),
        gbcontrol_start("gbcontrol_start"),
        layerreduce_start("layerreduce_start"), 
        layernorm_start("layernorm_start"),
        zeropadding_start("zeropadding_start"),
        attention_start("attention_start"),
        gbcore_large_rva_in("gbcore_large_rva_in"),
        gbcore_large_rva_out("gbcore_large_rva_out"),
        gbcore_small_rva_in("gbcore_small_rva_in"),
        gbcore_small_rva_out("gbcore_small_rva_out"),        
        
        gbcontrol_rva_in("gbcontrol_rva_in"),
        gbcontrol_rva_out("gbcontrol_rva_out"),
        layerreduce_rva_in("layerreduce_rva_in"),
        layerreduce_rva_out("layerreduce_rva_out"),
        layernorm_rva_in("layernorm_rva_in"),
        layernorm_rva_out("layernorm_rva_out"),
        zeropadding_rva_in("zeropadding_rva_in"),
        zeropadding_rva_out("zeropadding_rva_out"),
        attention_rva_in("attention_rva_in"),
        attention_rva_out("attention_rva_out")
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
    gbcore_large_rva_in.Reset();
    gbcore_small_rva_in.Reset();    
    gbcontrol_rva_in.Reset();
    layerreduce_rva_in.Reset();
    layernorm_rva_in.Reset();    
    zeropadding_rva_in.Reset();  
    attention_rva_in.Reset();
    
    gbcontrol_start.Reset();
    layerreduce_start.Reset();
    layernorm_start.Reset();
    zeropadding_start.Reset();
    attention_start.Reset();
    
    SC_SRAM_CONFIG.write(0);

    #pragma hls_pipeline_init_interval 1    
    while(1){
      // port TransferNB     rva_out.TransferNB();
      //gbcore_rva_in.TransferNB();
      //gbcontrol_rva_in.TransferNB();
      //layerreduce_rva_in.TransferNB();
      //layernorm_rva_in.TransferNB();    
      //zeropadding_rva_in.TransferNB();   
      
      //gbcontrol_start.TransferNB();
      //layerreduce_start.TransferNB();
      //layernorm_start.TransferNB();
      //zeropadding_start.TransferNB();
      
      // Axi input
      spec::Axi::SlaveToRVA::Write rva_in_reg;
      if (rva_in.PopNB(rva_in_reg)) {
        NVUINT4 tmp = nvhls::get_slc<4>(rva_in_reg.addr, 20);
        NVUINT16 local_index = nvhls::get_slc<16>(rva_in_reg.addr, 4);
        switch (tmp) {   
          case 0x0: {
            switch (local_index) {
              case 0x1: 
                gbcontrol_start.Push(1);
                break;
              case 0x2: 
                layerreduce_start.Push(1);
                break;
              case 0x3:
                layernorm_start.Push(1);
                break;              
              case 0x4:
                zeropadding_start.Push(1);
                break;              
              case 0x5: // TODO attention start 
                attention_start.Push(1);
                break; 
              default:
                break;
            }
            break;
          }
          case 0x3: 
            if (rva_in_reg.rw) {
              SC_SRAM_CONFIG.write(nvhls::get_slc<32>(rva_in_reg.data, 0));
            } 
            else {
              gbcore_large_rva_in.Push(rva_in_reg);              
            }
            break;
          case 0x4: {
            if (local_index == 0x01) {
              // local 1: Large Buffer Config     
              gbcore_large_rva_in.Push(rva_in_reg);
            }
            else if (local_index == 0x02) {
              // local 2: Small Buffer Config            
              gbcore_small_rva_in.Push(rva_in_reg);            
            } 
            break;
          }
          case 0x5: 
            // Large Buffer Access
            gbcore_large_rva_in.Push(rva_in_reg);
            break;
          case 0x6:
            // Small Buffer Access
            gbcore_small_rva_in.Push(rva_in_reg);
            break;
          case 0x7:
            gbcontrol_rva_in.Push(rva_in_reg);
            break;
          case 0x8:
            layerreduce_rva_in.Push(rva_in_reg);
            break;            
          case 0x9:
            layernorm_rva_in.Push(rva_in_reg);
            break;
          case 0xA:
            zeropadding_rva_in.Push(rva_in_reg);
            break;
          case 0xB: // Attehtion
            attention_rva_in.Push(rva_in_reg);
            break;         
          default: 
            break;
        }              
      }
      wait();
    }
	}
	void RVAOutRun() {
    rva_out.Reset();
    gbcore_large_rva_out.Reset();  
    gbcore_small_rva_out.Reset();            
    gbcontrol_rva_out.Reset();        
    layerreduce_rva_out.Reset();       
    layernorm_rva_out.Reset(); 
    zeropadding_rva_out.Reset(); 
    attention_rva_out.Reset(); 

    #pragma hls_pipeline_init_interval 1
    while(1){
      spec::Axi::SlaveToRVA::Read rva_out_reg;
      bool is_valid = 0;
      
      if (gbcore_large_rva_out.PopNB(rva_out_reg)) {
        is_valid = 1;       
      }
      else if (gbcore_small_rva_out.PopNB(rva_out_reg)) {
        is_valid = 1;       
      }      
      else if (gbcontrol_rva_out.PopNB(rva_out_reg)) {
        is_valid = 1; 
      }
      else if (layerreduce_rva_out.PopNB(rva_out_reg)) {
        is_valid = 1; 
      }
      else if (layernorm_rva_out.PopNB(rva_out_reg)) {
        is_valid = 1; 
      }
      else if (zeropadding_rva_out.PopNB(rva_out_reg)) {
        is_valid = 1; 
      }
      else if (attention_rva_out.PopNB(rva_out_reg)) {
        is_valid = 1;
      }
      
      if (is_valid) {
        rva_out.Push(rva_out_reg);
      }
      wait();
    }		
	}  
};

class GBDone : public match::Module { 
  static const int kDebugLevel = 3;
  SC_HAS_PROCESS(GBDone);
 public: 
  Connections::Out<bool> done; 
  Connections::In<bool> gbcontrol_done;
  Connections::In<bool> layerreduce_done;
  Connections::In<bool> layernorm_done;
  Connections::In<bool> zeropadding_done; 
  Connections::In<bool> attention_done;   
  
   // Constructor
  GBDone (sc_module_name nm)
      : match::Module(nm),
        gbcontrol_done("gbcontrol_done"),
        layerreduce_done("layerreduce_done"), 
        layernorm_done("layernorm_done"),
        zeropadding_done("zeropadding_done"),
        attention_done("attention_done")
  {
    SC_THREAD(GBDoneRun);
    sensitive << clk.pos();
    async_reset_signal_is(rst, false);
  }    
  
  void GBDoneRun() {
    done.Reset();
    gbcontrol_done.Reset();
    layerreduce_done.Reset();
    layernorm_done.Reset();
    zeropadding_done.Reset(); 
    attention_done.Reset();

    #pragma hls_pipeline_init_interval 1
    while(1) {
      bool is_done = 0, done_reg = 0;
      if (gbcontrol_done.PopNB(done_reg)) {
        is_done = 1;
      }
      else if (layerreduce_done.PopNB(done_reg)) {
        is_done = 1;
      }
      else if (layernorm_done.PopNB(done_reg)) {
        is_done = 1;
      }
      else if (zeropadding_done.PopNB(done_reg)) {
        is_done = 1;
      }
      else if (attention_done.PopNB(done_reg)) {
        is_done = 1;
      }
      if (is_done == 1){
        done.Push(1);       
      }
      wait();
    }
  }
};


// GB module does not handle the distribution of workloads across PEs, that part is implements 
class GBModule : public match::Module { 
  static const int kDebugLevel = 3;
  SC_HAS_PROCESS(GBModule);
 public:
  Connections::In<spec::Axi::SlaveToRVA::Write>     rva_in;
  Connections::Out<spec::Axi::SlaveToRVA::Read>     rva_out;
  Connections::Out<bool> done;
  
  //GBControl <-> PE
  Connections::In<spec::StreamType>   data_in;          
  Connections::Out<spec::StreamType>  data_out;
  Connections::Out<bool>              pe_start;
  Connections::In<bool>               pe_done;  
  
  
  // GBCore 3, 4, 5, 6
  Connections::Combinational<spec::Axi::SlaveToRVA::Write>    gbcore_large_rva_in;
  Connections::Combinational<spec::Axi::SlaveToRVA::Read>     gbcore_large_rva_out; 
  Connections::Combinational<spec::Axi::SlaveToRVA::Write>    gbcore_small_rva_in;
  Connections::Combinational<spec::Axi::SlaveToRVA::Read>     gbcore_small_rva_out;    
  // GBControl 7
  Connections::Combinational<spec::Axi::SlaveToRVA::Write>    gbcontrol_rva_in;
  Connections::Combinational<spec::Axi::SlaveToRVA::Read>     gbcontrol_rva_out;  
  // LayerReduce 8
  Connections::Combinational<spec::Axi::SlaveToRVA::Write>    layerreduce_rva_in;
  Connections::Combinational<spec::Axi::SlaveToRVA::Read>     layerreduce_rva_out;  
  // LayerNorm 9
  Connections::Combinational<spec::Axi::SlaveToRVA::Write>    layernorm_rva_in;
  Connections::Combinational<spec::Axi::SlaveToRVA::Read>     layernorm_rva_out; 
  // ZeroPadding A
  Connections::Combinational<spec::Axi::SlaveToRVA::Write>    zeropadding_rva_in;
  Connections::Combinational<spec::Axi::SlaveToRVA::Read>     zeropadding_rva_out;    
  // TODO: For Attention module B  
  Connections::Combinational<spec::Axi::SlaveToRVA::Write>    attention_rva_in;
  Connections::Combinational<spec::Axi::SlaveToRVA::Read>     attention_rva_out;     
 
 
  Connections::Combinational<bool> gbcontrol_start;
  Connections::Combinational<bool> layerreduce_start;
  Connections::Combinational<bool> layernorm_start;
  Connections::Combinational<bool> zeropadding_start; 
  Connections::Combinational<bool> attention_start; 
    
  Connections::Combinational<bool> gbcontrol_done;
  Connections::Combinational<bool> layerreduce_done;
  Connections::Combinational<bool> layernorm_done;
  Connections::Combinational<bool> zeropadding_done; 
  Connections::Combinational<bool> attention_done;   
  
  // GBControl
  Connections::Combinational<spec::GB::Large::DataReq>      gbcontrol_large_req;
  Connections::Combinational<spec::GB::Large::DataRsp<1>>   gbcontrol_large_rsp;  
  Connections::Combinational<spec::GB::Small::DataReq>      gbcontrol_small_req;
  Connections::Combinational<spec::GB::Small::DataRsp>      gbcontrol_small_rsp;
  // LayerReduce
  Connections::Combinational<spec::GB::Large::DataReq>      layerreduce_large_req;
  Connections::Combinational<spec::GB::Large::DataRsp<2>>   layerreduce_large_rsp;  
  // layerNorm
  Connections::Combinational<spec::GB::Large::DataReq>      layernorm_large_req;
  Connections::Combinational<spec::GB::Large::DataRsp<1>>   layernorm_large_rsp;  
  Connections::Combinational<spec::GB::Small::DataReq>      layernorm_small_req;
  Connections::Combinational<spec::GB::Small::DataRsp>      layernorm_small_rsp;  
  // ZeroPadding
  Connections::Combinational<spec::GB::Large::DataReq>      zeropadding_large_req;
  Connections::Combinational<spec::GB::Large::DataRsp<1>>   zeropadding_large_rsp;    
  
  Connections::Combinational<spec::GB::Large::DataReq>       attention_large_req;
  Connections::Combinational<spec::GB::Large::DataRsp<16>>  attention_large_rsp;   
  Connections::Combinational<spec::GB::Small::DataReq>       attention_small_req;
  Connections::Combinational<spec::GB::Small::DataRsp>      attention_small_rsp;   


  
  sc_signal<NVUINT32> SC_SRAM_CONFIG;
  
  GBRVA         gbrva_inst;
  GBDone        gbdone_inst;
  
  GBCore        gbcore_inst; 
  GBControl     gbcontrol_inst; 
  LayerReduce   layerreduce_inst;
  LayerNorm     layernorm_inst;
  ZeroPadding   zeropadding_inst;
  Attention     attention_inst;
  
  
  GBModule(sc_module_name nm)
      : match::Module(nm),
        rva_in  ("rva_in"),
        rva_out ("rva_out"),
        done    ("done"),
        // GB <-> PE (GBControl)
        data_in   ("data_in"),
        data_out  ("data_out"),
        pe_start  ("pe_start"),
        pe_done   ("pe_done"),
        
        gbcore_large_rva_in       ("gbcore_large_rva_in"),
        gbcore_large_rva_out      ("gbcore_large_rva_out"), 
        gbcore_small_rva_in       ("gbcore_small_rva_in"),
        gbcore_small_rva_out      ("gbcore_small_rva_out"), 
                
        gbcontrol_rva_in    ("gbcontrol_rva_in"),
        gbcontrol_rva_out   ("gbcontrol_rva_out"),
        layerreduce_rva_in  ("layerreduce_rva_in"),
        layerreduce_rva_out ("layerreduce_rva_out"),     
        layernorm_rva_in    ("layernorm_rva_in"),
        layernorm_rva_out   ("layernorm_rva_out"),  
        zeropadding_rva_in  ("zeropadding_rva_in"),
        zeropadding_rva_out ("zeropadding_rva_out"),
        attention_rva_in    ("attention_rva_in"),
        attention_rva_out   ("attention_rva_out"),
        
        gbcontrol_start     ("gbcontrol_start"),
        layerreduce_start   ("layerreduce_start"),
        layernorm_start     ("layernorm_start"),
        zeropadding_start   ("zeropadding_start"), 
        attention_start     ("attention_start"),        
        
        gbcontrol_done      ("gbcontrol_done"),
        layerreduce_done    ("layerreduce_done"),
        layernorm_done      ("layernorm_done"),
        zeropadding_done    ("zeropadding_done"),         
        attention_done      ("attention_done"),
        
        //GB Control, LayerReduce, LayerNorm, ZeroPadding
        gbcontrol_large_req   ("gbcontrol_large_req"),
        gbcontrol_large_rsp   ("gbcontrol_large_rsp"),  
        gbcontrol_small_req   ("gbcontrol_small_req"),
        gbcontrol_small_rsp   ("gbcontrol_small_rsp"), 
        
        layerreduce_large_req ("layerreduce_large_req"),
        layerreduce_large_rsp ("layerreduce_large_rsp"),
        
        layernorm_large_req   ("layernorm_large_req"),
        layernorm_large_rsp   ("layernorm_large_rsp"),
        layernorm_small_req   ("layernorm_small_req"),
        layernorm_small_rsp   ("layernorm_small_rsp"),
        
        zeropadding_large_req ("zeropadding_large_req"),
        zeropadding_large_rsp ("zeropadding_large_rsp"),
        
        attention_large_req ("attention_large_req"),
        attention_large_rsp ("attention_large_rsp"),
        attention_small_req ("attention_small_req"),
        attention_small_rsp ("attention_small_rsp"),
                
        SC_SRAM_CONFIG("SC_SRAM_CONFIG"),
        
        gbrva_inst("gbrva_inst"),
        gbdone_inst("gbdone_inst"),
        gbcore_inst("gbcore_inst"),
	gbcontrol_inst("gbcontrol_inst"), 
	layerreduce_inst("layerreduce_inst"),
	layernorm_inst("layernorm_inst"),
        zeropadding_inst("zeropadding_inst"),
        attention_inst("attention_inst")
  {
    //gbrva_inst
    gbrva_inst.clk(clk);
    gbrva_inst.rst(rst);
    gbrva_inst.rva_in(rva_in);
    gbrva_inst.rva_out(rva_out);  
    gbrva_inst.gbcontrol_start(gbcontrol_start);
    gbrva_inst.layerreduce_start(layerreduce_start);
    gbrva_inst.layernorm_start(layernorm_start);
    gbrva_inst.zeropadding_start(zeropadding_start);
    gbrva_inst.attention_start(attention_start);
    
    gbrva_inst.gbcore_large_rva_in      (gbcore_large_rva_in);
    gbrva_inst.gbcore_large_rva_out     (gbcore_large_rva_out); 
    gbrva_inst.gbcore_small_rva_in      (gbcore_small_rva_in);
    gbrva_inst.gbcore_small_rva_out     (gbcore_small_rva_out);     
     
    gbrva_inst.gbcontrol_rva_in   (gbcontrol_rva_in);
    gbrva_inst.gbcontrol_rva_out  (gbcontrol_rva_out);  
    gbrva_inst.layerreduce_rva_in (layerreduce_rva_in);
    gbrva_inst.layerreduce_rva_out(layerreduce_rva_out);  
    gbrva_inst.layernorm_rva_in   (layernorm_rva_in);
    gbrva_inst.layernorm_rva_out  (layernorm_rva_out); 
    gbrva_inst.zeropadding_rva_in (zeropadding_rva_in);
    gbrva_inst.zeropadding_rva_out(zeropadding_rva_out);  
    gbrva_inst.attention_rva_in   (attention_rva_in);
    gbrva_inst.attention_rva_out  (attention_rva_out);  
        
          
    gbrva_inst.SC_SRAM_CONFIG(SC_SRAM_CONFIG);
    
    
    //gbdone_inst
    gbdone_inst.clk(clk);
    gbdone_inst.rst(rst);
    gbdone_inst.done(done); 
    gbdone_inst.gbcontrol_done(gbcontrol_done);
    gbdone_inst.layerreduce_done(layerreduce_done);
    gbdone_inst.layernorm_done(layernorm_done);
    gbdone_inst.zeropadding_done(zeropadding_done);    
    gbdone_inst.attention_done(attention_done);
    //gbcore_inst
    gbcore_inst.clk                   (clk);
    gbcore_inst.rst                   (rst);
    gbcore_inst.rva_in_large(gbcore_large_rva_in);
    gbcore_inst.rva_out_large(gbcore_large_rva_out);
    gbcore_inst.rva_in_small(gbcore_small_rva_in);
    gbcore_inst.rva_out_small(gbcore_small_rva_out);
        
    gbcore_inst.gbcontrol_large_req   (gbcontrol_large_req  );
    gbcore_inst.gbcontrol_large_rsp   (gbcontrol_large_rsp  );  
    gbcore_inst.gbcontrol_small_req   (gbcontrol_small_req  );
    gbcore_inst.gbcontrol_small_rsp   (gbcontrol_small_rsp  );
    gbcore_inst.layerreduce_large_req (layerreduce_large_req);
    gbcore_inst.layerreduce_large_rsp (layerreduce_large_rsp);  
    gbcore_inst.layernorm_large_req   (layernorm_large_req  );
    gbcore_inst.layernorm_large_rsp   (layernorm_large_rsp  );  
    gbcore_inst.layernorm_small_req   (layernorm_small_req  );
    gbcore_inst.layernorm_small_rsp   (layernorm_small_rsp  );
    gbcore_inst.zeropadding_large_req (zeropadding_large_req);
    gbcore_inst.zeropadding_large_rsp (zeropadding_large_rsp);  
    
    gbcore_inst.attention_large_req   (attention_large_req);
    gbcore_inst.attention_large_rsp   (attention_large_rsp);      
    gbcore_inst.attention_small_req   (attention_small_req);
    gbcore_inst.attention_small_rsp   (attention_small_rsp); 
      
    gbcore_inst.SC_SRAM_CONFIG(SC_SRAM_CONFIG);
    
    //gbcontrol_inst
    gbcontrol_inst.clk        (clk); 
    gbcontrol_inst.rst        (rst); 
    gbcontrol_inst.rva_in     (gbcontrol_rva_in);
    gbcontrol_inst.rva_out    (gbcontrol_rva_out);
    gbcontrol_inst.start      (gbcontrol_start);
    gbcontrol_inst.done       (gbcontrol_done);
    gbcontrol_inst.large_req  (gbcontrol_large_req);
    gbcontrol_inst.large_rsp  (gbcontrol_large_rsp);
    gbcontrol_inst.small_req  (gbcontrol_small_req);
    gbcontrol_inst.small_rsp  (gbcontrol_small_rsp);
    gbcontrol_inst.data_out   (data_out);
    gbcontrol_inst.data_in    (data_in);
    gbcontrol_inst.pe_start   (pe_start);
    gbcontrol_inst.pe_done    (pe_done);
    
    //layerreduce_inst
    layerreduce_inst.clk      (clk);
    layerreduce_inst.rst      (rst);
    layerreduce_inst.rva_in   (layerreduce_rva_in);
    layerreduce_inst.rva_out  (layerreduce_rva_out);
    layerreduce_inst.start    (layerreduce_start);
    layerreduce_inst.done     (layerreduce_done);
    layerreduce_inst.large_req(layerreduce_large_req);
    layerreduce_inst.large_rsp(layerreduce_large_rsp);  
        
    //layernorm_inst
    layernorm_inst.clk        (clk);
    layernorm_inst.rst        (rst);
    layernorm_inst.rva_in     (layernorm_rva_in);
    layernorm_inst.rva_out    (layernorm_rva_out);
    layernorm_inst.start      (layernorm_start);
    layernorm_inst.done       (layernorm_done);
    layernorm_inst.large_req  (layernorm_large_req);
    layernorm_inst.large_rsp  (layernorm_large_rsp);  
    layernorm_inst.small_req  (layernorm_small_req);
    layernorm_inst.small_rsp  (layernorm_small_rsp);      
        
    //zeropadding_inst
    zeropadding_inst.clk        (clk);
    zeropadding_inst.rst        (rst);
    zeropadding_inst.rva_in     (zeropadding_rva_in);
    zeropadding_inst.rva_out    (zeropadding_rva_out);  
    zeropadding_inst.start      (zeropadding_start);
    zeropadding_inst.done       (zeropadding_done);
    zeropadding_inst.large_req  (zeropadding_large_req);
    zeropadding_inst.large_rsp  (zeropadding_large_rsp);  
    
    attention_inst.clk        (clk);
    attention_inst.rst        (rst);
    attention_inst.rva_in     (attention_rva_in);
    attention_inst.rva_out    (attention_rva_out);
    attention_inst.start      (attention_start);
    attention_inst.done       (attention_done);
    attention_inst.large_req  (attention_large_req);
    attention_inst.large_rsp  (attention_large_rsp);
    attention_inst.small_req  (attention_small_req);
    attention_inst.small_rsp  (attention_small_rsp);
  }
  
};

#endif
