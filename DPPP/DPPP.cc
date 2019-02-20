//# DPPP.cc: Program to execute steps like averaging and flagging on an MS
//# Copyright (C) 2010
//# ASTRON (Netherlands Institute for Radio Astronomy)
//# P.O.Box 2, 7990 AA Dwingeloo, The Netherlands
//#
//# This file is part of the LOFAR software suite.
//# The LOFAR software suite is free software: you can redistribute it and/or
//# modify it under the terms of the GNU General Public License as published
//# by the Free Software Foundation, either version 3 of the License, or
//# (at your option) any later version.
//#
//# The LOFAR software suite is distributed in the hope that it will be useful,
//# but WITHOUT ANY WARRANTY; without even the implied warranty of
//# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//# GNU General Public License for more details.
//#
//# You should have received a copy of the GNU General Public License along
//# with the LOFAR software suite. If not, see <http://www.gnu.org/licenses/>.
//#
//# $Id$
//#
//# @author Ger van Diepen

#include "DPRun.h"
#include "Version.h"

#include <boost/filesystem/operations.hpp>

#include <iostream>
#include <stdexcept>
#include <papi.h>

#include <time.h>
#include <stdint.h>

#include <unistd.h>

//#ifndef PAPI_TEST_H
//#define PAPI_TEST_H
//#include <papi_test.h>
//#endif


// Define handler that tries to print a backtrace.
//Exception::TerminateHandler t(Exception::terminate);
//#define NUM_EVENTS 1

#define NUM_EVENTS 40
#define ALL_EVENTS 102


void print_PAPI_events(int *Events);


void print_time(){
   struct timespec tms;
   if (clock_gettime(CLOCK_REALTIME,&tms)) {
              return;}
   int64_t micros = tms.tv_sec * 1000000;
   micros += tms.tv_nsec/1000;
   if (tms.tv_nsec % 1000 >= 500) {
           ++micros;}

   std::cout.precision(4);
   std::cout<< micros <<'\n';
   return;
}

void            
test_fail( const char *file, int line, const char *call, int retval )
{               
//function copied from papi_test.cc since linking is hard and CMake CHates me
        #define RED    "\033[1;31m"
        #define NORMAL "\033[0m"
        static int TESTS_COLOR = 1;
        char buf[128];

        (void)file;

        memset( buf, '\0', sizeof ( buf ) );

        if (TESTS_COLOR) fprintf(stdout,"%s",RED);
        fprintf( stdout, "FAILED!!!");
        if (TESTS_COLOR) fprintf(stdout,"%s",NORMAL);
        fprintf( stdout, "\nLine # %d ", line );

        if ( retval == PAPI_ESYS ) {
                sprintf( buf, "System error in %s", call );
                perror( buf );
        } else if ( retval > 0 ) {
                fprintf( stdout, "Error: %s\n", call );
        } else if ( retval == 0 ) {
#if defined(sgi) 
                fprintf( stdout, "SGI requires root permissions for this test\n" );
#else
                fprintf( stdout, "Error: %s\n", call );
#endif
        } else {
                fprintf( stdout, "Error in %s: %s\n", call, PAPI_strerror( retval ) );
        } 

   fprintf(stdout, "Some tests require special hardware, permissions, OS, compilers\n"
                   "or library versions. PAPI may still function perfectly on your \n"
                   "system without the particular feature being tested here.       \n");

        /* NOTE: Because test_fail is called from thread functions,
           calling PAPI_shutdown here could prevent some threads
           from being able to free memory they have allocated.
         */
        if ( PAPI_is_initialized(  ) ) {
                PAPI_shutdown(  );
        }          

        /* This is stupid.  Threads are the rare case */
        /* and in any case an exit() should clear everything out */                                                
        /* adding back the exit() call */

        exit(1);
}               



