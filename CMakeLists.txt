cmake_minimum_required (VERSION 3.5.1)
project(two_state_information_filter)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
set(CMAKE_CXX_STANDARD 14)
add_compile_options(-Wall -Wextra -Wpedantic)

# Note: the warning about unused functions has been silenced because it has been found to give
# wrong warnings about static functions in rotation.h and timing.h, which are used in the code.
add_compile_options(-Wno-unused-function)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(TSIF_VERBOSE 0 CACHE STRING "Verbose flag")
add_definitions(-DTSIF_VERBOSE=${TSIF_VERBOSE})

find_package(catkin REQUIRED)
find_package(Eigen3 REQUIRED)

include_directories(
  include
  ${catkin_INCLUDE_DIRS}
  ${EIGEN3_INCLUDE_DIR}
)

add_executable(test_state src/test_state.cpp)
add_executable(test_quaternion src/test_quaternion.cpp)
add_executable(test_unit_vector src/test_unit_vector.cpp)
add_executable(test_timeline src/test_timeline.cpp)
add_executable(test_imu_gps src/test_imu_gps.cpp src/simulator.cpp src/camera.cpp)

add_executable(mytest src/test.cpp)


if(ENABLE_OPENCV)
  set(OpenCV_DIR /opt/ros/kinetic/share/OpenCV-3.2.0-dev)
  find_package(OpenCV REQUIRED)
  add_executable(test_vio src/test_vio.cpp src/simulator.cpp src/camera.cpp)
  target_link_libraries(test_vio ${OpenCV_LIBS})
endif()

catkin_package(
  INCLUDE_DIRS include ${EIGEN3_INCLUDE_DIR}
  LIBRARIES ${PROJECT_NAME}
  CATKIN_DEPENDS
)

add_library(${PROJECT_NAME}
  src/camera.cpp
  src/simulator.cpp
)

target_link_libraries(${PROJECT_NAME} ${catkin_LIBRARIES} pthread)

# Install header
install(DIRECTORY include/tsif/
  DESTINATION ${CATKIN_GLOBAL_INCLUDE_DESTINATION}/tsif/
)

# Install executables and library
install(TARGETS 
          ${PROJECT_NAME}
          test_state 
          test_quaternion 
          test_unit_vector 
          test_timeline 
          test_imu_gps mytest
  ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
)

# Install config 
install(DIRECTORY cfg
  DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}
)

#############
## Testing ##
#############
if (CATKIN_ENABLE_TESTING)
  catkin_add_gtest(test_${PROJECT_NAME}
    test/empty_test.cpp
  )
  target_include_directories(test_${PROJECT_NAME} PRIVATE
    include
  )
  target_include_directories(test_${PROJECT_NAME} SYSTEM PUBLIC
    ${catkin_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIR}
  )
  target_link_libraries(test_${PROJECT_NAME}
    gtest_main
    ${PROJECT_NAME}
  )

  ###################
  ## Code_coverage ##
  ###################
  find_package(cmake_code_coverage QUIET)
  if(cmake_code_coverage_FOUND)
    add_gtest_coverage(
      TEST_BUILD_TARGETS
        test_${PROJECT_NAME}
    )
  endif()
endif()

#################
## Clang_tools ##
#################
find_package(cmake_clang_tools QUIET)
if(cmake_clang_tools_FOUND)
  add_default_clang_tooling(
    DISABLE_CLANG_FORMAT
  )
endif(cmake_clang_tools_FOUND)
