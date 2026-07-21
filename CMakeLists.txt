cmake_minimum_required(VERSION 3.24)

project(
  robot_design_copilot
  VERSION 0.1.0
  DESCRIPTION "Early-stage robot design assistant"
  LANGUAGES CXX
)

include(CTest)
include(FetchContent)

option(
  ROBOT_ENGINE_FETCH_DEPENDENCIES
  "Download missing C++ dependencies during configure"
  ON
)
option(
  ROBOT_ENGINE_ENABLE_ROBOTICS
  "Build Pinocchio kinematics and Ceres integration"
  OFF
)

find_package(nlohmann_json 3.12.0 CONFIG QUIET)
if(NOT nlohmann_json_FOUND)
  if(NOT ROBOT_ENGINE_FETCH_DEPENDENCIES)
    message(FATAL_ERROR "nlohmann_json 3.12.0 is required")
  endif()
  FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(nlohmann_json)
endif()

find_package(Eigen3 3.4.1 EXACT CONFIG QUIET)
if(NOT Eigen3_FOUND)
  if(NOT ROBOT_ENGINE_FETCH_DEPENDENCIES)
    message(FATAL_ERROR "Eigen 3.4.1 is required")
  endif()
  set(EIGEN_BUILD_DOC OFF CACHE BOOL "" FORCE)
  set(EIGEN_BUILD_TESTING OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    Eigen3
    URL https://gitlab.com/libeigen/eigen/-/archive/3.4.1/eigen-3.4.1.tar.bz2
    URL_HASH SHA256=8bb7280b7551bf06418d11a9671fdf998cb927830cf21589b394382d26779821
    DOWNLOAD_EXTRACT_TIMESTAMP FALSE
  )
  FetchContent_MakeAvailable(Eigen3)
endif()

if(ROBOT_ENGINE_ENABLE_ROBOTICS)
  find_package(pinocchio 3.8.0 CONFIG REQUIRED)
  find_package(Ceres 2.2.0 CONFIG REQUIRED)
endif()

add_subdirectory(engine)
