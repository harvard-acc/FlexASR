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

#include <stdio.h>
#include <systemc.h>
#include <mc_scverify.h>
#include <match_scverify.h>
#include "AdpfloatSpec.h"
#include "AdpfloatUtils.h"
#include "SM6Spec.h"
#include "Datapath.h"
#include "AdpfloatUtils.h"
#include <bitset>
#include <testbench/nvhls_rand.h>

//#include <math.h>

CCS_MAIN (int argc, char *argv[]) {
  nvhls::set_random_seed();
  
  spec::AdpfloatBiasType adpbias_weight = 3;
  spec::AdpfloatBiasType adpbias_input = 4;
    
  AdpfloatType<8, 3> adpfloat;

  //cout << adpfloat << endl;
  
  spec::VectorType dp_weight[spec::kNumVectorLanes];
  spec::VectorType dp_input;
  //spec::AccumScalarType dp_output[spec::kNumVectorLanes];
  spec::AccumVectorType dp_output;
  double dp_output_float[spec::kNumVectorLanes];
    
  float ref_weight[spec::kNumVectorLanes][spec::kVectorSize];
  float ref_input[spec::kVectorSize];
  float ref_output[spec::kNumVectorLanes];
  



  //cout << endl << "Reference Weight" << endl; 
  for (int i = 0; i < spec::kNumVectorLanes; i++) {
    for (int j = 0; j < spec::kVectorSize; j++) {
      dp_weight[i][j] = nvhls::get_rand<spec::kAdpfloatWordWidth>();
      if (j % 5 == 0) {
        dp_weight[i][j] = 0;
      }
      //cout << std::bitset<8>(dp_weight[i][j].to_int()) << "\t";
      AdpfloatType<8, 3> tmp(dp_weight[i][j]);
      ref_weight[i][j] = tmp.to_float(adpbias_weight);
      //cout << ref_weight[i][j] << endl;
    }
    //cout << "================================" << endl;
  }

  // FIXME: might gerenate 10000000 illegal(?) term
  //cout << endl << "Reference Input" << endl; 
  for (int i = 0; i < spec::kVectorSize; i++) {
    dp_input[i] = nvhls::get_rand<spec::kAdpfloatWordWidth>();
    if (i % 4 == 0) {
      dp_input[i] = 0;
    }    
    
    
    //cout << std::bitset<8>(dp_input[i].to_int()) << "\t";
    AdpfloatType<8, 3> tmp(dp_input[i]);
    ref_input[i] = tmp.to_float(adpbias_input);
    //cout << ref_input[i] << endl;
  }
  //cout << "================================" << endl;

  

  // get reference output 
  //cout << endl << "Reference Output" << endl; 
  for (int i = 0; i < spec::kNumVectorLanes; i++) {
    ref_output[i] = 0;
    for (int j = 0; j < spec::kVectorSize; j++) {
       ref_output[i] += ref_weight[i][j]*ref_input[j];
    }
    //cout << ref_output[i] << endl;
  }
  //cout << "================================" << endl;
  //cout << sc_time() << endl;
  //NVUINTW(spec::kNumVectorLanes) iszero_out;
  CCS_DESIGN(Datapath)(dp_weight,dp_input,dp_output);
  //cout << sc_time() << endl;
  
  
  
  cout << endl << "DP Output\t" << "Ref Output" << endl; 
  const int tmp_pwr = 2*spec::kAdpfloatOffset + adpbias_input + adpbias_weight - 2*spec::kAdpfloatManWidth;
  //cout << "tmp_pwr: " << tmp_pwr << endl;
  double scale = pow(2, tmp_pwr);
  //cout << "scale: " << scale << endl;
  for (int i = 0; i < spec::kNumVectorLanes; i++) {
    //cout << std::bitset<32>(dp_output[i].to_int64()) << "\t";
    dp_output_float[i] = dp_output[i];
    dp_output_float[i] = dp_output_float[i]*scale;
    cout << dp_output_float[i] << "  \t" << ref_output[i] << endl;
    assert( abs(dp_output_float[i] - ref_output[i]) < 1E-9 );
  }  
  
  CCS_RETURN(0);
}
