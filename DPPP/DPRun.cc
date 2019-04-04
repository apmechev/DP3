//# DPRun.cc: Class to run steps like averaging and flagging on an MS
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

#include <boost/algorithm/string.hpp>

#include "ApplyBeam.h"
#include "ApplyCal.h"
#include "Averager.h"
#include "Counter.h"
#include "Demixer.h"
#include "DemixerNew.h"
#include "DPBuffer.h"
#include "DPInfo.h"
#include "DPLogger.h"
#include "Filter.h"
#include "GainCal.h"
#include "H5ParmPredict.h"
#include "Interpolate.h"
#include "MedFlagger.h"
#include "MSReader.h"
#include "MSUpdater.h"
#include "MSWriter.h"
#include "MultiMSReader.h"
#include "PhaseShift.h"
#include "Predict.h"
#include "PreFlagger.h"
#include "ProgressMeter.h"
#include "ScaleData.h"
#include "Split.h"
#include "StationAdder.h"
#include "UVWFlagger.h"
#include "Upsample.h"

#include "../Common/Timer.h"
#include "../Common/StreamUtil.h"

#include "../AOFlaggerStep/AOFlaggerStep.h"

#include "../DDECal/DDECal.h"

#include <casacore/casa/OS/Path.h>
#include <casacore/casa/OS/DirectoryIterator.h>
#include <casacore/casa/OS/Timer.h>
#include <casacore/casa/OS/DynLib.h>
#include <casacore/casa/Utilities/Regex.h>

