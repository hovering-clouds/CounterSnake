# ---- CMake requirements ----

cmake_minimum_required(VERSION 3.11)

set(TOP_PROJECT_NAME "OmniSketch")
project(${TOP_PROJECT_NAME})

# ---- CXX standard ----

set(CMAKE_CXX_STANDARD 17)
# To disable warnings in common/toml.h on a few c++20 compiler flags
set(CMAKE_CXX_FLAGS "-Wno-unknown-warning-option -Wno-deprecated-declarations")

# ---- Python Components ----

find_package(Python COMPONENTS Interpreter)

# ---- Include testing ----

include(CTest)

# ---- Include path of header files ----

include_directories(src)

# ---- Submodules ----

add_subdirectory(third_party/fmt)
add_subdirectory(third_party/toml)
# Configuring toml & eigen needs a bit more work
include_directories(third_party/toml/include)
include_directories(third_party/eigen)

# ---- Boost libraries ----

find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

# ---- Compile static libraries ----

add_library(OmniTools src/impl/utils.cpp src/impl/logger.cpp src/impl/data.cpp src/impl/test.cpp src/impl/hash.cpp)
target_link_libraries(OmniTools fmt)

# ---- Add testing ----

add_subdirectory(test)

# ---- Pcap Library ----
set(PCPP_ROOT /home/liu_xunpeng/software/pcap++/PcapPlusPlus-24.09) # add this because my pcap++ is not at normal place
function(check_library)
  find_library(${ARGV0} ${ARGV1} ${PCPP_ROOT}/lib)
  if(NOT ${ARGV0})
    message(FATAL_ERROR "lib ${ARGV1} not found")
  else()
    message(STATUS "lib ${ARGV1} found at ${${ARGV0}}")
  endif()
endfunction(check_library)

if(BUILD_PCAP_PARSER)
  check_library(PCPP Pcap++)
  check_library(PACPP Packet++)
  check_library(COMPP Common++)
  check_library(PCAP pcap)
endif()

# ---- Pcap Parser ----

if(NOT PCPP_INCLUDE_DIR)
  set(PCPP_INCLUDE_DIR ${PCPP_ROOT}/include/pcapplusplus)
  message(STATUS "pcapplusplus is included at ${PCPP_INCLUDE_DIR} by default")
else()
  message(STATUS "pcapplusplus is included at ${PCPP_INCLUDE_DIR}")
endif()

if(BUILD_PCAP_PARSER)
  add_executable(parser ${CMAKE_CURRENT_SOURCE_DIR}/src/pcap_parser/parser.cpp)
  target_include_directories(parser PRIVATE ${PCPP_INCLUDE_DIR})
  target_link_libraries(parser ${PCPP} ${PACPP} ${COMPP} ${PCAP} fmt OmniTools)
  add_executable(parser_kosarak ${CMAKE_CURRENT_SOURCE_DIR}/src/pcap_parser/parser_kosarak.cpp)
  target_include_directories(parser_kosarak PRIVATE ${PCPP_INCLUDE_DIR})
  target_link_libraries(parser_kosarak ${PCPP} ${PACPP} ${COMPP} ${PCAP} fmt OmniTools)
  add_executable(synthesizer ${CMAKE_CURRENT_SOURCE_DIR}/src/pcap_parser/synthesizer.cpp)
  target_link_libraries(synthesizer fmt OmniTools)
endif()

# ---- User-defined sketches ---- #

function(add_user_sketch)
  add_custom_target(${ARGV1}
  DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/driver/${ARGV1}Driver.cpp
  )
  add_custom_command(
    OUTPUT
    ${CMAKE_CURRENT_SOURCE_DIR}/src/driver/${ARGV1}Driver.cpp
    COMMAND
        ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/src/generate_driver.py sketch_test/${ARGV1}Test.h
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/sketch_test/${ARGV1}Test.h
    WORKING_DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}/src
  )
  
  add_executable(${ARGV0} ${CMAKE_CURRENT_SOURCE_DIR}/src/driver/${ARGV1}Driver.cpp)
  target_link_libraries(${ARGV0} OmniTools)
endfunction(add_user_sketch)

# Normal Sketch
add_user_sketch(CM CMSketch)
add_user_sketch(FR FlowRadar)
add_user_sketch(HP HashPipe)
add_user_sketch(DT Deltoid)
add_user_sketch(PR PRSketch)
add_user_sketch(ES ElasticSketch)
add_user_sketch(MV MVSketch)
add_user_sketch(SL SketchLearn)
add_user_sketch(NZE NZESketch)

# Additive Counter Sharing
add_executable(Dway ${CMAKE_CURRENT_SOURCE_DIR}/src/layer_counter/TestDway.cpp)
target_link_libraries(Dway OmniTools)
add_executable(DwayNeg ${CMAKE_CURRENT_SOURCE_DIR}/src/layer_counter/TestDwayNeg.cpp)
target_link_libraries(DwayNeg OmniTools)
add_executable(Brick ${CMAKE_CURRENT_SOURCE_DIR}/src/layer_counter/TestBrick.cpp)
target_link_libraries(Brick OmniTools)
add_executable(Pyramid ${CMAKE_CURRENT_SOURCE_DIR}/src/layer_counter/TestPyramid.cpp)
target_link_libraries(Pyramid OmniTools)
add_executable(Bitsense ${CMAKE_CURRENT_SOURCE_DIR}/src/layer_counter/TestBitsense.cpp)
target_link_libraries(Bitsense OmniTools)
add_executable(BitMatcher ${CMAKE_CURRENT_SOURCE_DIR}/src/layer_counter/TestBitMatcher.cpp)
target_link_libraries(BitMatcher OmniTools)
add_executable(Sac ${CMAKE_CURRENT_SOURCE_DIR}/src/layer_counter/TestSac.cpp)
target_link_libraries(Sac OmniTools)
add_executable(Stingy ${CMAKE_CURRENT_SOURCE_DIR}/src/layer_counter/TestStingy.cpp)
target_link_libraries(Stingy OmniTools)
add_executable(Diamond ${CMAKE_CURRENT_SOURCE_DIR}/src/layer_counter/TestDiamond.cpp)
target_link_libraries(Diamond OmniTools)
