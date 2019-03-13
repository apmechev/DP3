//# DPRun.h: Class to run steps like averaging and flagging on an MS
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

#ifndef DPPP_DPRUN_H
#define DPPP_DPRUN_H

// @file
// @brief Class to run steps like averaging and flagging on an MS

#include "DPStep.h"
#include "MSReader.h"

#include <map>


//PAPI stuff
#include <papi.h>

void print_PAPI_events(int *Events);
int* init_PAPI_events();

#define NUM_EVENTS 40
#define ALL_EVENTS 102

#define PAPIRED    "\033[1;31m"
#define PAPINORMAL "\033[0m"


/*
void
test_fail( const char *file, int line, const char *call, int retval )
{ 
//function copied from papi_test.cc since linking is hard and CMake CHates me
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

        if ( PAPI_is_initialized(  ) ) {
                PAPI_shutdown(  );
        }
        exit(1);
} 


int* init_PAPI_events(){
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
  
   std::cout<<"PAPI_start_counters_retval"<<retval<<"\n";
   if ( retval != PAPI_OK  )
        test_fail( __FILE__, __LINE__, "PAPI_start_counters", retval  );

   print_PAPI_events(Events);
   std::cout<< "Instrumented with PAPI version: "<< PAPI_VER_CURRENT <<" \n";
   return Events;
}
*/

namespace DP3 {
  namespace DPPP {

    // @ingroup NDPPP

    // This class contains a single static function that creates and executes
    // the steps defined in the parset file.
    // The parset file is documented on the LOFAR wiki.
    void
    test_fail( const char *file, int line, const char *call, int retval );
    class DPRun
    {
    public:
      // Define the function to create a step from the given parameterset.
      typedef DPStep::ShPtr StepCtor (DPInput*, const class ParameterSet&,
                                      const std::string& prefix);

      // Add a function creating a DPStep to the map.
      static void registerStepCtor (const std::string&, StepCtor*);
      void test_fail( const char *file, int line, const char *call, int retval );
      // Create a step object from the given parameters.
      // It looks up the step type in theirStepMap. If not found, it will
      // try to load a shared library with that name and execute the
      // register function in it.
      static StepCtor* findStepCtor (const std::string& type);

      // Execute the steps defined in the parset file.
      // Possible parameters given at the command line are taken into account.
      static void execute (const std::string& parsetName,
                           int argc=0, char* argv[] = 0);

      // Create the step objects.
      static DPStep::ShPtr makeSteps (const ParameterSet& parset,
                                      const std::string& prefix,
                                      DPInput* reader);

    private:
      // Create an output step, either an MSWriter or an MSUpdater
      // If no data are modified (for example if only count was done),
      // still an MSUpdater is created, but it will not write anything.
      // It reads the output name from the parset. If the prefix is "", it
      // reads msout or msout.name, otherwise it reads name from the output step
      // Create an updater step if an input MS was given; otherwise a writer.
      // Create an updater step only if needed (e.g. not if only count is done).
      // If the user specified an output MS name, a writer or updater is always created
      // If there is a writer, the reader needs to read the visibility data.
      // reader should be the original reader
      static DPStep::ShPtr makeOutputStep(MSReader* reader,
          const ParameterSet& parset, const string& prefix,
          casacore::String& currentMSName);

      // The map to create a step object from its type name.
      static std::map<std::string, StepCtor*> theirStepMap;
    };

  } //# end namespace
}

#endif