void showUsage( ) {
   int retval;
   char code_name[PAPI_MAX_STR_LEN];

   int all_events[ALL_EVENTS] = { PAPI_L1_DCM, PAPI_L1_ICM, PAPI_L2_DCM, PAPI_L2_ICM, PAPI_L3_DCM, PAPI_L3_ICM, PAPI_L1_TCM, PAPI_L2_TCM, PAPI_L3_TCM, PAPI_CA_SNP, PAPI_CA_SHR, PAPI_CA_CLN, PAPI_CA_INV, PAPI_CA_ITV, PAPI_L3_LDM, PAPI_L3_STM, PAPI_BRU_IDL, PAPI_FXU_IDL, PAPI_FPU_IDL, PAPI_LSU_IDL, PAPI_TLB_DM, PAPI_TLB_IM, PAPI_TLB_TL, PAPI_L1_LDM, PAPI_L1_STM, PAPI_L2_LDM, PAPI_L2_STM, PAPI_BTAC_M, PAPI_PRF_DM, PAPI_L3_DCH, PAPI_TLB_SD, PAPI_CSR_FAL, PAPI_CSR_SUC, PAPI_CSR_TOT, PAPI_MEM_SCY, PAPI_MEM_RCY, PAPI_MEM_WCY, PAPI_STL_ICY, PAPI_FUL_ICY, PAPI_STL_CCY, PAPI_FUL_CCY, PAPI_HW_INT, PAPI_BR_UCN, PAPI_BR_CN, PAPI_BR_TKN, PAPI_BR_NTK, PAPI_BR_MSP, PAPI_BR_PRC, PAPI_FMA_INS, PAPI_TOT_IIS, PAPI_TOT_INS, PAPI_INT_INS, PAPI_FP_INS, PAPI_LD_INS, PAPI_SR_INS, PAPI_BR_INS, PAPI_VEC_INS, PAPI_RES_STL, PAPI_FP_STAL, PAPI_TOT_CYC, PAPI_LST_INS, PAPI_SYC_INS, PAPI_L1_DCH, PAPI_L2_DCH, PAPI_L1_DCA, PAPI_L2_DCA, PAPI_L3_DCA, PAPI_L1_DCR, PAPI_L2_DCR, PAPI_L3_DCR, PAPI_L1_DCW, PAPI_L2_DCW, PAPI_L3_DCW, PAPI_L1_ICH, PAPI_L2_ICH, PAPI_L3_ICH, PAPI_L1_ICA, PAPI_L2_ICA, PAPI_L3_ICA, PAPI_L1_ICR, PAPI_L2_ICR, PAPI_L3_ICR, PAPI_L1_ICW, PAPI_L2_ICW, PAPI_L3_ICW, PAPI_L1_TCH, PAPI_L2_TCH, PAPI_L3_TCH, PAPI_L1_TCA, PAPI_L2_TCA, PAPI_L3_TCA, PAPI_L1_TCR, PAPI_L2_TCR, PAPI_L3_TCR, PAPI_L1_TCW, PAPI_L2_TCW, PAPI_L3_TCW, PAPI_FML_INS, PAPI_FAD_INS, PAPI_FDV_INS, PAPI_FSQ_INS, PAPI_FNV_INS };
   
   retval = PAPI_library_init(PAPI_VER_CURRENT);
   vector<int> events_vec;

   for (int i=0;i<ALL_EVENTS;i++){
       if (PAPI_query_event(all_events[i]) != PAPI_OK ){
       PAPI_event_code_to_name(all_events[i], code_name);
       std::cout<<"PAPI Event "<< code_name <<" cannot be accesssed and will not be used\n";}
       else {events_vec.push_back(all_events[i]);
       }
   }
   

   int* Events = &events_vec[0];
   int num_hwcntrs = 0;
 
   num_hwcntrs = PAPI_num_counters();
   if (num_hwcntrs>NUM_EVENTS) 
       num_hwcntrs=NUM_EVENTS;

   
   retval = PAPI_start_counters(Events, num_hwcntrs);
   print_time();

   std::cout<<"PAPI_start_counters_retval"<<retval<<"\n";
   if ( retval != PAPI_OK  )
        test_fail( __FILE__, __LINE__, "PAPI_start_counters", retval  );

   print_PAPI_events(Events);

   std::cout<< "Instrumented with PAPI version: "<< PAPI_VER_CURRENT <<" \n";
   std::cout <<
    "Usage: DPPP [-v] [parsetfile] [parsetkeys...]\n"
    "  parsetfile: a file containing one parset key=value pair per line\n"
    "  parsetkeys: any number of parset key=value pairs, e.g. msin=my.MS\n\n"
    "If both a file and command-line keys are specified, the keys on the command\n"
    "line override those in the file.\n"
    "If no arguments are specified, the program tries to read \"NDPPP.parset\"\n"
    "or \"DPPP.parset\" as a default.\n"
    "-v will show version info and exit.\n"
    "Documentation is at:\n"
    "http://www.lofar.org/wiki/doku.php?id=public:user_software:documentation:ndppp\n";

   print_PAPI_events(Events);
   std::cout<<'\n';
//   print_PAPI_events(Events);

}

