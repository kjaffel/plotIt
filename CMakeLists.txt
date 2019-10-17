cmake_minimum_required( VERSION 2.8 )

project(plotIt LANGUAGES CXX)

include(GNUInstallDirs)
include(ExternalProject)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

if(EXISTS "$ENV{CMAKE_PREFIX_PATH}/include/boost")
    message(STATUS "Will use $ENV{CMAKE_PREFIX_PATH} as base path for boost")
    set(BOOST_ROOT $ENV{CMAKE_PREFIX_PATH})
    set(Boost_NO_BOOST_CMAKE ON)
endif()

find_package( ROOT REQUIRED )
find_package( Boost REQUIRED COMPONENTS filesystem regex system )

message(STATUS "Found Boost with include dir ${Boost_INCLUDE_DIR} (change by passing e.g. '-DBoost_NO_BOOST_CMAKE=ON -DBOOST_ROOT=$(scram tool tag boost BOOST_BASE)')")

ExternalProject_Add(
  yaml-cpp
  URL https://github.com/jbeder/yaml-cpp/archive/yaml-cpp-0.6.2.tar.gz
  CMAKE_ARGS -DYAML_CPP_BUILD_TOOLS=OFF -DYAML_CPP_BUILD_CONTRIB=OFF -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/external
  )
ExternalProject_Add(
  tclap
  URL https://github.com/eile/tclap/archive/tclap-1-2-1-release-final.tar.gz
  BUILD_IN_SOURCE 1
  PATCH_COMMAND ./autotools.sh
  CONFIGURE_COMMAND ./configure --prefix=${CMAKE_CURRENT_BINARY_DIR}/external
  )

set(SRCS
  src/plotIt.cc
  src/summary.cc
  src/systematics.cc
  src/TH1Plotter.cc
  src/types.cc
  src/utilities.cc
  src/uuid.cc
  )

add_executable(plotIt ${SRCS})
add_dependencies(plotIt yaml-cpp tclap)
target_include_directories(plotIt PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> PRIVATE ${ROOT_INCLUDE_DIR} ${Boost_INCLUDE_DIRS} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/external/include)
target_link_libraries(plotIt ${Boost_LIBRARIES} ${ROOT_LIBRARIES} ${CMAKE_CURRENT_BINARY_DIR}/external/lib/libyaml-cpp.a)
install(TARGETS plotIt
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
