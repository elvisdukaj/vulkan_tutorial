cmake_minimum_required(VERSION 3.16)
project(vulkan_template LANGUAGES CXX)

find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)

add_executable(${PROJECT_NAME} main.cpp vulkanext.cpp)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_compile_definitions(${PROJECT_NAME} PRIVATE GLFW_INCLUDE_VULKAN)
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan glfw glm::glm)
