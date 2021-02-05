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

#ifndef GBCORE_H
#define GBCORE_H

#include <systemc.h>
#include <nvhls_int.h>
#include <nvhls_types.h>
#include <nvhls_vector.h>
#include <nvhls_module.h>

#include "GBSpec.h"
#include "SM6Spec.h"
#include "AxiSpec.h"
#include <arbitrated_crossbar.h>
#include <ArbitratedScratchpadDP.h>

// Update 0318 Two Buffers have seperate CThreads

class GBCore : public match::Module {
  static const int kDebugLevel = 4;
  SC_HAS_PROCESS(GBCore);

  // GBCore Config
  // AXI Config For Large Buffer
  NVUINT8   num_vector_large[spec::GB::Large::kMaxNumManagers]; 
  NVUINT16  base_large[spec::GB::Large::kMaxNumManagers];    // this should be 4
  
  // AXI Config For Small Buffer  
  NVUINT16  base_small[spec::GB::Small::kMaxNumManagers];    // this should be 8  

  spec::GB::Large::Address    large_read_addrs           [spec::GB::Large::kNumReadPorts];
  bool                        large_read_req_valid       [spec::GB::Large::kNumReadPorts]; 
  spec::GB::Large::Address    large_write_addrs          [spec::GB::Large::kNumWritePorts];
  bool                        large_write_req_valid      [spec::GB::Large::kNumWritePorts];
  spec::GB::Large::WordType   large_write_data           [spec::GB::Large::kNumWritePorts];
  bool                        large_read_ack             [spec::GB::Large::kNumReadPorts]; 
  bool                        large_write_ack            [spec::GB::Large::kNumWritePorts];
  bool                        large_read_ready           [spec::GB::Large::kNumReadPorts];
  spec::GB::Large::WordType   large_port_read_out        [spec::GB::Large::kNumReadPorts];
  bool                        large_port_read_out_valid  [spec::GB::Large::kNumReadPorts];
  

  spec::GB::Small::Address    small_read_addrs            [spec::GB::Small::kNumReadPorts]; 
  bool                        small_read_req_valid        [spec::GB::Small::kNumReadPorts];     
  spec::GB::Small::Address    small_write_addrs           [spec::GB::Small::kNumWritePorts];
  bool                        small_write_req_valid       [spec::GB::Small::kNumWritePorts];
  spec::GB::Small::WordType   small_write_data            [spec::GB::Small::kNumWritePorts];
  bool                        small_read_ack              [spec::GB::Small::kNumReadPorts]; 
  bool                        small_write_ack             [spec::GB::Small::kNumWritePorts];
  bool                        small_read_ready            [spec::GB::Small::kNumReadPorts];
  spec::GB::Small::WordType   small_port_read_out         [spec::GB::Small::kNumReadPorts];
  bool                        small_port_read_out_valid   [spec::GB::Small::kNumReadPorts];  


 public:
  
  Connections::In<spec::Axi::SlaveToRVA::Write>   rva_in_large;   
  Connections::Out<spec::Axi::SlaveToRVA::Read>   rva_out_large;
      
  Connections::In<spec::GB::Large::DataReq>       gbcontrol_large_req;
  Connections::Out<spec::GB::Large::DataRsp<1>>   gbcontrol_large_rsp;   
  Connections::In<spec::GB::Large::DataReq>       layerreduce_large_req;
  Connections::Out<spec::GB::Large::DataRsp<2>>   layerreduce_large_rsp;       
  Connections::In<spec::GB::Large::DataReq>       layernorm_large_req;
  Connections::Out<spec::GB::Large::DataRsp<1>>   layernorm_large_rsp;  
  Connections::In<spec::GB::Large::DataReq>       zeropadding_large_req;
  Connections::Out<spec::GB::Large::DataRsp<1>>   zeropadding_large_rsp;  
  Connections::In<spec::GB::Large::DataReq>       attention_large_req;
  Connections::Out<spec::GB::Large::DataRsp<16>>  attention_large_rsp;   
  