namespace DP3 {
  namespace DPPP {

void print_PAPI_events(int *Events){
/*Prints out all the PAPI events in the events_set and their names*/

   long_long values[NUM_EVENTS];
   char code_name[PAPI_MAX_STR_LEN];

   struct timespec tms1;
   struct timespec tms;

   if (clock_gettime(CLOCK_REALTIME,&tms1)) {
           return;
                          }
   int retval = PAPI_read_counters(values, NUM_EVENTS);
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
   //std::cout.precision(4);
   if ( retval != PAPI_OK  )
     test_fail( __FILE__, __LINE__, "PAPI_read_counters", retval  );
   for (int i=0; i<NUM_EVENTS; i++){
//      PAPI_event_code_to_name(Events[i], code_name);
      std::cout<<  int64_t (micros1 + (micros-micros1)/2) <<": Ev_" << i << " = " <<values[i]<<"\n";
   }
   std::cout<<'\n';
 return;
}

void
test_fail( const char *file, int line, const char *call, int retval )
{
//function copied from papi_test.cc since linking is hard and CMake CHates me
        static int TESTS_COLOR = 1;
        char buf[128];

        (void)file;

        memset( buf, '\0', sizeof ( buf ) );

        if (TESTS_COLOR) fprintf(stdout,"%s",PAPIRED);
        fprintf( stdout, "FAILED!!!");
        if (TESTS_COLOR) fprintf(stdout,"%s",PAPINORMAL);
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


std::vector<int> init_PAPI_events(){
   int retval;
   char code_name[PAPI_MAX_STR_LEN];

   int all_events[ALL_EVENTS] =  {PAPI_L1_DCM, PAPI_L2_DCA, PAPI_L3_DCA};//{ PAPI_L1_DCM, PAPI_L1_ICM, PAPI_L2_DCM, PAPI_L2_ICM, PAPI_L3_DCM, PAPI_L3_ICM, PAPI_L1_TCM, PAPI_L2_TCM, PAPI_L3_TCM, PAPI_CA_SNP, PAPI_CA_SHR, PAPI_CA_CLN, PAPI_CA_INV, PAPI_CA_ITV, PAPI_L3_LDM, PAPI_L3_STM, PAPI_BRU_IDL, PAPI_FXU_IDL, PAPI_FPU_IDL, PAPI_LSU_IDL, PAPI_TLB_DM, PAPI_TLB_IM, PAPI_TLB_TL, PAPI_L1_LDM, PAPI_L1_STM, PAPI_L2_LDM, PAPI_L2_STM, PAPI_BTAC_M, PAPI_PRF_DM, PAPI_L3_DCH, PAPI_TLB_SD, PAPI_CSR_FAL, PAPI_CSR_SUC, PAPI_CSR_TOT, PAPI_MEM_SCY, PAPI_MEM_RCY, PAPI_MEM_WCY, PAPI_STL_ICY, PAPI_FUL_ICY, PAPI_STL_CCY, PAPI_FUL_CCY, PAPI_HW_INT, PAPI_BR_UCN, PAPI_BR_CN, PAPI_BR_TKN, PAPI_BR_NTK, PAPI_BR_MSP, PAPI_BR_PRC, PAPI_FMA_INS, PAPI_TOT_IIS, PAPI_TOT_INS, PAPI_INT_INS, PAPI_FP_INS, PAPI_LD_INS, PAPI_SR_INS, PAPI_BR_INS, PAPI_VEC_INS, PAPI_RES_STL, PAPI_FP_STAL, PAPI_TOT_CYC, PAPI_LST_INS, PAPI_SYC_INS, PAPI_L1_DCH, PAPI_L2_DCH, PAPI_L1_DCA, PAPI_L2_DCA, PAPI_L3_DCA, PAPI_L1_DCR, PAPI_L2_DCR, PAPI_L3_DCR, PAPI_L1_DCW, PAPI_L2_DCW, PAPI_L3_DCW, PAPI_L1_ICH, PAPI_L2_ICH, PAPI_L3_ICH, PAPI_L1_ICA, PAPI_L2_ICA, PAPI_L3_ICA, PAPI_L1_ICR, PAPI_L2_ICR, PAPI_L3_ICR, PAPI_L1_ICW, PAPI_L2_ICW, PAPI_L3_ICW, PAPI_L1_TCH, PAPI_L2_TCH, PAPI_L3_TCH, PAPI_L1_TCA, PAPI_L2_TCA, PAPI_L3_TCA, PAPI_L1_TCR, PAPI_L2_TCR, PAPI_L3_TCR, PAPI_L1_TCW, PAPI_L2_TCW, PAPI_L3_TCW, PAPI_FML_INS, PAPI_FAD_INS, PAPI_FDV_INS, PAPI_FSQ_INS, PAPI_FNV_INS };
   std::cout.precision(4);          
   retval = PAPI_library_init(PAPI_VER_CURRENT);
   std::vector<int> event_vec;
   for (int i=0;i<ALL_EVENTS;i++){
       if (PAPI_query_event(all_events[i]) != PAPI_OK ){
       PAPI_event_code_to_name(all_events[i], code_name);
       std::cout<<"PAPI Event "<< code_name <<" cannot be accesssed and will not be used\n";
       }
       else {event_vec.push_back(all_events[i]);
       }
   }

   
   int* Events =  &event_vec[0];
   int num_hwcntrs = 0;

   num_hwcntrs = PAPI_num_counters();
   if (num_hwcntrs>event_vec.size())
       num_hwcntrs=event_vec.size();
   std::cout << num_hwcntrs<<"\n";
   retval = PAPI_start_counters(Events, num_hwcntrs);

   std::cout<<"PAPI_start_counters_retval"<<retval<<"\n";
   if ( retval != PAPI_OK  )
        test_fail( __FILE__, __LINE__, "PAPI_start_counters", retval  );

   print_PAPI_events(Events);
   std::cout<< "Instrumented with PAPI version: "<< PAPI_VER_CURRENT <<" \n";

   return event_vec;
}


    // Initialize the statics.
    std::map<std::string, DPRun::StepCtor*> DPRun::theirStepMap;

    void DPRun::registerStepCtor (const std::string& type, StepCtor* func)
    {
      theirStepMap[type] = func;
    }

    DPRun::StepCtor* DPRun::findStepCtor (const std::string& type)
    {
      std::map<std::string,StepCtor*>::const_iterator iter =
        theirStepMap.find (type);
      if (iter != theirStepMap.end()) {
        return iter->second;
      }
      // Try to load the step from a dynamic library with that name
      // (in lowercase).
      // A dot can be used to have a specific library name (so multiple
      // steps can use the same shared library).
      std::string libname(type);
      boost::algorithm::to_lower(libname);
      string::size_type pos = libname.find_first_of (".");
      if (pos != string::npos) {
        libname = libname.substr (0, pos);
      }
      // Try to load and initialize the dynamic library.
      casacore::DynLib dl(libname, string("libdppp_"), "register_"+libname, false);
      if (dl.getHandle()) {
        // See if registered now.
        iter = theirStepMap.find (type);
        if (iter != theirStepMap.end()) {
          return iter->second;
        }
      }
      throw Exception("Step type " + type +
            " is unknown and no shared library lib" + libname + " or libdppp_" +
            libname + " found in (DY)LD_LIBRARY_PATH");
    }


    void DPRun::execute (const string& parsetName, int argc, char* argv[])
    {
      casacore::Timer timer;
      NSTimer nstimer;
      nstimer.start();
      ParameterSet parset;


      if (! parsetName.empty()) {
        parset.adoptFile (parsetName);
      }
      // Adopt possible parameters given at the command line.


      parset.adoptArgv (argc, argv); //# works fine if argc==0 and argv==0
      DPLogger::useLogger = parset.getBool ("uselogger", false);
      bool showProgress   = parset.getBool ("showprogress", true);
      bool showTimings    = parset.getBool ("showtimings", true);
      // checkparset is an integer parameter now, but accept a bool as well
      // for backward compatibility.


     int checkparset = 0;
      try {
        checkparset = parset.getInt ("checkparset", 0);
      } catch (...) {
        DPLOG_WARN_STR ("Parameter checkparset should be an integer value");
        checkparset = parset.getBool ("checkparset") ? 1:0;
      }
//      std::vector<int> event_vec = init_PAPI_events();
//      int* PAPI_Events = &event_vec[0];
//      event_vec = new std::vector<int>;
      int justAnInt=0;
      int* PAPI_Events = &justAnInt;
      bool showcounts = parset.getBool ("showcounts", true);

      uint numThreads = parset.getInt("numthreads", ThreadPool::NCPUs());

      // Create the steps, link them together
      DPStep::ShPtr firstStep = makeSteps (parset, "", 0);

      DPStep::ShPtr step = firstStep;
      DPStep::ShPtr lastStep;
      while (step) {
        step = step->getNextStep();
      }
      
      // Call updateInfo()
      DPInfo dpInfo;
      dpInfo.setNThreads(numThreads);
      firstStep->setInfo (std::move(dpInfo));

      // Show the steps.  
      step = firstStep;
      while (step) {
        std::ostringstream os;
        step->show (os);
        DPLOG_INFO (os.str(), true);
        lastStep = step;
        step = step->getNextStep();
      }
      if (checkparset >= 0) {
        // Show unused parameters (might be misspelled).
        std::vector<std::string> unused = parset.unusedKeys();
        if (! unused.empty()) {
          DPLOG_WARN_STR
            (
                "\n*** WARNING: the following parset keywords were not used ***"
             << "\n             maybe they are misspelled"
             << "\n    " << unused << std::endl);
          if (checkparset!=0)
            throw Exception("Unused parset keywords found");
        }
      }
      // Process until the end.
      uint ntodo = firstStep->getInfo().ntime();
      DPLOG_INFO_STR ("Processing " << ntodo << " time slots ...");
      

//              print_PAPI_events(PAPI_Events);

      {
        ProgressMeter* progress = 0;
        if (showProgress) {
          progress = new ProgressMeter(0.0, ntodo, "NDPPP",
                                       "Time slots processed",
                                       "", "", true, 1);
        }
        double ndone = 0;
        if (showProgress  &&  ntodo > 0) {
          progress->update (ndone, PAPI_Events, true);
        }
        DPBuffer buf;
        while (firstStep->process (buf)) {
          ++ndone;
          if (showProgress  &&  ntodo > 0) {
            progress->update (ndone, PAPI_Events, true);
          }
        }
        delete progress;
      }
      // Finish the processing.
//      print_PAPI_events(PAPI_Events);


      DPLOG_INFO_STR ("Finishing processing ...");
      firstStep->finish();
      // Give all steps the option to add something to the MS written.
      // It starts with the last step to get the name of the output MS,
      // but each step must first call its previous step before
      // it adds something itself.
      lastStep->addToMS("");

      // Show the counts where needed.
      if (showcounts) {
      step = firstStep;
        while (step) {
          std::ostringstream os;
          step->showCounts (os);
          DPLOG_INFO (os.str(), true);
          step = step->getNextStep();
        }
      }
      // Show the overall timer.
      nstimer.stop();
      double duration = nstimer.getElapsed();
      std::ostringstream ostr;
      ostr << std::endl;
      // Output special line for pipeline use.
      if (DPLogger::useLogger) {
        ostr << "Start timer output" << std::endl;
      }
      timer.show (ostr, "Total NDPPP time");
      DPLOG_INFO (ostr.str(), true);
      if (showTimings) {
        // Show the timings per step.
        step = firstStep;
        while (step) {
          std::ostringstream os;
          step->showTimings (os, duration);
        if (! os.str().empty()) {
          DPLOG_INFO (os.str(), true);
        }
          step = step->getNextStep();
        }
      }
      if (DPLogger::useLogger) {
        ostr << "End timer output\n";
      }
      //print_PAPI_events(PAPI_Events);

      // The destructors are called automatically at this point.
    }

    DPStep::ShPtr DPRun::makeSteps (const ParameterSet& parset,
                                    const string& prefix,
                                    DPInput* reader)
    {
      DPStep::ShPtr firstStep;
      DPStep::ShPtr lastStep;
      if (!reader) {
        // Get input and output MS name.
        // Those parameters were always called msin and msout.
        // However, SAS/MAC cannot handle a parameter and a group with the same
        // name, hence one can also use msin.name and msout.name.
        std::vector<string> inNames = parset.getStringVector ("msin.name",
                                                         std::vector<string>());
        if (inNames.empty()) {
          inNames = parset.getStringVector ("msin"); //Error happens here
        }
        if (inNames.size() == 0)
          throw Exception("No input MeasurementSets given");
        // Find all file names matching a possibly wildcarded input name.
        // This is only possible if a single name is given.
        if (inNames.size() == 1) {
          if (inNames[0].find_first_of ("*?{['") != string::npos) {
            std::vector<string> names;
            names.reserve (80);
            casacore::Path path(inNames[0]);
            casacore::String dirName(path.dirName());
            casacore::Directory dir(dirName);
            // Use the basename as the file name pattern.
            casacore::DirectoryIterator dirIter (dir,
                                             casacore::Regex::fromPattern(path.baseName()));
            while (!dirIter.pastEnd()) {
              names.push_back (dirName + '/' + dirIter.name());
              dirIter++;
            }
            if (names.empty())
              throw Exception("No datasets found matching msin "
                       + inNames[0]);
            inNames = names;
          }
        }

        // Get the steps.
        // Currently the input MS must be given.
        // In the future it might be possible to have a simulation step instead.
        // Create MSReader step if input ms given.
        if (inNames.size() == 1) {
          reader = new MSReader (inNames[0], parset, "msin.");
        } else {
          reader = new MultiMSReader (inNames, parset, "msin.");
        }
        firstStep = DPStep::ShPtr (reader);
      }

      casacore::Path pathIn (reader->msName());
      casacore::String currentMSName (pathIn.absoluteName());

      // Create the other steps.
      std::vector<string> steps = parset.getStringVector (prefix + "steps");
      lastStep = firstStep;
      DPStep::ShPtr step;
      for (std::vector<string>::const_iterator iter = steps.begin();
           iter != steps.end(); ++iter) {
        string prefix(*iter + '.');
        // The alphabetic part of the name is the default step type.
        // This allows names like average1, out3.
        string defaulttype = (*iter);
        while (defaulttype.size()>0 && std::isdigit(*defaulttype.rbegin())) {
          defaulttype.resize(defaulttype.size()-1);
        }

        string type = parset.getString(prefix+"type", defaulttype);
        boost::algorithm::to_lower(type);
        // Define correct name for AOFlagger synonyms.
        if (type == "aoflagger" || type == "aoflag") {
          step = DPStep::ShPtr(new AOFlaggerStep (reader, parset, prefix));
        } else if (type == "averager"  ||  type == "average"  ||  type == "squash") {
          step = DPStep::ShPtr(new Averager (reader, parset, prefix));
        } else if (type == "madflagger"  ||  type == "madflag") {
          step = DPStep::ShPtr(new MedFlagger (reader, parset, prefix));
        } else if (type == "preflagger"  ||  type == "preflag") {
          step = DPStep::ShPtr(new PreFlagger (reader, parset, prefix));
        } else if (type == "uvwflagger"  ||  type == "uvwflag") {
          step = DPStep::ShPtr(new UVWFlagger (reader, parset, prefix));
        } else if (type == "counter"  ||  type == "count") {
          step = DPStep::ShPtr(new Counter (reader, parset, prefix));
        } else if (type == "phaseshifter"  ||  type == "phaseshift") {
          step = DPStep::ShPtr(new PhaseShift (reader, parset, prefix));
#ifdef HAVE_LOFAR_BEAM
        } else if (type == "demixer"  ||  type == "demix") {
          step = DPStep::ShPtr(new Demixer (reader, parset, prefix));
        } else if (type == "smartdemixer"  ||  type == "smartdemix") {
          step = DPStep::ShPtr(new DemixerNew (reader, parset, prefix));
        } else if (type == "applybeam") {
          step = DPStep::ShPtr(new ApplyBeam (reader, parset, prefix));
#endif
        } else if (type == "stationadder"  ||  type == "stationadd") {
          step = DPStep::ShPtr(new StationAdder (reader, parset, prefix));
        } else if (type == "scaledata") {
          step = DPStep::ShPtr(new ScaleData (reader, parset, prefix));
        } else if (type == "filter") {
          step = DPStep::ShPtr(new Filter (reader, parset, prefix));
        } else if (type == "applycal"  ||  type == "correct") {
          step = DPStep::ShPtr(new ApplyCal (reader, parset, prefix));
        } else if (type == "predict") {
          step = DPStep::ShPtr(new Predict (reader, parset, prefix));
        } else if (type == "h5parmpredict") {
          step = DPStep::ShPtr(new H5ParmPredict (reader, parset, prefix));
        } else if (type == "gaincal"  ||  type == "calibrate") {
          step = DPStep::ShPtr(new GainCal (reader, parset, prefix));
        } else if (type == "upsample") {
          step = DPStep::ShPtr(new Upsample (reader, parset, prefix));
        } else if (type == "split" || type == "explode") {
          step = DPStep::ShPtr(new Split (reader, parset, prefix));
        } else if (type == "ddecal") {
          step = DPStep::ShPtr(new DDECal (reader, parset, prefix));
        } else if (type == "interpolate") {
          step = DPStep::ShPtr(new Interpolate (reader, parset, prefix));
        } else if (type == "out" || type=="output" || type=="msout") {
          step = makeOutputStep(dynamic_cast<MSReader*>(reader), parset, prefix, currentMSName);
        } else {
          // Maybe the step is defined in a dynamic library.
          step = findStepCtor(type) (reader, parset, prefix);
        }
        if (lastStep) {
          lastStep->setNextStep (step);
        }
        lastStep = step;
        // Define as first step if not defined yet.
        if (!firstStep) {
          firstStep = step;
        }
      }
      // Add an output step if not explicitly added in steps (unless last step is a 'split' step)
      if (steps.size()==0 || (
          steps[steps.size()-1] != "out" &&
          steps[steps.size()-1] != "output" &&
          steps[steps.size()-1] != "msout" &&
          steps[steps.size()-1] != "split")) {
        step = makeOutputStep(dynamic_cast<MSReader*>(reader), parset, "msout.", currentMSName);
        lastStep->setNextStep (step);
        lastStep = step;
      }

      // Add a null step, so the last step can use getNextStep->process().
      DPStep::ShPtr nullStep(new NullStep());
      if (lastStep) {
        lastStep->setNextStep (nullStep);
      } else {
        firstStep = nullStep;
      }
      return firstStep;
    }

    DPStep::ShPtr DPRun::makeOutputStep (MSReader* reader,
                                         const ParameterSet& parset,
                                         const string& prefix,
                                         casacore::String& currentMSName)
    {
      DPStep::ShPtr step;
      casacore::String outName;
      bool doUpdate = false;
      if (prefix == "msout.") {
        // The last output step.
        outName = parset.getString ("msout.name", "");
        if (outName.empty()) {
          outName = parset.getString ("msout");
        }
      } else {
        // An intermediate output step.
        outName = parset.getString(prefix + "name");
      }

      // A name equal to . or the last name means an update of the last MS.
      if (outName.empty()  ||  outName == ".") {
        outName  = currentMSName;
        doUpdate = true;
      } else {
        casacore::Path pathOut(outName);
        if (currentMSName == pathOut.absoluteName()) {
          outName  = currentMSName;
          doUpdate = true;
        }
      }
      if (doUpdate) {
        // Create MSUpdater.
        // Take care the history is not written twice.
        // Note that if there is nothing to write, the updater won't do anything.
        step = DPStep::ShPtr(new MSUpdater(dynamic_cast<MSReader*>(reader),
                                           outName, parset, prefix,
                                           outName!=currentMSName));
      } else {
        step = DPStep::ShPtr(new MSWriter (reader, outName, parset, prefix));
        reader->setReadVisData (true);
      }
      casacore::Path pathOut(outName);
      currentMSName = pathOut.absoluteName();
      return step;
    }


  } //# end namespace
}
