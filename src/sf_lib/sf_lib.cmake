
include_directories(${PROJECT_SOURCE_DIR}/src/sf_lib/sf_hash)
include_directories(${PROJECT_SOURCE_DIR}/src/sf_lib/sf_pool)
include_directories(${PROJECT_SOURCE_DIR}/src/sf_lib/sf_timer)
include_directories(${PROJECT_SOURCE_DIR}/src/sf_lib/sf_tools)
include_directories(${PROJECT_SOURCE_DIR}/src/sf_lib/sf_shm)
include_directories(${PROJECT_SOURCE_DIR}/src/sf_lib/sf_net)
include_directories(${PROJECT_SOURCE_DIR}/src/sf_lib/sf_port)
include_directories(${PROJECT_SOURCE_DIR}/src/sf_lib/sf_flow)

include_directories(${PROJECT_SOURCE_DIR}/src/sf_lib/sf_lock)

#source files
aux_source_directory(${PROJECT_SOURCE_DIR}/src/sf_lib/sf_hash  SF_LIB_SOURCES)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/sf_lib/sf_pool  SF_LIB_SOURCES)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/sf_lib/sf_timer  SF_LIB_SOURCES)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/sf_lib/sf_tools  SF_LIB_SOURCES)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/sf_lib/sf_shm  SF_LIB_SOURCES)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/sf_lib/sf_flow  SF_LIB_SOURCES)

set(SF_LIB_MULTIARCH_SOURCES ${PROJECT_SOURCE_DIR}/src/sf_lib/sf_timer/sf_timer_input.c)
set(SF_LIB_MULTIARCH_SOURCES ${PROJECT_SOURCE_DIR}/src/sf_lib/sf_timer/sf_flow_proc.c)

#message("SF_LIB_SOURCES : ${SF_LIB_SOURCES}")
