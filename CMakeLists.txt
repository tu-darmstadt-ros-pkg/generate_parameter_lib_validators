cmake_minimum_required(VERSION 3.16)
project(generate_parameter_lib_validators)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(tl-expected REQUIRED)

# Header-only INTERFACE library
add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(
  ${PROJECT_NAME}
  INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>)

target_link_libraries(${PROJECT_NAME} INTERFACE rclcpp::rclcpp tl::expected)

# --- Installation ---
install(DIRECTORY include/ DESTINATION include)

install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}Targets)

ament_export_targets(${PROJECT_NAME}Targets HAS_LIBRARY_TARGET)
ament_export_include_directories(include)
ament_export_dependencies(rclcpp tl-expected)

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  ament_add_gtest(test_valid_topic_name test/test_valid_topic_name.cpp)
  target_link_libraries(test_valid_topic_name ${PROJECT_NAME})
endif()

# ament_package handles this automatically when passed as CONFIG_EXTRAS
ament_package(CONFIG_EXTRAS "cmake/${PROJECT_NAME}-extras.cmake")
