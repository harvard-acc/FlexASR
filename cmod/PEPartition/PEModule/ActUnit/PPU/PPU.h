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

#ifndef PPU_H
#define PPU_H

#include <systemc.h>
#include <nvhls_int.h>
#include <nvhls_types.h>
#include <nvhls_vector.h>
#include <nvhls_module.h>

#include <ac_int.h>
#include <ac_fixed.h>
#include <ac_math.h>
#include <ac_math/ac_sigmoid_pwl.h>
#include <ac_math/ac_tanh_pwl.h>
#include <ac_math/ac_div.h>
#include <ac_math/ac_pow_pwl.h>
#include <ac_math/ac_inverse_sqrt_pwl.h>
#include "SM6Spec.h"
#include "AdpfloatUtils.h"
#include "AdpfloatSpec.h"


// Important Update: change everything to vector type
// spec::kNumVectorLanes
// spec::ActVectorType
// spec::VectorType
//       #pragma hls_unroll yes

#pragma hls_design ccore
#pragma hls_ccore_type combinational
void Sigmoid (const spec::ActVectorType in, spec::ActVectorType& out)
{
  spec::ActVectorType out_tmp;
  #pragma hls_unroll yes
  for (int i = 0; i < spec::kNumVectorLanes; i++) {
    ac_fixed<spec::kActWordWidth, spec::kActNumInt, true, AC_TRN, AC_WRAP> in_ac; 
    ac_fixed<spec::kActWordWidth, spec::kActNumInt, false, AC_TRN, AC_WRAP> out_ac;

    in_ac.set_slc(0, in[i]);
   
    out_ac = ac_math::ac_sigmoid_pwl
            <ac_fixed<spec::kActWordWidth, spec::kActNumInt, false, AC_TRN, AC_WRAP> >(in_ac); 
      
          
    out_tmp[i].set_slc(0, nvhls::get_slc<spec::kActWordWidth>(out_ac, 0));
  }
  out = out_tmp;
}

#pragma hls_design ccore
#pragma hls_ccore_type combinational
void Tanh (const spec::ActVectorType in, spec::ActVectorType& out)
{
  spec::ActVectorType out_tmp;
  #pragma hls_unroll yes
  for (int i = 0; i < spec::kNumVectorLanes; i++) {
    ac_fixed<spec::kActWordWidth, spec::kActNumInt, true, AC_TRN, AC_WRAP> in_ac; 
    ac_fixed<spec::kActWordWidth, spec::kActNumInt, false, AC_TRN, AC_WRAP> out_ac;

    in_ac.set_slc(0, in[i]);

    out_ac = ac_math::ac_tanh_pwl
            <ac_fixed<spec::kActWordWidth, spec::kActNumInt, false, AC_TRN, AC_WRAP> >(in_ac);

    out_tmp[i].set_slc(0, nvhls::get_slc<spec::kActWordWidth>(out_ac, 0));
  }
  out = out_tmp;
}


#pragma hls_design ccore
#pragma hls_ccore_type combinational
void EMul (const spec::ActVectorType in_1, const spec::ActVectorType in_2, spec::ActVectorType& out) {
  spec::ActVectorType out_tmp;
  #pragma hls_unroll yes
  for (int i = 0; i < spec::kNumVectorLanes; i++) {  
    NVINTW(2*spec::kActWordWidth) tmp;
    tmp = in_1[i]*in_2[i];
    tmp = nvhls::right_shift<NVINTW(2*spec::kActWordWidth)>(tmp, spec::kActNumFrac);
    out_tmp[i] = nvhls::get_slc<spec::kActWordWidth>(tmp, 0);
  }
  out = out_tmp;  
  // TODO: overflow checking ?
}

#pragma hls_design ccore
#pragma hls_ccore_type combinational
void EAdd (const spec::ActVectorType in_1, const spec::ActVectorType in_2, spec::ActVectorType& out)  {
  spec::ActVectorType out_tmp; 
  #pragma hls_unroll yes
  for (int i = 0; i < spec::kNumVectorLanes; i++) {  
    out_tmp[i] = in_1[i] + in_2[i];
  }  
  // TODO: overflow checking ?
  out = out_tmp;    
}

#pragma hls_design ccore
#pragma hls_ccore_type combinational
void Relu (const spec::ActVectorType in, spec::ActVectorType& out) {
  spec::ActVectorType out_tmp;   
  #pragma hls_unroll yes
  for (int i = 0; i < spec::kNumVectorLanes; i++) {  
    if (in[i] < 0) out_tmp[i] = 0;
    else out_tmp[i] = in[i];
  }
  out = out_tmp;
}  

#pragma hls_design ccore
#pragma hls_ccore_type combinational
void OneX (const spec::ActVectorType in, spec::ActVectorType& out) {
  spec::ActVectorType out_tmp;     
  #pragma hls_unroll yes
  for (int i = 0; i < spec::kNumVectorLanes; i++) {  
    out_tmp[i] = (1<<spec::kActNumFrac) - in[i];
  }
  out = out_tmp;
}

#pragma hls_design ccore
#pragma hls_ccore_type combinational
void VSum (const spec::ActVectorType in, spec::ActScalarType& out) {
  spec::ActScalarType out_tmp = 0;;         
  #pragma hls_unroll yes
  for (int i = 0; i < spec::kNumVectorLanes; i++) {
    out_tmp += in[i];
  }     
  out = out_tmp;  
}

#pragma hls_design ccore
#pragma hls_ccore_type combinational
void Fixed2Adpfloat(const spec::ActVectorType in, spec::VectorType& out, const AdpfloatBiasType adpfloat_bias) {
  spec::VectorType out_tmp;
  #pragma hls_unroll yes
  for (int i = 0; i < spec::kNumVectorLanes; i++) {  
    AdpfloatType<spec::kAdpfloatWordWidth, spec::kAdpfloatExpWidth> out_adpfloat; 
    out_adpfloat.set_value_fixed<spec::kActWordWidth, spec::kActNumFrac>(in[i], adpfloat_bias);
    out_tmp[i] = out_adpfloat.to_rawbits();  
  }
  out = out_tmp;    
}

// Scalar Square Root Inverse
#pragma hls_design ccore
#pragma hls_ccore_type combinational
void SInvSqrt(const spec::ActScalarType in, spec::ActScalarType& out) {
  spec::ActScalarType out_tmp;
  
  ac_fixed<spec::kActWordWidth, spec::kActNumInt, false, AC_TRN, AC_WRAP> in_ac; 
  ac_fixed<spec::kActWordWidth, spec::kActNumInt, false, AC_TRN, AC_WRAP> out_ac;  
  in_ac.set_slc(0, in);
  ac_math::ac_inverse_sqrt_pwl(in_ac, out_ac);
  out_tmp.set_slc(0, nvhls::get_slc<spec::kActWordWidth>(out_ac, 0));
  
  out = out_tmp;
}


#pragma hls_design ccore
#pragma hls_ccore_type combinational
void Adpfloat2Fixed(const spec::VectorType in, spec::ActVectorType& out, const AdpfloatBiasType adpfloat_bias){
  spec::ActVectorType out_tmp;
  #pragma hls_unroll yes
  for (int i = 0; i < spec::kNumVectorLanes; i++) {  
    AdpfloatType<spec::kAdpfloatWordWidth, spec::kAdpfloatExpWidth> input_adpfloat(in[i]);
    out_tmp[i] = input_adpfloat.to_fixed<spec::kActWordWidth, spec::kActNumFrac>(adpfloat_bias);  
  }
  
  out = out_tmp;  
}

#endif
