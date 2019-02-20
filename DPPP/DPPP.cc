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

//#ifndef PAPI_TEST_H
//#define PAPI_TEST_H
//#include <papi_test.h>
//#endif


// Define handler that tries to print a backtrace.
//Exception::TerminateHandler t(Exception::terminate);
//#define NUM_EVENTS 1

#define NUM_EVENTS 5
#define ALL_EVENTS 6


void print_PAPI_events(int *Events);

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



void showUsage() {
   int retval;
//   #define NUM_EVENTS 5
//   #define ALL_EVENTS 6
   char code_name[PAPI_MAX_STR_LEN];

   int all_events[ALL_EVENTS] = { PAPI_L1_DCM, PAPI_L2_TCM, PAPI_L2_ICM, PAPI_FP_INS, PAPI_TOT_INS, PAPI_TOT_CYC};
   
   retval = PAPI_library_init(PAPI_VER_CURRENT);

   for (int i=0;i<ALL_EVENTS;i++){
       if (PAPI_query_event(all_events[i]) != PAPI_OK ){
       PAPI_event_code_to_name(all_events[i], code_name);
       std::cout<<"PAPI Event "<< code_name <<" cannot be accesssed and will not be used\n";}
   }

   int Events[NUM_EVENTS] = { PAPI_L1_DCM, PAPI_L2_TCM, PAPI_L2_ICM, PAPI_TOT_INS, PAPI_TOT_CYC };
   int num_hwcntrs = 0;
 
   num_hwcntrs = PAPI_num_counters();
//   if ((num_hwcntrs = PAPI_num_counters()) != PAPI_OK){
//          std::cout<<PAPI_num_counters();
//           test_fail( __FILE__, __LINE__, "PAPI_num_counters", retval   );}
  


   if (num_hwcntrs>NUM_EVENTS) 
       num_hwcntrs=NUM_EVENTS;

   
   retval = PAPI_start_counters(Events, num_hwcntrs);
   std::cout<<"PAPI_start_counters_retval"<<retval<<"/n";
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
}

void print_PAPI_events(int *Events){
/*Prints out all the PAPI events in the events_set and their names*/
   std::cout<<'\n';

   long_long values[NUM_EVENTS];
   char code_name[PAPI_MAX_STR_LEN];

   int  retval = PAPI_read_counters( values, NUM_EVENTS  );
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
