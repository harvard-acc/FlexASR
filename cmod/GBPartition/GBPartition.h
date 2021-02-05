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

#ifndef __GBPARTITION__
#define __GBPARTITION__

#include <systemc.h>
#include <nvhls_int.h>
#include <nvhls_types.h>
#include <nvhls_vector.h>
#include <nvhls_module.h>

#include "SM6Spec.h"
#include "AxiSpec.h"

#include "GBModule/GBModule.h"


SC_MODULE(GBPartition) {
  static const int kDebugLevel = 3;
 public:
  sc_in<bool>  clk;
  sc_in<bool>  rst; 
  

  Connections::Out<bool> done;
  
  // AXI slave read write
  typename spec::Axi::axi4_::read::template slave<>   if_axi_rd;
  typename spec::Axi::axi4_::write::template slave<>  if_axi_wr;
  
  //GBControl <-> PE
  Connections::In<spec::StreamType>   data_in;          
  Connections::Out<spec::StreamType>  data_out;
  Connections::Out<bool>              pe_start;
  Connections::In<bool>               pe_done;  
 
  Connections::Combinational<spec::Axi::SlaveToRVA::Write>     rva_in;
  Connections::Combinational<spec::Axi::SlaveToRVA::Read>      rva_out;
  
  GBModule                gbmodule_inst;
  spec::Axi::SlaveToRVA   rva_inst;
      
  SC_HAS_PROCESS(GBPartition);
  GBPartition(sc_module_name name)
     : sc_module(name), 
     clk("clk"),
     rst("rst"),
     if_axi_rd("if_axi_rd"),
     if_axi_wr("if_axi_wr"),
     gbmodule_inst("gbmodule_inst"),
     rva_inst  ("rva_inst")
  {
    rva_inst.clk(clk);
    rva_inst.reset_bar(rst);
    rva_inst.if_axi_rd(if_axi_rd);
    rva_inst.if_axi_wr(if_axi_wr);
    rva_inst.if_rv_rd(rva_out);
    rva_inst.if_rv_wr(rva_in);

    gbmodule_inst.clk(clk);
    gbmodule_inst.rst(rst);    
    gbmodule_inst.rva_in(rva_in);
    gbmodule_inst.rva_out(rva_out);
    gbmodule_inst.done(done);  
    gbmodule_inst.data_in(data_in);          
    gbmodule_inst.data_out(data_out);
    gbmodule_inst.pe_start(pe_start);
    gbmodule_inst.pe_done(pe_done);  
  }      
  
};



#endif 
