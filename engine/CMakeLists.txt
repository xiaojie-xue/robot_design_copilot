add_library(robot-engine-core STATIC
  src/dependencies.cpp
  src/protocol/handler.cpp
  src/protocol/frame.cpp
  src/protocol/session.cpp
)

if(ROBOT_ENGINE_ENABLE_ROBOTICS)
  target_sources(robot-engine-core PRIVATE src/kinematics/reference_arm.cpp)
endif()

add_library(robot::engine-core ALIAS robot-engine-core)

target_compile_features(robot-engine-core PUBLIC cxx_std_20)
target_include_directories(robot-engine-core
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(
  robot-engine-core
  PRIVATE Eigen3::Eigen nlohmann_json::nlohmann_json Threads::Threads
)

if(ROBOT_ENGINE_ENABLE_ROBOTICS)
  target_link_libraries(
    robot-engine-core
    PUBLIC pinocchio::pinocchio Ceres::ceres
  )
  target_compile_definitions(robot-engine-core PUBLIC ROBOT_ENGINE_WITH_ROBOTICS=1)
endif()
target_compile_definitions(
  robot-engine-core
  PRIVATE ROBOT_ENGINE_VERSION="${PROJECT_VERSION}"
)

if(MSVC)
  target_compile_options(robot-engine-core PRIVATE /W4 /permissive-)
else()
  target_compile_options(robot-engine-core PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
endif()

add_executable(robot-engine-transport-echo
  src/cli/transport_echo.cpp
)
target_link_libraries(robot-engine-transport-echo PRIVATE robot::engine-core)

add_executable(robot-engine-cli
  src/cli/main.cpp
)
target_link_libraries(robot-engine-cli PRIVATE robot::engine-core)

if(BUILD_TESTING)
  add_executable(robot-engine-frame-tests
    tests/frame_test.cpp
  )
  target_link_libraries(robot-engine-frame-tests PRIVATE robot::engine-core)
  add_test(NAME robot-engine-frame-tests COMMAND robot-engine-frame-tests)

  add_executable(robot-engine-protocol-tests
    tests/protocol_test.cpp
  )
  target_link_libraries(
    robot-engine-protocol-tests
    PRIVATE robot::engine-core nlohmann_json::nlohmann_json
  )
  target_compile_definitions(
    robot-engine-protocol-tests
    PRIVATE
      ROBOT_PROTOCOL_FIXTURE_DIR="${PROJECT_SOURCE_DIR}/protocol/v1/examples"
  )
  add_test(NAME robot-engine-protocol-tests COMMAND robot-engine-protocol-tests)

  add_executable(robot-engine-session-tests
    tests/session_test.cpp
  )
  target_link_libraries(
    robot-engine-session-tests
    PRIVATE robot::engine-core nlohmann_json::nlohmann_json
  )
  add_test(NAME robot-engine-session-tests COMMAND robot-engine-session-tests)

  if(ROBOT_ENGINE_ENABLE_ROBOTICS)
    add_executable(robot-engine-kinematics-tests
      tests/kinematics_test.cpp
    )
    target_link_libraries(robot-engine-kinematics-tests PRIVATE robot::engine-core)
    add_test(NAME robot-engine-kinematics-tests COMMAND robot-engine-kinematics-tests)
  endif()

  find_package(Python3 COMPONENTS Interpreter REQUIRED)
  add_test(
    NAME robot-engine-cli-integration
    COMMAND
      ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/tests/cli_integration_test.py
      $<TARGET_FILE:robot-engine-cli>
  )
endif()
