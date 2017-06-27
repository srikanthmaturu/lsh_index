cmake_minimum_required(VERSION 3.4)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules")
include(AppendCompilerFlags)

enable_testing()

project(lsh_index C CXX)

append_cxx_compiler_flags("-msse4.2 -std=c++14 -Wall -DNDEBUG -fopenmp" "GCC" CMAKE_CXX_FLAGS)
append_cxx_compiler_flags("-O3 -ffast-math -funroll-loops" "GCC" CMAKE_CXX_FLAGS)


set(CMAKE_CXX_STANDARD 14)

INCLUDE_DIRECTORIES(${CMAKE_HOME_DIRECTORY}/include/
                    ${CMAKE_HOME_DIRECTORY}/src/)

set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

set(BOOST_LIBRARYDIR "/util/opt/boost/1.59/gcc/5.4/lib")
set(BOOST_INCLUDEDIR "/util/opt/boost/1.59/gcc/5.4/include")

find_package(Boost COMPONENTS serialization filesystem REQUIRED)

if(Boost_FOUND)
    INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
    message("Boost Version: ${Boost_VERSION}")
    message("Boost found")
else()
    message("Boost not found")
endif()

ADD_SUBDIRECTORY(lib)

add_library(xxhash include/min_hash_index/xxhash.c)
ADD_EXECUTABLE(utility src/utility.cpp)

FILE(STRINGS ${CMAKE_HOME_DIRECTORY}/exp0.config exp0.lines REGEX "^[^#].*")
FOREACH(line ${exp0.lines})
    MESSAGE("line = ${line}")
    LIST(GET line 0 index_name)
    LIST(GET line 1 index_type)
    MESSAGE("Exp 0 ${index_name}; ${index_type};")
    STRING(REGEX REPLACE "min_hash_idx<([^>]+)>" "\\1" t_part ${index_type})
    STRING(REGEX REPLACE "," ";" t_params ${t_part})
    #[[STRING(REGEX REPLACE "," "_" t_suffix ${t_part})
    SET(exec ${index_name}_${t_suffix})]]
    LIST(GET t_params 0 UINT_TYPE)
    LIST(GET t_params 1 W_SIZE)
    LIST(GET t_params 2 APP_SEQ_LEN)
    LIST(GET t_params 3 T_LIM)
    LIST(GET t_params 4 THRESHOLD)
    LIST(GET t_params 5 N_PERMS)
    LIST(LENGTH t_params p_size)
    STRING(REGEX REPLACE "\\." "_" TH ${THRESHOLD})
    IF(${p_size} EQUAL 6)
        SET(exec ${index_name}_UIT_${UINT_TYPE}_WS_${W_SIZE}_ASL_${APP_SEQ_LEN}_TH_${TH}_TL_${T_LIM}_NP_${N_PERMS})
    ELSE()
        LIST(GET t_params 7 N_SEGS)
        LIST(GET t_params 8 SG_SIZE)
        SET(exec ${index_name}_UIT_${UINT_TYPE}_WS_${W_SIZE}_ASL_${APP_SEQ_LEN}_TH_${TH}_TL_${T_LIM}_NP_${N_PERMS}_NS_${N_SEGS}_SGS_${SG_SIZE})
    ENDIF()

    ADD_EXECUTABLE(${exec} src/index.cpp)
    TARGET_LINK_LIBRARIES(${exec} ${Boost_LIBRARIES} xxhash)
    SET_PROPERTY(TARGET ${exec} PROPERTY COMPILE_DEFINITIONS
                    INDEX_NAME="${index_name}"
                    LSH_INDEX_TYPE=${index_type}
                    UINT_TYPE=${UINT_TYPE}
                    W_SIZE=${W_SIZE}
                    APP_SEQ_LEN=${APP_SEQ_LEN}
                    THRESHOLD=${THRESHOLD}
                    T_LIM=${T_LIM}
                    N_PERMS=${N_PERMS})
    message("Target ${exec}")
    message("Index Type ${index_type}")
ENDFOREACH()
