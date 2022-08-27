cmake_minimum_required(VERSION 3.9)

# project 
project(shallow)

if(UNIX)
  message(status " Setting GCC flags")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions -g -Wall -std=c++14 -mpopcnt")
  execute_process(COMMAND bash "-c" "git log -1 --format=\"%H\" | cut -c1-4" OUTPUT_VARIABLE HASHCOMMIT OUTPUT_STRIP_TRAILING_WHITESPACE)
  message(status " Commit: " ${HASHCOMMIT})
  string(CONCAT OUTPUT_EXEC_NAME ${PROJECT_NAME} "_" ${VERSION})
  message(status " Executable name: " ${OUTPUT_EXEC_NAME})
  message(status " Git hash commit: " ${VERSION})
  message(status " Executable file name: " ${OUTPUT_EXEC_NAME})
  
elseif(MSVC)
  message(status " Setting MSVC flags")
  #log -1 --format=%H
  message(${PROJECT_SOURCE_DIR})
  execute_process(COMMAND "git.exe" "log" "-1" "--format=%H" WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} OUTPUT_VARIABLE FULLHASHCOMMIT OUTPUT_STRIP_TRAILING_WHITESPACE)
  message(status " Commit: " ${FULLHASHCOMMIT})
  string(SUBSTRING ${FULLHASHCOMMIT} 0 4 HASHCOMMIT)
  execute_process(COMMAND "git.exe" "describe" "--abbrev=0" WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} OUTPUT_VARIABLE FULLTAG OUTPUT_STRIP_TRAILING_WHITESPACE)
  message(status " Version: " ${FULLTAG})
  
  if (FULLTAG MATCHES "^([a-zA-Z]*)([0-9])([.]*)$")
    set(VERSIONTAG ${CMAKE_MATCH_2})
    message(STATUS "Version found: " ${VERSIONTAG})
  else()
    message(FATAL_ERROR "mismatch in " ${FULLTAG})
  endif()
  
  string(CONCAT MSVC_TARGET_NAME_POSTFIX ${PROJECT_NAME}.${HASHCOMMIT})
  string(CONCAT MSVC_TARGET_NAME_DEBUG ${PROJECT_NAME} "_d")
  string(CONCAT MSVC_TARGET_NAME_RELEASDEBUGINFO ${PROJECT_NAME} "_rd")
  string(CONCAT MSVC_TARGET_NAME_RELEASESPECIAL ${PROJECT_NAME} "_s")
  string(CONCAT MSVC_TARGET_NAME_RELEASEDEPLOY ${PROJECT_NAME} "_v${VERSIONTAG}_${CMAKE_GENERATOR_PLATFORM}")
  message(status ${MSVC_TARGET_NAME_POSTFIX})
  set(OUTPUT_EXEC_NAME ${PROJECT_NAME})
  
  # configurations
  set(CMAKE_CONFIGURATION_TYPES "Debug;Release;ReleaseDebugInfo;ReleaseSpecial;ReleaseDeploy")
   
  # Cxx-flags
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} /D_DEBUG /MTd /Zi /Ob0 /Od /RTC1")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} /GL /Oi /Ot /MT /Ox /Ob2 /D NDEBUG")
  set(CMAKE_CXX_FLAGS_RELEASEDEBUGINFO "${CMAKE_CXX_FLAGS} /DRELEASEDEBUGINFO /MT /Zi /Ob0 /Od /D NDEBUG")
  set(CMAKE_CXX_FLAGS_RELEASESPECIAL "${CMAKE_CXX_FLAGS} /DRELEASESPECIAL /GL /Oi /Ot /MT /Ox /Ob2 /D NDEBUG")
  set(CMAKE_CXX_FLAGS_RELEASEDEPLOY "${CMAKE_CXX_FLAGS} /DRELEASEDEPLOY /GL /Oi /Ot /MT /Ox /Ob2 /D NDEBUG")

  # linker flags
  set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS} /debug")
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS} /LTCG /INCREMENTAL:NO")
  set(CMAKE_EXE_LINKER_FLAGS_RELEASEDEBUGINFO "${CMAKE_EXE_LINKER_FLAGS} /debug")
  set(CMAKE_EXE_LINKER_FLAGS_RELEASESPECIAL "${CMAKE_EXE_LINKER_FLAGS} /LTCG /INCREMENTAL:NO")
  set(CMAKE_EXE_LINKER_FLAGS_RELEASEDEPLOY "${CMAKE_EXE_LINKER_FLAGS} /LTCG /INCREMENTAL:NO")

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

add_executable(${OUTPUT_EXEC_NAME} ${SOURCE_FILES} ${INCLUDE_FILES})
target_link_libraries(${OUTPUT_EXEC_NAME})
set_target_properties(${OUTPUT_EXEC_NAME} PROPERTIES OUTPUT_NAME_RELEASE ${MSVC_TARGET_NAME_POSTFIX})
set_target_properties(${OUTPUT_EXEC_NAME} PROPERTIES OUTPUT_NAME_DEBUG ${MSVC_TARGET_NAME_DEBUG})
set_target_properties(${OUTPUT_EXEC_NAME} PROPERTIES OUTPUT_NAME_RELEASEDEBUGINFO ${MSVC_TARGET_NAME_RELEASDEBUGINFO})
set_target_properties(${OUTPUT_EXEC_NAME} PROPERTIES OUTPUT_NAME_RELEASESPECIAL ${MSVC_TARGET_NAME_RELEASESPECIAL})
set_target_properties(${OUTPUT_EXEC_NAME} PROPERTIES OUTPUT_NAME_RELEASEDEPLOY ${MSVC_TARGET_NAME_RELEASEDEPLOY})
