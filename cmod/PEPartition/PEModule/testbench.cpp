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


#include "PEModule.h"
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

#include <iostream>
#include <fstream>

#include "SM6Spec.h"
#include "AxiSpec.h"
#include "AdpfloatSpec.h"
#include "AdpfloatUtils.h"

#include "helper.h"

#include "../../testbench/libnpy/npy.hpp"

#define NVHLS_VERIFY_BLOCKS (PEModule)
#include <nvhls_verify.h>


#ifdef COV_ENABLE
   #pragma CTC SKIP
#endif

SC_MODULE(Source) {
  sc_in<bool> clk;
  sc_in<bool> rst;  
  Connections::Out<spec::StreamType> input_port;  
  Connections::Out<bool> start; 
  Connections::Out<spec::Axi::SlaveToRVA::Write> rva_in;

  bool start_src;
 
  SC_CTOR(Source) {
    SC_THREAD(run);
    sensitive << clk.pos();
    async_reset_signal_is(rst, false);
  }

  void run() {
    spec::Axi::SlaveToRVA::Write  rva_in_src;
    spec::AdpfloatBiasType adpbias_weight = 2; 
    spec::AdpfloatBiasType adpbias_bias = 2; 
    spec::AdpfloatBiasType adpbias_input = 1; 

    std::string filename;
    std::vector<unsigned long> npy_shape;
    std::vector<double>  npy_data;
    std::vector<std::vector<double>>  X_ref, H_ref;
  
    std::vector<double>  Bx_ref, Bh_ref;
    std::vector<double>  Bii_ref, Bif_ref, Big_ref, Bio_ref, Bhi_ref, Bhf_ref, Bhg_ref, Bho_ref;
    std::vector<double>  Xt_ref, Ht_ref;
    std::vector<std::vector<double>> Wx_ref, Wh_ref;
    std::vector<std::vector<double>> Wii_ref(256), Wif_ref(256), Wig_ref(256), Wio_ref(256), Whi_ref(256), Whf_ref(256), Whg_ref(256), Who_ref(256);

    spec::VectorType weight_vec; 
    spec::VectorType bias_vec; 
    spec::VectorType input_vec; 
    AdpfloatType<8, 3> adpfloat_tmp;

    wait();

    //Axi PEConfig 0x04-0x01
    rva_in_src.rw = 1;
    rva_in_src.data = set_bytes<16>("00_00_00_00_00_00_00_00_00_00_04_02_01_00_01_01"); //is_valid=1, is_zero_first=1, is_cluster=0, is_bias=1, num_manager=2, num_output=04 
    rva_in_src.addr = set_bytes<3>("40_00_10");  // last 4 bits never used 
    ofstream myfile;
    myfile.open ("axi_commands_for_slim_LSTM.csv");
    myfile << "0," << "W," << hex << rva_in_src.addr << "," << hex << rva_in_src.data << "\n";
    rva_in.Push(rva_in_src);
    wait();     

    //Axi pe manager 0 config 0x04-0x02
    rva_in_src.rw = 1;
    rva_in_src.data = set_bytes<16>("00_00_00_00_00_00_00_02_00_00_00_01_01_02_02_00"); //zero_active=0, adplfloat_bias_weight=2 , adplfloat_bias_bias=2 , adplfloat_bias_input=1 , num_input=01 , base_weight=0 , base_bias=02 (0x0002) , base_input=0 
    rva_in_src.addr = set_bytes<3>("40_00_20");  // last 4 bits never used 
    myfile << "2," << "W," << hex << rva_in_src.addr << "," << hex << rva_in_src.data << "\n";
    rva_in.Push(rva_in_src);
    wait();  

    //Axi pe manager 1 config 0x04-0x04
    rva_in_src.rw = 1;
    rva_in_src.data = set_bytes<16>("00_00_00_00_00_10_00_06_00_40_00_01_01_02_02_01"); //zero_active=1, adplfloat_bias_weight=2 , adplfloat_bias_bias=2 , adplfloat_bias_input=1 , num_input=01 , base_weight=64 , base_bias=06 (0x0006) , base_input=16 
    rva_in_src.addr = set_bytes<3>("40_00_40");  // last 4 bits never used 
    myfile << "2," << "W," << hex << rva_in_src.addr << "," << hex << rva_in_src.data << "\n";
    rva_in.Push(rva_in_src);
    wait(); 

    //Storing weight_ih_l0 in Weight SRAM
    npy_shape.clear(); npy_data.clear();
    npy::LoadArrayFromNumpy("/group/ttambe/sm6/hls_tapeout/cmod/testbench/dataset_enc256_dec256_unilstm/encoder.rnn_3.weight_ih_l0.npy", npy_shape, npy_data);
    cout << "Wx_ref size: " << "- shape0 is: " << npy_shape[0]  << " size1 is: " << npy_shape[1] << " size2 is: " << npy_shape[2] << endl;
    Wx_ref = to_2d(npy_shape[0], npy_shape[1], npy_data);

    //Creating Wii_ref weight
    for (unsigned int i=0; i < 16; i++) {
       for (unsigned int j=0; j < 16; j++) {
           Wii_ref[i].push_back(Wx_ref[i][j]); // = Wx_ref[i][j];
       }
    } 
    //cout << "printing first row of Wii" << endl;
    //PrintVector(Wii_ref[0]);
    for (unsigned int i=0; i < 1; i++) {  
      for (int j=0; j < 1; j++) {
        for (int m=0; m < spec::kNumVectorLanes; m++) {

               for (int k=0; k < spec::kNumVectorLanes; k++) {
                 adpfloat_tmp.Reset();
                 adpfloat_tmp.set_value(Wii_ref[16*i+m][16*j + k], adpbias_weight);
                 weight_vec[k] =adpfloat_tmp.to_rawbits();
                 adpfloat_tmp.Reset();
               } // for k
           rva_in_src.rw = 1;
           rva_in_src.data = weight_vec.to_rawbits();
           rva_in_src.addr = 0x500000 + (16*(64*i + j) + m)*16;
           //cout << "addresss_wii: " << 16*(64*i + m) + j << endl;
           myfile << "2," << "W," << hex << rva_in_src.addr << "," << hex << rva_in_src.data << "\n";
           rva_in.Push(rva_in_src);
           if (rva_in_src.addr ==  0x500000 + (16*(64*15 + 15) + 15)*16) {
              cout << "Pushed last Wii_ref weight vector: " << endl;
           }
           wait();
        } // for j
      } // for m
    } // for i 
    wait();


    //Creating Wif_ref weight
    for (unsigned int i=0; i < 16; i++) {
       for (unsigned int j=0; j < 16; j++) {
           Wif_ref[i].push_back(Wx_ref[i+256][j]); // = Wx_ref[i][j];
       }
    } 
    //cout << "printing first row of Wif" << endl;
    //PrintVector(Wif_ref[0]);

    for (unsigned int i=0; i < 1; i++) {  
      for (int j=0; j < 1; j++) {
        for (int m=0; m < spec::kNumVectorLanes; m++) {

               for (int k=0; k < spec::kNumVectorLanes; k++) {
                 adpfloat_tmp.Reset();
                 adpfloat_tmp.set_value(Wif_ref[16*i+m][16*j + k], adpbias_weight);
                 weight_vec[k] =adpfloat_tmp.to_rawbits();
                 adpfloat_tmp.Reset();
               } // for k
           rva_in_src.rw = 1;
           rva_in_src.data = weight_vec.to_rawbits();
           rva_in_src.addr = 0x500000 + (16*(64*i + j) + m + 32)*16;
           //cout << "addresss_wif: " << 16*(64*i + m) + j + 512 << endl;
           myfile << "2," << "W," << hex << rva_in_src.addr << "," << hex << rva_in_src.data << "\n";
           rva_in.Push(rva_in_src);
           if (rva_in_src.addr ==  0x500000 + (16*(64*15 + 15) + 15 + 512)*16) {
              cout << "Pushed last Wif_ref weight vector: " << endl;
           }
           wait();
        } // for j
      } // for m
    } // for i 
    wait();

    //Creating Wig_ref weight
    for (unsigned int i=0; i < 16; i++) {
       for (unsigned int j=0; j < 16; j++) {
           Wig_ref[i].push_back(Wx_ref[i+512][j]); // = Wx_ref[i][j];
       }
    } 
    for (unsigned int i=0; i < 1; i++) {  
      for (int j=0; j < 1; j++) {
        for (int m=0; m < spec::kNumVectorLanes; m++) {

               for (int k=0; k < spec::kNumVectorLanes; k++) {
                 adpfloat_tmp.Reset();
                 adpfloat_tmp.set_value(Wig_ref[16*i+m][16*j + k], adpbias_weight);
                 weight_vec[k] =adpfloat_tmp.to_rawbits();
                 adpfloat_tmp.Reset();
               } // for k
           rva_in_src.rw = 1;
           rva_in_src.data = weight_vec.to_rawbits();
           rva_in_src.addr = 0x500000 + (16*(64*i + j) + m + 16)*16;
           //cout << "addresss_wig: " << 16*(64*i + m) + j + 256 << endl;
           myfile << "2," << "W," << hex << rva_in_src.addr << "," << hex << rva_in_src.data << "\n";
           rva_in.Push(rva_in_src);
           if (rva_in_src.addr ==  0x500000 + (16*(64*15 + 15) + 15 + 256)*16) {
              cout << "Pushed last Wig_ref weight vector: " << endl;
           }
           wait();
        } // for j
      } // for m
    } // for i 
    wait();


    //Creating Wio_ref weight
    for (unsigned int i=0; i < 16; i++) {
       for (unsigned int j=0; j < 16; j++) {
           Wio_ref[i].push_back(Wx_ref[i+768][j]); // = Wx_ref[i][j];
       }
    } 
    for (unsigned int i=0; i < 1; i++) {  
      for (int j=0; j < 1; j++) {
        for (int m=0; m < spec::kNumVectorLanes; m++) {

               for (int k=0; k < spec::kNumVectorLanes; k++) {
                 adpfloat_tmp.Reset();
                 adpfloat_tmp.set_value(Wio_ref[16*i+m][16*j + k], adpbias_weight);
                 weight_vec[k] =adpfloat_tmp.to_rawbits();
                 adpfloat_tmp.Reset();
               } // for k
           rva_in_src.rw = 1;
           rva_in_src.data = weight_vec.to_rawbits();
           rva_in_src.addr = 0x500000 + (16*(64*i + j) + m + 48)*16;
           //cout << "addresss_wio: " << 16*(64*i + m) + j + 768 << endl;
           myfile << "2," << "W," << hex << rva_in_src.addr << "," << hex << rva_in_src.data << "\n";
           rva_in.Push(rva_in_src);
           if (rva_in_src.addr ==  0x500000 + (16*(64*15 + 15) + 15 + 768)*16) {
              cout << "Pushed last Wio_ref weight vector: " << endl;
           }
           wait();
        } // for j
      } // for m
    } // for i 
    wait();

    //Storing weight_hh_l0 in Weight SRAM
    npy_shape.clear(); npy_data.clear();
    npy::LoadArrayFromNumpy("/group/ttambe/sm6/hls_tapeout/cmod/testbench/dataset_enc256_dec256_unilstm/encoder.rnn_3.weight_hh_l0.npy", npy_shape, npy_data);
    cout << "Wh_ref size: " << "- shape0 is: " << npy_shape[0]  << " size1 is: " << npy_shape[1] << " size2 is: " << npy_shape[2] << endl;
    Wh_ref = to_2d(npy_shape[0], npy_shape[1], npy_data);

    //Creating Whi_ref weight
    for (unsigned int i=0; i < 16; i++) {
       for (unsigned int j=0; j < 16; j++) {
           Whi_ref[i].push_back(Wh_ref[i][j]); 
       }
    } 
    for (unsigned int i=0; i < 1; i++) {  
      for (int j=0; j < 1; j++) {
        for (int m=0; m < spec::kNumVectorLanes; m++) {

               for (int k=0; k < spec::kNumVectorLanes; k++) {
                 adpfloat_tmp.Reset();
                 adpfloat_tmp.set_value(Whi_ref[16*i+m][16*j + k], adpbias_weight);
                 weight_vec[k] =adpfloat_tmp.to_rawbits();
                 adpfloat_tmp.Reset();
               } // for k
           rva_in_src.rw = 1;
           rva_in_src.data = weight_vec.to_rawbits();
           rva_in_src.addr = 0x500000 + (64 + 16*(64*i + j) + m)*16;
           //cout << "addresss_whi: " << 16384 + 16*(64*i + m) + j << endl;
           myfile << "2," << "W," << hex << rva_in_src.addr << "," << hex << rva_in_src.data << "\n";
           rva_in.Push(rva_in_src);
           if (rva_in_src.addr ==  0x500000 + (16384 + 16*(64*15 + 15) + 15)*16) {
              cout << "Pushed last Whi_ref weight vector: " << endl;
           }
           wait();
        } // for j
      } // for m
    } // for i 
    wait();

    //Creating Whf_ref weight
    for (unsigned int i=0; i < 16; i++) {
       for (unsigned int j=0; j < 16; j++) {
           Whf_ref[i].push_back(Wh_ref[i+256][j]); 
       }
    } 
    for (unsigned int i=0; i < 1; i++) {  
      for (int j=0; j < 1; j++) {
        for (int m=0; m < spec::kNumVectorLanes; m++) {

               for (int k=0; k < spec::kNumVectorLanes; k++) {
                 adpfloat_tmp.Reset();
                 adpfloat_tmp.set_value(Whf_ref[16*i+m][16*j + k], adpbias_weight);
                 weight_vec[k] =adpfloat_tmp.to_rawbits();
                 adpfloat_tmp.Reset();
               } // for k
           rva_in_src.rw = 1;
           rva_in_src.data = weight_vec.to_rawbits();
           rva_in_src.addr = 0x500000 + (64 + 16*(64*i + j) + m + 32)*16;
           //cout << "addresss_whf: " << 16384 + 16*(64*i + m) + j + 512 << endl;
           myfile << "2," << "W," << hex << rva_in_src.addr << "," << hex << rva_in_src.data << "\n";
           rva_in.Push(rva_in_src);
           if (rva_in_src.addr ==  0x500000 + (16384 + 16*(64*15 + 15) + 15 + 512)*16) {
              cout << "Pushed last Whf_ref weight vector: " << endl;
           }
           wait();
        } // for j
      } // for m
    } // for i 
    wait();


    //Creating Whg_ref weight
    for (unsigned int i=0; i < 16; i++) {
       for (unsigned int j=0; j < 16; j++) {
           Whg_ref[i].push_back(Wh_ref[i+512][j]); 
       }
    } 
    for (unsigned int i=0; i < 1; i++) {  
      for (int j=0; j < 1; j++) {
        for (int m=0; m < spec::kNumVectorLanes; m++) {

               for (int k=0; k < spec::kNumVectorLanes; k++) {
                 adpfloat_tmp.Reset();
                 adpfloat_tmp.set_value(Whg_ref[16*i+m][16*j + k], adpbias_weight);
                 weight_vec[k] =adpfloat_tmp.to_rawbits();
                 adpfloat_tmp.Reset();
               } // for k
           rva_in_src.rw = 1;
           rva_in_src.data = weight_vec.to_rawbits();
           rva_in_src.addr = 0x500000 + (64 + 16*(64*i + j) + m + 16)*16;
           //cout << "addresss_whg: " << 16384 + 16*(64*i + m) + j + 256 << endl;
           myfile << "2," << "W," << hex << rva_in_src.addr << "," << hex << rva_in_src.data << "\n";
           rva_in.Push(rva_in_src);
           if (rva_in_src.addr ==  0x500000 + (16384 + 16*(64*15 + 15) + 15 + 256)*16) {
              cout << "Pushed last Whg_ref weight vector: " << endl;
           }
           wait();
        } // for j
      } // for m
    } // for i 
    wait();


    //Creating Who_ref weight
    for (unsigned int i=0; i < 16; i++) {
       for (unsigned int j=0; j < 16; j++) {
           Who_ref[i].push_back(Wh_ref[i+768][j]); 
       }
    } 
    for (unsigned int i=0; i < 1; i++) {  
      for (int j=0; j < 1; j++) {
        for (int m=0; m < spec::kNumVectorLanes; m++) {

               for (int k=0; k < spec::kNumVectorLanes; k++) {
                 adpfloat_tmp.Reset();
                 adpfloat_tmp.set_value(Who_ref[16*i+m][16*j + k], adpbias_weight);
                 weight_vec[k] =adpfloat_tmp.to_rawbits();
                 adpfloat_tmp.Reset();
               } // for k
           rva_in_src.rw = 1;
           rva_in_src.data = weight_vec.to_rawbits();
           rva_in_src.addr = 0x500000 + (64 + 16*(64*i + j) + m + 48)*16;
           //cout << "addresss_who: " << 16384 + 16*(64*i + m) + j + 768 << endl;
           myfile << "2," << "W," << hex << rva_in_src.addr << "," << hex << rva_in_src.data << "\n";
           rva_in.Push(rva_in_src);
           if (rva_in_src.addr ==  0x500000 + (16384 + 16*(64*15 + 15) + 15 + 768)*16) {
              cout << "Pushed last Who_ref weight vector: " << endl;
           }
           wait();
        } // for j
      } // for m
    } // for i 

    //bias_ih
    npy_shape.clear(); npy_data.clear();
    npy::LoadArrayFromNumpy("/group/ttambe/sm6/hls_tapeout/cmod/testbench/dataset_enc256_dec256_unilstm/encoder.rnn_3.bias_ih_l0.npy", npy_shape, npy_data);
    cout << "Bx_ref size: " << "- shape0 is: " << npy_shape[0] << endl;
    Bx_ref = npy_data; 
    //cout << "printing Bx_ref" << endl;
    //PrintVector(Bx_ref);

    //Creating Bii_ref
    for (unsigned int i=0; i < 16; i++) {
        Bii_ref.push_back(Bx_ref[i]);
    }
    //cout << "printing bii" << endl;
    //PrintVector(Bii_ref);
    for (unsigned int i=0; i < 1; i++) { 
      //for (unsigned int m=0; m <  
        for (int j=0; j < spec::kNumVectorLanes; j++) {
           adpfloat_tmp.Reset();
           adpfloat_tmp.set_value(Bii_ref[16*i + j], adpbias_bias);
           bias_vec[j] =adpfloat_tmp.to_rawbits();
           adpfloat_tmp.Reset();
         } // for j
      rva_in_src.rw = 1;
      rva_in_src.data = bias_vec.to_rawbits();
      rva_in_src.addr = 0x600000 + (2 + 4*i)*16;
      //cout << "addresss_bii: " << 32 + 4*i << endl;
      myfile << "2," << "W," << hex << rva_in_src.addr << "," << hex << rva_in_src.data << "\n";
      rva_in.Push(rva_in_src);
      if (rva_in_src.addr ==  0x600000 + (32 + 4*15)*16) {
         cout << "Pushed last Bii_ref bias vector: " << endl;
      }
      wait();
    }  // for i
    wait();


    //Creating Bif_ref
    for (unsigned int i=0; i < 16; i++) {
        Bif_ref.push_back(Bx_ref[i+256]);
    }
    for (unsigned int i=0; i < 1; i++) { 
      //for (unsigned int m=0; m <  
        for (int j=0; j < spec::kNumVectorLanes; j++) {
           adpfloat_tmp.Reset();
           adpfloat_tmp.set_value(Bif_ref[16*i + j], adpbias_bias);
           bias_vec[j] =adpfloat_tmp.to_rawbits();
           adpfloat_tmp.Reset();
         } // for j
      rva_in_src.rw = 1;
      rva_in_src.data = bias_vec.to_rawbits();
      rva_in_src.addr = 0x600000 + (2 + 4*i + 2)*16;
      //cout << "addresss_bif: " << 32 + 4*i + 2 << endl;
      myfile << "2," << "W," << hex << rva_in_src.addr << "," << hex << rva_in_src.data << "\n";
      rva_in.Push(rva_in_src);
      if (rva_in_src.addr ==  0x600000 + (32 + 4*15 + 2)*16) {
         cout << "Pushed last Bif_ref bias vector: " << endl;
      }
      wait();
    }  // for i
    wait();

    //Creating Big_ref
    for (unsigned int i=0; i < 16; i++) {
        Big_ref.push_back(Bx_ref[i+512]);
    }
    for (unsigned int i=0; i < 1; i++) { 
      //for (unsigned int m=0; m <  
        for (int j=0; j < spec::kNumVectorLanes; j++) {
           adpfloat_tmp.Reset();
           adpfloat_tmp.set_value(Big_ref[16*i + j], adpbias_bias);
           bias_vec[j] =adpfloat_tmp.to_rawbits();
           adpfloat_tmp.Reset();
         } // for j
      rva_in_src.rw = 1;
      rva_in_src.data = bias_vec.to_rawbits();
      rva_in_src.addr = 0x600000 + (2 + 4*i + 1)*16;
      //cout << "addresss_big: " << 32 + 4*i + 1 << endl;
      myfile << "2," << "W," << hex << rva_in_src.addr << "," << hex << rva_in_src.data << "\n";
      rva_in.Push(rva_in_src);
      if (rva_in_src.addr ==  0x600000 + (32 + 4*15 + 1)*16) {
         cout << "Pushed last Big_ref bias vector: " << endl;
      }
      wait();
    }  // for i
    wait();

    //Creating Bio_ref
    for (unsigned int i=0; i < 16; i++) {
        Bio_ref.push_back(Bx_ref[i+768]);
    }
    for (unsigned int i=0; i < 1; i++) { 
      //for (unsigned int m=0; m <  
        for (int j=0; j < spec::kNumVectorLanes; j++) {
           adpfloat_tmp.Reset();
           adpfloat_tmp.set_value(Bio_ref[16*i + j], adpbias_bias);
           bias_vec[j] =adpfloat_tmp.to_rawbits();
           adpfloat_tmp.Reset();
         } // for j
      rva_in_src.rw = 1;
      rva_in_src.data = bias_vec.to_rawbits();
      rva_in_src.addr = 0x600000 + (2 + 4*i + 3)*16;
      myfile << "2," << "W," << hex << rva_in_src.addr << "," << hex << rva_in_src.data << "\n";
      rva_in.Push(rva_in_src);
      if (rva_in_src.addr ==  0x600000 + (32 + 4*15 + 3)*16) {
         cout << "Pushed last Bio_ref bias vector: " << endl;
      }
      wait();
    }  // for i
    wait();


    //bias_hh
    npy_shape.clear(); npy_data.clear();
    npy::LoadArrayFromNumpy("/group/ttambe/sm6/hls_tapeout/cmod/testbench/dataset_enc256_dec256_unilstm/encoder.rnn_3.bias_hh_l0.npy", npy_shape, npy_data);
    cout << "Bh_ref size: " << "- shape0 is: " << npy_shape[0] << endl;
    Bh_ref = npy_data; 

    //Creating Bhi_ref
    for (unsigned int i=0; i < 16; i++) {
        Bhi_ref.push_back(Bh_ref[i]);
    }
    for (unsigned int i=0; i < 1; i++) { 
      //for (unsigned int m=0; m <  
        for (int j=0; j < spec::kNumVectorLanes; j++) {
           adpfloat_tmp.Reset();
           adpfloat_tmp.set_value(Bhi_ref[16*i + j], adpbias_bias);
           bias_vec[j] =adpfloat_tmp.to_rawbits();
           adpfloat_tmp.Reset();
         } // for j
      rva_in_src.rw = 1;
      rva_in_src.data = bias_vec.to_rawbits();
      rva_in_src.addr = 0x600000 + (6 + 4*i)*16;
      myfile << "2," << "W," << hex << rva_in_src.addr << "," << hex << rva_in_src.data << "\n";
      rva_in.Push(rva_in_src);
      if (rva_in_src.addr ==  0x600000 + (96 + 4*15)*16) {
         cout << "Pushed last Bhi_ref bias vector: " << endl;
      }
      wait();
    }  // for i
    wait();


    //Creating Bhf_ref
    for (unsigned int i=0; i < 16; i++) {
        Bhf_ref.push_back(Bh_ref[i+256]);
    }
    for (unsigned int i=0; i < 1; i++) { 
      //for (unsigned int m=0; m <  
        for (int j=0; j < spec::kNumVectorLanes; j++) {
           adpfloat_tmp.Reset();
           adpfloat_tmp.set_value(Bhf_ref[16*i + j], adpbias_bias);
           bias_vec[j] =adpfloat_tmp.to_rawbits();
           adpfloat_tmp.Reset();
         } // for j
      rva_in_src.rw = 1;
      rva_in_src.data = bias_vec.to_rawbits();
      rva_in_src.addr = 0x600000 + (6 + 4*i + 2)*16;
      //cout << "addresss_bhf: " << 96 + 4*i + 2 << endl;
      myfile << "2," << "W," << hex << rva_in_src.addr << "," << hex << rva_in_src.data << "\n";
      rva_in.Push(rva_in_src);
      if (rva_in_src.addr ==  0x600000 + (96 + 4*15 + 2)*16) {
         cout << "Pushed last Bhf_ref bias vector: " << endl;
      }
      wait();
    }  // for i
    wait();

    //Creating Bhg_ref
    for (unsigned int i=0; i < 16; i++) {
        Bhg_ref.push_back(Bh_ref[i+512]);
    }
    for (unsigned int i=0; i < 1; i++) { 
      //for (unsigned int m=0; m <  
        for (int j=0; j < spec::kNumVectorLanes; j++) {
           adpfloat_tmp.Reset();
           adpfloat_tmp.set_value(Bhg_ref[16*i + j], adpbias_bias);
           bias_vec[j] =adpfloat_tmp.to_rawbits();
           adpfloat_tmp.Reset();
         } // for j
      rva_in_src.rw = 1;
      rva_in_src.data = bias_vec.to_rawbits();
      rva_in_src.addr = 0x600000 + (6 + 4*i + 1)*16;
      //cout << "addresss_bhg: " << 96 + 4*i + 1 << endl;
      myfile << "2," << "W," << hex << rva_in_src.addr << "," << hex << rva_in_src.data << "\n";
      rva_in.Push(rva_in_src);
      if (rva_in_src.addr ==  0x600000 + (96 + 4*15 + 1)*16) {
         cout << "Pushed last Bhg_ref bias vector: " << endl;
      }
      wait();
    }  // for i
    wait();

    //Creating Bho_ref
    for (unsigned int i=0; i < 16; i++) {
        Bho_ref.push_back(Bh_ref[i+768]);
    }
    for (unsigned int i=0; i < 1; i++) { 
      //for (unsigned int m=0; m <  
        for (int j=0; j < spec::kNumVectorLanes; j++) {
           adpfloat_tmp.Reset();
           adpfloat_tmp.set_value(Bho_ref[16*i + j], adpbias_bias);
           bias_vec[j] =adpfloat_tmp.to_rawbits();
           adpfloat_tmp.Reset();
         } // for j
      rva_in_src.rw = 1;
      rva_in_src.data = bias_vec.to_rawbits();
      rva_in_src.addr = 0x600000 + (6 + 4*i + 3)*16;
      //cout << "addresss_bho: " << 96 + 4*i + 3 << endl;
      myfile << "2," << "W," << hex << rva_in_src.addr << "," << hex << rva_in_src.data << "\n";
      rva_in.Push(rva_in_src);
      if (rva_in_src.addr ==  0x600000 + (96 + 4*15 + 3)*16) {
         cout << "Pushed last Bho_ref bias vector: " << endl;
      }
      wait();
    }  // for i
    wait();

    //Storing Xt_ref in Input SRAM
    npy_shape.clear(); npy_data.clear();
    npy::LoadArrayFromNumpy("/group/ttambe/sm6/hls_tapeout/cmod/testbench/dataset_enc256_dec256_unilstm/x_l3.npy", npy_shape, npy_data);
    X_ref = to_2d(npy_shape[0], npy_shape[2], npy_data);
    cout << "X_ref size: " << "- shape0 is: " << npy_shape[0] << "- shape1 is: " << npy_shape[2] << endl;
    Xt_ref = X_ref[0]; //Xt_ref is first timestep 
    //cout << "print xt_ref" << endl;
    //PrintVector(Xt_ref);
    for (unsigned int i=0; i < (npy_shape[2] / 256); i++) {  
        for (int j=0; j < spec::kNumVectorLanes; j++) {
           adpfloat_tmp.Reset();
           adpfloat_tmp.set_value(Xt_ref[16*i + j], adpbias_input);
           input_vec[j] =adpfloat_tmp.to_rawbits();
           adpfloat_tmp.Reset();
         } // for j
      rva_in_src.rw = 1;
      rva_in_src.data = input_vec.to_rawbits();
      rva_in_src.addr = 0x600000 + i*16;
      myfile << "input activations" << "\n";
      myfile << "2," << "W," << hex << rva_in_src.addr << "," << hex << rva_in_src.data << "\n";
      myfile << "end of input activations" << "\n";
      rva_in.Push(rva_in_src);
      if (rva_in_src.addr ==  0x600000 + ((npy_shape[2]/256) - 1)*16) {
         cout << "Pushed last Xt_ref input vector: " << endl;
      }
      wait();
    }  // for i
    wait();

    //ActUnit Axi Config
    rva_in_src.rw = 1;
    rva_in_src.data = set_bytes<16>("00_00_00_00_00_00_00_00_00_00_00_01_18_02_01_01"); //is_valid=1, is_zero_first=1, adpfloat_bias=2, num_inst=24, num_output=1, buffer/output addr_base=0
    rva_in_src.addr = set_bytes<3>("80_00_10"); 
    myfile << "2," << "W," << hex << rva_in_src.addr << "," << hex << rva_in_src.data << "\n";
    rva_in.Push(rva_in_src);
    wait();

    //First 16 ActUnit instructions for LSTM computation
    rva_in_src.rw = 1;
    rva_in_src.data = set_bytes<16>("81_96_18_A4_86_38_34_91_B4_86_38_34_A0_81_34_30");
    rva_in_src.addr = set_bytes<3>("80_00_20"); 
    myfile << "2," << "W," << hex << rva_in_src.addr << "," << hex << rva_in_src.data << "\n";
    rva_in.Push(rva_in_src);
    wait();

    //Next 8 ActUnit instructions to complete LSTM computation
    rva_in_src.rw = 1;
    rva_in_src.data = set_bytes<16>("00_00_00_00_00_00_00_00_40_91_A4_86_38_34_B0_20");
    rva_in_src.addr = set_bytes<3>("80_00_30"); 
    myfile << "2," << "W," << hex << rva_in_src.addr << "," << hex << rva_in_src.data << "\n";
    rva_in.Push(rva_in_src);
    wait();

    //send pe_start and act_start signal
    rva_in_src.rw = 1;
    rva_in_src.data = set_bytes<16>("00_00_00_00_00_00_00_00_00_00_00_00_00_00_00_00"); 
    rva_in_src.addr = set_bytes<3>("00_00_00");  
    myfile << "2," << "W," << hex << rva_in_src.addr << "," << hex << rva_in_src.data << "\n";
    rva_in.Push(rva_in_src);
    wait(); 
    myfile.close();    

  } // run()
}; //SC MODULE Source