  Connections::In<spec::Axi::SlaveToRVA::Write>   rva_in_small; 
  Connections::Out<spec::Axi::SlaveToRVA::Read>   rva_out_small;  
  Connections::In<spec::GB::Small::DataReq>       gbcontrol_small_req;
  Connections::Out<spec::GB::Small::DataRsp>      gbcontrol_small_rsp;    
  Connections::In<spec::GB::Small::DataReq>       layernorm_small_req;
  Connections::Out<spec::GB::Small::DataRsp>      layernorm_small_rsp;   

  Connections::In<spec::GB::Small::DataReq>       attention_small_req;
  Connections::Out<spec::GB::Small::DataRsp>      attention_small_rsp;   

    
  // Access only by the larger buffer thread
  sc_in<NVUINT32> SC_SRAM_CONFIG;
  
  
  // Constructor
  GBCore (sc_module_name nm)
      : match::Module(nm),
        rva_in_large("rva_in_large"),
        rva_out_large("rva_out_large"),
        
        gbcontrol_large_req   ("gbcontrol_large_req"),
        gbcontrol_large_rsp   ("gbcontrol_large_rsp"),
        layerreduce_large_req ("layerreduce_large_req"),
        layerreduce_large_rsp ("layerreduce_large_rsp"),
        layernorm_large_req   ("layernorm_large_req"),
        layernorm_large_rsp   ("layernorm_large_rsp"),         
       
        zeropadding_large_req ("zeropadding_large_req"),        
        zeropadding_large_rsp ("zeropadding_large_rsp"),
        attention_large_req   ("attention_large_req"),
        attention_large_rsp   ("attention_large_rsp"),


        rva_in_small          ("rva_in_small"),
        rva_out_small         ("rva_out_small"),
        gbcontrol_small_req   ("gbcontrol_small_req"),
        gbcontrol_small_rsp   ("gbcontrol_small_rsp"),                                                        
        layernorm_small_req   ("layernorm_small_req"),
        layernorm_small_rsp   ("layernorm_small_rsp"),  
        attention_small_req   ("attention_small_req"),
        attention_small_rsp   ("attention_small_rsp"), 
                               
        SC_SRAM_CONFIG        ("SC_SRAM_CONFIG")
  {
    SC_THREAD(LargeRun);
    sensitive << clk.pos();
    async_reset_signal_is(rst, false);
      
    SC_THREAD(SmallRun);
    sensitive << clk.pos();
    async_reset_signal_is(rst, false);
  }  

/*** B. GBCore state (including SRAMs) ***/
  ArbitratedScratchpadDP<spec::GB::Large::kNumBanks,      
                         spec::GB::Large::kNumReadPorts,  
                         spec::GB::Large::kNumWritePorts, 
                         spec::GB::Large::kEntriesPerBank, 
                         spec::GB::Large::WordType, 
                         false, true> large_mem;            // single port SRAM

  ArbitratedScratchpadDP<spec::GB::Small::kNumBanks,    
                         spec::GB::Small::kNumReadPorts, 
                         spec::GB::Small::kNumWritePorts,
                         spec::GB::Small::kEntriesPerBank, 
                         spec::GB::Small::WordType, 
                         false, true> small_mem;            // single port SRAM  

  // ArbitratedCrossbar<spec::GB::Large::DataReq, 5, 1, 0, 0> arbxbar_large;

