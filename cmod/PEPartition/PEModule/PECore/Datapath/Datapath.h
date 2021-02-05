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

#ifndef __DATAPATH__
#define __DATAPATH__

//  Datapath that does computation on adpfloat 
#include <nvhls_int.h>
#include <nvhls_types.h>
#include "SM6Spec.h"
#include "AdpfloatSpec.h"
#include "AdpfloatUtils.h"

// Zero skipping 
void ProductSum(const spec::VectorType in_1, const spec::VectorType in_2, spec::AccumScalarType& out) {
  spec::AccumScalarType out_tmp = 0; 
  
  #pragma hls_unroll yes
  #pragma cluster addtree 
  #pragma cluster_type both  
  for (int j = 0; j < spec::kVectorSize; j++) {
    AdpfloatType<8,3> in_1_adpfloat(in_1[j]);
    AdpfloatType<8,3> in_2_adpfloat(in_2[j]);
    spec::AccumScalarType acc_tmp;
    adpfloat_mul(in_1_adpfloat, in_2_adpfloat, acc_tmp);
    out_tmp += acc_tmp;
  }
  out = out_tmp;
}


void Datapath(spec::VectorType weight_in[spec::kNumVectorLanes], 
              spec::VectorType input_in,
              spec::AccumVectorType& accum_out)
{
  spec::AccumVectorType accum_out_tmp;            
  #pragma hls_unroll yes 
  for (int i = 0; i < spec::kNumVectorLanes; i++) {
    ProductSum(weight_in[i], input_in, accum_out_tmp[i]);
  }
  
  accum_out = accum_out_tmp;
}





#endif