SC_MODULE(Dest) {
  sc_in<bool> clk;
  sc_in<bool> rst;
  Connections::In<spec::Axi::SlaveToRVA::Read> rva_out;
  Connections::In<spec::StreamType> output_port; 
  Connections::In<bool> done;  

  spec::StreamType output_port_dest;
  bool done_dest;

  SC_CTOR(Dest) {
    SC_THREAD(PopOutport);
    sensitive << clk.pos();
    async_reset_signal_is(rst, false);
    SC_THREAD(PopDone);
    sensitive << clk.pos();
    async_reset_signal_is(rst, false);
  }    

  void PopOutport() {
   wait();
 
   while (1) {
     if (output_port.PopNB(output_port_dest)) {
        //cout << hex << sc_time_stamp() << " output_port data = " << output_port_dest.data << endl;
        cout << sc_time_stamp() << "Design output_port result" << " \t " << endl;
        for (int i = 0; i < spec::kNumVectorLanes; i++) {
          AdpfloatType<8,3> tmp(output_port_dest.data[i]);
          cout << tmp.to_float(2) << endl; 
        }
     }
     wait(); 
   } // while
   
  } //PopOutputport

  void PopDone() {
   wait();
 
   while (1) {
     if (done.PopNB(done_dest)) {
        //cout << hex << sc_time_stamp() << " output_port data = " << done_dest.data << endl;
        cout << sc_time_stamp() << "Design done result" << " \t " << done_dest << endl;
     }
     wait(); 
   } // while
   
  } //PopDone

}; //SC MODULE Dest


SC_MODULE(testbench) {
  SC_HAS_PROCESS(testbench);
  sc_clock clk;
  sc_signal<bool> rst;

  Connections::Combinational<spec::StreamType> input_port;     
  Connections::Combinational<bool> start;  
  Connections::Combinational<spec::Axi::SlaveToRVA::Write> rva_in;
  Connections::Combinational<spec::Axi::SlaveToRVA::Read> rva_out;
  Connections::Combinational<spec::StreamType> output_port; 
  Connections::Combinational<bool> done;  

  NVHLS_DESIGN(PEModule) dut;
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
     dut.input_port(input_port);
     dut.start(start);
     dut.rva_in(rva_in);
     dut.rva_out(rva_out);
     dut.output_port(output_port);
     dut.done(done);

     source.clk(clk);
     source.rst(rst);
     source.input_port(input_port);
     source.start(start);
     source.rva_in(rva_in);

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
    wait(300, SC_NS );
    std::cout << "@" << sc_time_stamp() <<" sc_stop" << std::endl;
    sc_stop();
  }  

}; //SC MODULE testbench


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