  // this N should matche the number of read 
  template<unsigned N>
  inline void SetLargeBuffer(const spec::GB::Large::DataReq large_req_reg) {
    NVUINT3                     memory_index = large_req_reg.memory_index;
    NVUINT8                     vector_index = large_req_reg.vector_index;
    NVUINT16                    timestep_index = large_req_reg.timestep_index;    
    spec::GB::Large::WordType   write_data = large_req_reg.write_data;
  
    NVUINT4   lower_timestep_index = nvhls::get_slc<4>(timestep_index, 0);
    NVUINT12  upper_timestep_index = nvhls::get_slc<12>(timestep_index, 4);  
    
    spec::GB::Large::Address base_addr = 
                                base_large[memory_index] + lower_timestep_index + 
                                (upper_timestep_index*num_vector_large[memory_index] + vector_index)*16;
    if (large_req_reg.is_write) {
      large_write_addrs         [0] = base_addr;
      //cout << "write_address in GBCore: " << base_addr << endl;
      large_write_req_valid     [0] = 1;
      large_write_data          [0] = write_data;
    }
    else {
      #pragma hls_unroll yes 
      for (unsigned i = 0; i < N; i++) { 
        large_read_addrs          [i] = base_addr + i;
        large_read_req_valid      [i] = 1;   
        large_read_ready          [i] = 1;
      }
    }
  }
 
  
  inline void SetSmallBuffer(const spec::GB::Small::DataReq small_req_reg) {
    NVUINT3                     memory_index = small_req_reg.memory_index;
    NVUINT8                     vector_index = small_req_reg.vector_index;
    spec::GB::Small::WordType   write_data = small_req_reg.write_data;
    
    spec::GB::Small::Address base_addr = vector_index + base_small[memory_index];
    if (small_req_reg.is_write) {
      small_write_addrs         [0] = base_addr;
      small_write_req_valid     [0] = 1;
      small_write_data          [0] = write_data;    
    }
    else {
      small_read_addrs          [0] = base_addr; 
      small_read_req_valid      [0] = 1;  
      small_read_ready          [0] = 1;  
    }    
  }
  
