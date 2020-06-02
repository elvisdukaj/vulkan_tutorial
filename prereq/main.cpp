#include <iostream>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

using namespace std;

int main() {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  auto window = glfwCreateWindow(800, 600, "Vulkan template", nullptr, nullptr);

  uint32_t extensions;
  vkEnumerateInstanceLayerProperties(&extensions, nullptr);
  cout << "There are " << extensions << " avaiable extensions" << endl;

  glm::mat4 mat;
  glm::vec4 vec;
  auto test = mat * vec;

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}