void print_PAPI_events(int *Events){
/*Prints out all the PAPI events in the events_set and their names*/

   long_long values[NUM_EVENTS];
   char code_name[PAPI_MAX_STR_LEN];

   struct timespec tms1;
   struct timespec tms;

   if (clock_gettime(CLOCK_REALTIME,&tms1)) {
           return;
                          }
   int retval = PAPI_read_counters( values, NUM_EVENTS  );

   if (clock_gettime(CLOCK_REALTIME,&tms)) {
           return;
               }

   int64_t micros = tms.tv_sec * 1000000;
   micros += tms.tv_nsec/1000;
   int64_t micros1 = tms1.tv_sec * 1000000;
   micros1 += tms1.tv_nsec/1000;
   if (tms.tv_nsec % 1000 >= 500) {
           ++micros;
               }

   if (tms1.tv_nsec % 1000 >= 500) {
              ++micros1;
                             }

   std::cout.precision(4);
   std::cout<< int64_t (micros1 + (micros-micros1)/2) <<'\n';

   if ( retval != PAPI_OK  )
     test_fail( __FILE__, __LINE__, "PAPI_read_counters", retval  );
   for (int i=0; i<NUM_EVENTS; i++){
      PAPI_event_code_to_name(Events[i], code_name);
      std::cout<< code_name <<": " <<values[i]<<"\n";
   }
   std::cout<<'\n';
 return;
}

   
int main(int argc, char *argv[])
{
  try
  {
    // Get the name of the parset file.
    if (argc>1) {
      string param = argv[1];
      if(param=="--help" ||
        param=="-help" || param=="-h" || 
        param=="--usage" || param=="-usage") {
        showUsage();
        return 0;
      }
      else if(param == "-v" || param == "--version")
      {
        std::cout << DPPPVersion::AsString() << '\n';
        return 0;
      }
    }

    string parsetName;
    if (argc > 1  &&  string(argv[1]).find('=') == string::npos) {
      // First argument is parset name (except if it's a key-value pair)
      parsetName = argv[1];
    } else if (argc==1) {
      // No arguments given: try to load [N]DPPP.parset
      if (boost::filesystem::exists("DPPP.parset")) {
        parsetName="DPPP.parset";
      } else if (boost::filesystem::exists("NDPPP.parset")) {
        parsetName="NDPPP.parset";
      } else { // No default file, show usage and exit
        showUsage();
        return 0;
      }
    }

    // Execute the parset file.
    DP3::DPPP::DPRun::execute (parsetName, argc, argv);
  } catch (std::exception& err) {
    std::cerr << "\nstd exception detected: " << err.what() << std::endl;
    return 1;
  }
  return 0;
}
