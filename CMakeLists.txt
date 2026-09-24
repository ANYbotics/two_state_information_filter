cmake_minimum_required(VERSION 3.16.3)
project(two_state_information_filter)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
set(CMAKE_CXX_STANDARD 17)
add_compile_options(
  -Wall
  -Wextra
  -Wpedantic
)

# Note: the warning about unused functions has been silenced because it has been found to give
# wrong warnings about static functions in rotation.h and timing.h, which are used in the code.
add_compile_options(-Wno-unused-function)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(TSIF_VERBOSE 0 CACHE STRING "Verbose flag")
add_definitions(-DTSIF_VERBOSE=${TSIF_VERBOSE})

set(
  PACKAGE_DEPENDENCIES
  Threads
  Eigen3
)

find_package(ament_cmake REQUIRED)

# NOTE(apoghosov): what I am trying to do here is to port to ROS2 the best way possible but I can
# not really take care about fixing everyting comming from ROS1. So, I leave it up maintainers to
# adjust dependencies and define the transitive conditional compiler flags for dependents properly ...

foreach(PKG ${PACKAGE_DEPENDENCIES})
  find_package(${PKG} REQUIRED)
endforeach()

add_library(${PROJECT_NAME} INTERFACE)

target_link_libraries(${PROJECT_NAME} INTERFACE Eigen3::Eigen)

target_include_directories(
  ${PROJECT_NAME}
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/tsif>
)

ament_target_dependencies(${PROJECT_NAME} ${PACKAGE_DEPENDENCIES})

#############
## Install ##
#############

install(DIRECTORY include/ DESTINATION include/tsif)

install(
  TARGETS
    ${PROJECT_NAME}
  EXPORT export_${PROJECT_NAME}
  LIBRARY
    DESTINATION lib
  ARCHIVE
    DESTINATION lib
  RUNTIME
    DESTINATION bin
)

ament_export_targets(export_${PROJECT_NAME} HAS_LIBRARY_TARGET)
ament_export_dependencies(${PACKAGE_DEPENDENCIES})

#############
## Testing ##
#############

if(BUILD_TESTING)
  add_executable(test_state src/test_state.cpp)
  target_link_libraries(test_state ${PROJECT_NAME})

  add_executable(test_quaternion src/test_quaternion.cpp)
  target_link_libraries(test_quaternion ${PROJECT_NAME})

  add_executable(test_unit_vector src/test_unit_vector.cpp)
  target_link_libraries(test_unit_vector ${PROJECT_NAME})

  add_executable(test_timeline src/test_timeline.cpp)
  target_link_libraries(test_timeline ${PROJECT_NAME})

  find_package(ament_cmake_gtest REQUIRED)

  ament_add_gtest(test_${PROJECT_NAME}
    test/empty_test.cpp
  )

  ###################
  ## Code_coverage ##
  ###################

  find_package(cmake_code_coverage QUIET)

  if(cmake_code_coverage_FOUND)
    add_gtest_coverage(TEST_BUILD_TARGETS test_${PROJECT_NAME})
  endif()
endif()

ament_package()