  void LargeRun() {
    rva_in_large.Reset();
    rva_out_large.Reset();
    gbcontrol_large_req.Reset();   
    gbcontrol_large_rsp.Reset();   
    layerreduce_large_req.Reset();     
    layerreduce_large_rsp.Reset();       
    layernorm_large_req.Reset();       
    layernorm_large_rsp.Reset();  
    zeropadding_large_req.Reset();     
    zeropadding_large_rsp.Reset();        
    attention_large_req.Reset();
    attention_large_rsp.Reset();
    
    #pragma hls_unroll yes    
    for (int i = 0; i < spec::GB::Large::kMaxNumManagers; i++) {
      num_vector_large[i] = 1; 
      base_large[i]        = 0;
    }

    #pragma hls_pipeline_init_interval 1
    while(1) {
      bool is_axi = 0;
      //bool is_axi_rsp = 0;
      NVUINT4 rsp_mode = 0; 
      spec::Axi::SlaveToRVA::Write rva_in_reg;
      spec::Axi::SlaveToRVA::Read rva_out_reg;          

      #pragma hls_unroll yes 
      for (unsigned i = 0; i < spec::GB::Large::kNumReadPorts; i++) { 
        large_read_addrs          [i] = 0;
        large_read_req_valid      [i] = 0;   
        large_read_ready          [i] = 0;
      }
      large_write_addrs         [0] = 0;
      large_write_req_valid     [0] = 0;
      large_write_data          [0] = 0;
       

      if (rva_in_large.PopNB(rva_in_reg)) {
        is_axi = 1;
        NVUINT4     tmp = nvhls::get_slc<4>(rva_in_reg.addr, 20);
        NVUINT16    local_index = nvhls::get_slc<16>(rva_in_reg.addr, 4);
        if(rva_in_reg.rw) {
          CDCOUT(sc_time_stamp()  << " GBCore Large: " << name() << "RVA Write " << endl, kDebugLevel);    

          switch (tmp) {
            case 0x4: {    
              if (local_index == 0x01) {
                #pragma hls_unroll yes    
                for (int i = 0; i < spec::GB::Large::kMaxNumManagers; i++) {
                  num_vector_large[i] = nvhls::get_slc<8>(rva_in_reg.data, 32*i);
                  base_large[i]       = nvhls::get_slc<16>(rva_in_reg.data, 32*i+16);
                }
              }
              break;
            }
            case 0x5: {    
              large_write_addrs         [0] = local_index;
              large_write_req_valid     [0] = 1;
              large_write_data          [0] = rva_in_reg.data; 
              break;
            }
            default: {
              break;
            }
          }          
        }
        else {
          CDCOUT(sc_time_stamp()  << " GBCore Large: " << name() << "RVA Read " << endl, kDebugLevel);
          //is_axi_rsp = 1;
          rva_out_reg.data = 0; 
          switch (tmp) {
          case 0x3: {
            rva_out_reg.data = SC_SRAM_CONFIG.read();
            rsp_mode = 0x3;            
            break;
          }
          case 0x4: {    
            if (local_index == 0x01) {
              #pragma hls_unroll yes    
              for (int i = 0; i < spec::GB::Large::kMaxNumManagers; i++) {
                rva_out_reg.data.set_slc<8>(32*i, num_vector_large[i]);
                rva_out_reg.data.set_slc<16>(32*i+16, base_large[i]);
              }
            }
            rsp_mode = 0x4;  
            break;          
          }
          case 0x5: {    
            large_read_addrs          [0] = local_index;
            large_read_req_valid      [0] = 1;   
            large_read_ready          [0] = 1;
            rsp_mode = 0x5;
            break;
          }
          default: {
            break;
          }
          }        
        }
      }      

// Change this part to Arxbar, If no axi, check streaming request  
// TODO The req should be changed to array form 
      // 1. PopNB list
      NVUINT5 valid_regs = 0; 
      NVUINT3 pos = 0;
      spec::GB::Large::DataReq large_req_regs[5];   
      if (is_axi == 0) {     
        valid_regs[0] = gbcontrol_large_req.  PopNB(large_req_regs[0]);
        valid_regs[1] = layerreduce_large_req.PopNB(large_req_regs[1]);
        valid_regs[2] = layernorm_large_req.  PopNB(large_req_regs[2]);
        valid_regs[3] = zeropadding_large_req.PopNB(large_req_regs[3]);
        valid_regs[4] = attention_large_req.  PopNB(large_req_regs[4]);
               
      // 2. leading one detect
        pos = nvhls::leading_ones<5, NVUINT5, NVUINT3>(valid_regs); 
      }
     
      if (valid_regs != 0) {
        spec::GB::Large::DataReq large_req_reg = large_req_regs[pos];
        switch(pos){ 
          case 0:
            SetLargeBuffer<1>(large_req_reg);
            if (!large_req_reg.is_write) {
              rsp_mode = 0x7;      
            }          
            break;
          case 1:
            SetLargeBuffer<2>(large_req_reg);
            if (!large_req_reg.is_write) {
              rsp_mode = 0x8;      
            }                  
            break;
          case 2:
            SetLargeBuffer<1>(large_req_reg);
            if (!large_req_reg.is_write) {
              rsp_mode = 0x9;      
            }           
            break;
          case 3:
            SetLargeBuffer<1>(large_req_reg);
            if (!large_req_reg.is_write) {
              rsp_mode = 0xA;      
            }            
            break;      
          case 4:
            SetLargeBuffer<16>(large_req_reg);   
            if (!large_req_reg.is_write) {
              rsp_mode = 0xB;
            }          
            break;
          default:        
            break;          
        }
      }
    
      large_mem.run(
        large_read_addrs          , 
        large_read_req_valid      ,     
        large_write_addrs         ,
        large_write_req_valid     ,
        large_write_data          ,
        large_read_ack            ,
        large_write_ack           ,
        large_read_ready          ,
        large_port_read_out       ,
        large_port_read_out_valid 
      );      
      

      switch (rsp_mode) {
        case 0x3:
        case 0x4: {
          rva_out_large.Push(rva_out_reg);        
          break;
        }
        case 0x5: {
          rva_out_reg.data = large_port_read_out[0].to_rawbits();
          rva_out_large.Push(rva_out_reg);
          break;        
        }
        case 0x7: { // GBControl
          spec::GB::Large::DataRsp<1>  large_rsp_reg;
          large_rsp_reg.read_vector[0] = large_port_read_out[0];
          gbcontrol_large_rsp.Push(large_rsp_reg);
          break;
        }
        case 0x8: { // LayerReduce
          spec::GB::Large::DataRsp<2>  large_rsp_reg;
          large_rsp_reg.read_vector[0] = large_port_read_out[0];
          large_rsp_reg.read_vector[1] = large_port_read_out[1];
          layerreduce_large_rsp.Push(large_rsp_reg);
          break;
        }
        case 0x9: { // LayerNorm
          spec::GB::Large::DataRsp<1>  large_rsp_reg;
          large_rsp_reg.read_vector[0] = large_port_read_out[0];
          layernorm_large_rsp.Push(large_rsp_reg);  
          break;
        }
        case 0xA: { // ZeroPadding
          spec::GB::Large::DataRsp<1>  large_rsp_reg;        
          large_rsp_reg.read_vector[0] = large_port_read_out[0];
          zeropadding_large_rsp.Push(large_rsp_reg);
          break;
        }
        case 0xB: {// TODO attention start 
          spec::GB::Large::DataRsp<16>  large_rsp_reg;                            
          #pragma hls_unroll yes
          for (int i = 0; i < 16; i++) {
            large_rsp_reg.read_vector[i] = large_port_read_out[i];
          }
          attention_large_rsp.Push(large_rsp_reg);
          break; 
        }
        default: {
          break;  
        }
      }    

      wait();
    }
  }


