add_library(dppp_testdyndppp SHARED
    TestDynStep.cc ${DPPP_OBJECT} ${PARMDB_OBJECT} ${BLOB_OBJECT} ${COMMON_OBJECT} /home/apmechev/GRIDTOOLS/papi/src/papi/lib/libpapi.a
)

set_target_properties(dppp_testdyndppp PROPERTIES VERSION ${DPPP_GLOBAL_SOVERSION})

#add_subdirectory(test)
