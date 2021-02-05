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


#ifndef __PEPARTITION__
#define __PEPARTITION__

#include <systemc.h>
#include <nvhls_int.h>
#include <nvhls_types.h>
#include <nvhls_vector.h>
#include <nvhls_module.h>

#include "SM6Spec.h"
#include "AxiSpec.h"

#include "PEModule/PEModule.h"


SC_MODULE(PEPartition) {
  static const int kDebugLevel = 3;
 public:
  sc_in<bool>  clk;
  sc_in<bool>  rst; 
 
  // AXI slave read write
  // Update 02152020 for matching new AXI format
  //typename spec::Axi::axi4_::read::slave   if_axi_rd;
  //typename spec::Axi::axi4_::write::slave  if_axi_wr;
  typename spec::Axi::axi4_::read::template slave<>   if_axi_rd;
  typename spec::Axi::axi4_::write::template slave<>  if_axi_wr;
  
  Connections::In<spec::StreamType>     input_port;     
  Connections::Out<spec::StreamType>    output_port; 
  Connections::In<bool>                 start;
  Connections::Out<bool>                done;  

  Connections::Combinational<spec::Axi::SlaveToRVA::Write>     rva_in;
  Connections::Combinational<spec::Axi::SlaveToRVA::Read>      rva_out;
  
  
  PEModule                pemodule_inst;
  spec::Axi::SlaveToRVA   rva_inst;
      
  SC_HAS_PROCESS(PEPartition);
  PEPartition(sc_module_name name)
     : sc_module(name), 
     clk("clk"),
     rst("rst"),
     if_axi_rd("if_axi_rd"),
     if_axi_wr("if_axi_wr"),
     pemodule_inst("pemodule_inst"),
     rva_inst  ("rva_inst")
  {
    rva_inst.clk(clk);
    rva_inst.reset_bar(rst);
    rva_inst.if_axi_rd(if_axi_rd);
    rva_inst.if_axi_wr(if_axi_wr);
    rva_inst.if_rv_rd(rva_out);
    rva_inst.if_rv_wr(rva_in);
    
    pemodule_inst.clk(clk);
    pemodule_inst.rst(rst);
    pemodule_inst.rva_in(rva_in);
    pemodule_inst.rva_out(rva_out);
    pemodule_inst.input_port(input_port);          
    pemodule_inst.output_port(output_port);
    pemodule_inst.start(start);
    pemodule_inst.done(done);  
  }      
  
};



#endif 
