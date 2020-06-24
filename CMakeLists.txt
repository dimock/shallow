cmake_minimum_required(VERSION 3.0)

# project 
project(shallow)

if(UNIX)
  message(status " Setting GCC flags")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions -g -Wall -std=c++14 -mpopcnt")
  execute_process(COMMAND bash "-c" "git log -1 --format=\"%H\" | cut -c1-6" OUTPUT_VARIABLE VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
  message(status " Version: " ${VERSION})
  string(CONCAT OUTPUT_EXEC_NAME ${PROJECT_NAME} "_" ${VERSION})
  message(status " Executable name: " ${OUTPUT_EXEC_NAME})
  
elseif(MSVC)
  message(status " Setting MSVC flags")

  set(OUTPUT_EXEC_NAME ${PROJECT_NAME})

  # configurations
  set(CMAKE_CONFIGURATION_TYPES "Debug;Release;ReleaseDebugInfo")
   
  # Cxx-flags
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} /D_DEBUG /MTd /Zi /Ob0 /Od /RTC1")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} /GL /Oi /Ot /MT /Ox /Ob2 /D NDEBUG")
  set(CMAKE_CXX_FLAGS_RELEASEDEBUGINFO "${CMAKE_CXX_FLAGS} /DRELEASEDEBUGINFO /MT /Zi /Ob0 /Od /D NDEBUG")

  # linker flags
  set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS} /debug")
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS} /LTCG /INCREMENTAL:NO")
  set(CMAKE_EXE_LINKER_FLAGS_RELEASEDEBUGINFO "${CMAKE_EXE_LINKER_FLAGS} /debug")

  add_definitions(-D_SBCS)
endif()

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_STATIC_RUNTIME ON)

find_package(Boost COMPONENTS filesystem REQUIRED)
if(Boost_FOUND)
    message(STATUS "Boost success!")
endif()
message(status "** Boost Include: ${Boost_INCLUDE_DIR}")
message(status "** Boost Libraries dirs: ${Boost_LIBRARY_DIRS}")
message(status "** Boost Libraries: ${Boost_LIBRARIES}")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories("${CMAKE_SOURCE_DIR}/include" ${Boost_INCLUDE_DIR})
LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})

file(GLOB_RECURSE SOURCE_FILES source/*)
file(GLOB_RECURSE INCLUDE_FILES include/*)

message(status " Git hash commit: " ${VERSION})
message(status " Executable file name: " ${OUTPUT_EXEC_NAME})

add_executable(${OUTPUT_EXEC_NAME} ${SOURCE_FILES} ${INCLUDE_FILES})
target_link_libraries(${OUTPUT_EXEC_NAME} ${Boost_LIBRARIES} ${Boost_FILESYSTEM_LIBRARY})
# ${Boost_SYSTEM_LIBRARY})
