cmake_minimum_required(VERSION 2.8)

# project 
project(shallow)

if(UNIX)
  message(status " Setting GCC flags")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions -g -Wall -std=c++14 -mpopcnt")
elseif(MSVC)
  message(status " Setting MSVC flags")
  # configurations
  set(CMAKE_CONFIGURATION_TYPES "Debug;Release")
   
  # Cxx-flags
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} /D_DEBUG /MTd /Zi /Ob0 /Od /RTC1")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} /GL /Oi /Ot /MT /Ox /Ob2 /D NDEBUG")

  # linker flags
  set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS} /debug")
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS} /LTCG /INCREMENTAL:NO")

  add_definitions(-D_SBCS)
endif()

find_package(Boost COMPONENTS filesystem REQUIRED)
message(status "** Boost Include: ${Boost_INCLUDE_DIR}")
message(status "** Boost Libraries dirs: ${Boost_LIBRARY_DIRS}")
message(status "** Boost Libraries: ${Boost_LIBRARIES}")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories("${CMAKE_SOURCE_DIR}/include" ${Boost_INCLUDE_DIR})
LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})

file(GLOB_RECURSE SOURCE_FILES source/*)
file(GLOB_RECURSE INCLUDE_FILES include/*)

add_executable(shallow ${SOURCE_FILES} ${INCLUDE_FILES})
target_link_libraries(shallow ${Boost_LIBRARIES})