  void SmallRun() {
    spec::Axi::SlaveToRVA::Write rva_in_reg;
    spec::Axi::SlaveToRVA::Read rva_out_reg;          
    spec::GB::Small::DataReq small_req_reg;
      
    rva_in_small.Reset();
    rva_out_small.Reset();
    gbcontrol_small_req.Reset();   
    gbcontrol_small_rsp.Reset();       
    layernorm_small_req.Reset();       
    layernorm_small_rsp.Reset();   
    attention_small_req.Reset(); 
    attention_small_rsp.Reset(); 
    
    #pragma hls_unroll yes    
    for (int i = 0; i < spec::GB::Small::kMaxNumManagers; i++) {    
      base_small[i]        = 0;
    }  

    #pragma hls_pipeline_init_interval 1    
    while(1) {
      NVUINT4 rsp_mode = 0;  
      bool is_axi = 0;    
      small_read_addrs          [0] = 0; 
      small_read_req_valid      [0] = 0;  
      small_read_ready          [0] = 0;  
      small_write_addrs         [0] = 0;
      small_write_req_valid     [0] = 0;
      small_write_data          [0] = 0;


      
      if (rva_in_small.PopNB(rva_in_reg)) {
        NVUINT4     tmp = nvhls::get_slc<4>(rva_in_reg.addr, 20);
        NVUINT16    local_index = nvhls::get_slc<16>(rva_in_reg.addr, 4);
        is_axi = 1;
        if(rva_in_reg.rw) {
          CDCOUT(sc_time_stamp()  << " GBCore Small: " << name() << "RVA Write " << endl, kDebugLevel);    
          //NVUINT1 preload_idx = !pe_config.active_idx;

          switch (tmp) {
            case 0x4: {    
              if (local_index == 0x02) {
                #pragma hls_unroll yes    
                for (int i = 0; i < spec::GB::Small::kMaxNumManagers; i++) {
                  base_small[i]        = nvhls::get_slc<16>(rva_in_reg.data, 16*i);
                } 
              }
              break;
            }
            case 0x6: {    
              small_write_addrs         [0] = local_index;
              small_write_req_valid     [0] = 1;
              small_write_data          [0] = rva_in_reg.data; 
              break;
            }
            default: {
              break;
            }
          }          
        }
        else {
          CDCOUT(sc_time_stamp()  << " GBCore Small: " << name() << "RVA Read " << endl, kDebugLevel);
          rva_out_reg.data = 0;  
          switch (tmp) {
            case 0x4: {    
              if (local_index == 0x02) {
                #pragma hls_unroll yes    
                for (int i = 0; i < spec::GB::Small::kMaxNumManagers; i++) {
                  rva_out_reg.data.set_slc<16>(16*i, base_small[i]);
                }
              }
              rsp_mode = 0x4; 
              break;           
            }
            case 0x6: {    
              small_read_addrs          [0] = local_index;
              small_read_req_valid      [0] = 1;   
              small_read_ready          [0] = 1;
              rsp_mode = 0x6;
              break;
            }
            default: {
              break;
            }
          }        
        }
      }
      
      NVUINT3 valid_regs = 0; 
      NVUINT3 pos = 0;
      spec::GB::Small::DataReq small_req_regs[5];         
      if (is_axi == 0) {
        valid_regs[0] = gbcontrol_small_req.  PopNB(small_req_regs[0]);
        valid_regs[1] = layernorm_small_req.  PopNB(small_req_regs[1]);
        valid_regs[2] = attention_small_req.  PopNB(small_req_regs[2]);
               
      // 2. leading one detect
        pos = nvhls::leading_ones<3, NVUINT3, NVUINT3>(valid_regs); 
      }
      
      
      
      if (valid_regs != 0) {
        spec::GB::Small::DataReq small_req_reg = small_req_regs[pos];
        switch(pos){ 
          case 0:
            SetSmallBuffer(small_req_reg);
            if (!small_req_reg.is_write) {
              rsp_mode = 0x7;      
            }          
            break;
          case 1:
            SetSmallBuffer(small_req_reg);
            if (!small_req_reg.is_write) {
              rsp_mode = 0x9;      
            }           
            break;    
          case 2:
            SetSmallBuffer(small_req_reg);   
            if (!small_req_reg.is_write) {
              rsp_mode = 0xB;
            }          
            break;
          default:        
            break;          
        }
      }      
      
      small_mem.run(
        small_read_addrs          , 
        small_read_req_valid      ,     
        small_write_addrs         ,
        small_write_req_valid     ,
        small_write_data          ,
        small_read_ack            ,
        small_write_ack           ,
        small_read_ready          ,
        small_port_read_out       ,
        small_port_read_out_valid 
      );    


      switch (rsp_mode) {
        case 0x4: {
          rva_out_small.Push(rva_out_reg);        
          break;
        }
        case 0x6: {
          rva_out_reg.data = small_port_read_out[0].to_rawbits();
          rva_out_small.Push(rva_out_reg);        
          break;        
        }
        case 0x7: { // GBControl
          spec::GB::Small::DataRsp  small_rsp_reg;
          small_rsp_reg.read_data = small_port_read_out[0];
          gbcontrol_small_rsp.Push(small_rsp_reg);
          break;
        }
        case 0x9: { // LayerNorm
          spec::GB::Small::DataRsp  small_rsp_reg;
          small_rsp_reg.read_data = small_port_read_out[0];
          layernorm_small_rsp.Push(small_rsp_reg);  
          break;
        }
        case 0xB: { // TODO attention start 
          spec::GB::Small::DataRsp  small_rsp_reg;
          small_rsp_reg.read_data = small_port_read_out[0];
          attention_small_rsp.Push(small_rsp_reg);          
          break; 
        }
        default: {
          break;
        }
      }    
      
      wait();
    }
  
  } 
};
#endif 
