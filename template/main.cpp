#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace std;

class Template {
public:
  Template(const char* title, int width = 800, int height = 600, bool debug = false)
      : mTitle{title}, mWidth{width}, mHeight{height}, mDebug{debug} {
    initWindow();
    initVulkan();
  }

  ~Template() {
    releaseVulkan();
    releaseWindow();
  }

  int run() {
    while (!glfwWindowShouldClose(mWindow)) {
      glfwPollEvents();
    }

    return 0;
  }

private:
  void initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    mWindow = glfwCreateWindow(mWidth, mHeight, mTitle, nullptr, nullptr);
  }

  void initVulkan() {
    initVulkanInstance();
    setupDebugMessenger();
    initDevice();
  }

  void releaseVulkan() {
    vkDestroyDebugUtilsMessengerEXT(mVulkan, mDebugMessenger, nullptr);
    vkDestroyInstance(mVulkan, nullptr);
  }

  void releaseWindow() {
    glfwDestroyWindow(mWindow);
    glfwTerminate();
  }

  void initVulkanInstance() {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = mTitle;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    auto requiredWindowExtensions = getWindowsRequiredExtensions();
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = requiredWindowExtensions.size();
    createInfo.ppEnabledExtensionNames = requiredWindowExtensions.data();

    const char* VALIDATION_LAYER = "VK_LAYER_KHRONOS_validation";
    std::vector<const char*> layers = {VALIDATION_LAYER};
    auto useValidation = isLayerSupport(VALIDATION_LAYER) && mDebug;

    if (mDebug) {
      auto supportedLayers = enumerateVulkanLayers();
      cout << "Layers: " << endl;
      for (const auto& layer : supportedLayers)
        cout << '\t' << layer.layerName << endl;
      cout << "Validation layer: " << boolalpha << useValidation << endl;
    }

    createInfo.enabledLayerCount = useValidation ? layers.size() : 0;
    createInfo.ppEnabledLayerNames = useValidation ? layers.data() : nullptr;

    if (mDebug) {
      auto extensions = vulkanExtensionProperties();
      cout << "Extensions: " << endl;
      for (const auto& ext : extensions)
        cout << '\t' << ext.extensionName << endl;
    }

    if (mDebug) {
      auto debugInfo = createDebugInfo();
      createInfo.pNext = &debugInfo;
    }

    if (vkCreateInstance(&createInfo, nullptr, &mVulkan) != VK_SUCCESS) {
      throw std::runtime_error{"Unable to create the Vulkan instance"};
    }
  }

  void initDevice() {
    auto devices = enumerateDevices();

    if (devices.empty()) {
      throw std::runtime_error{"No Vulkan device found!"};
    }

    sort(begin(devices), end(devices), [this](auto device1, auto device2) {
      return deviceScore(device1) > deviceScore(device2);
    });

    mPhisicalDevice = *begin(devices);

    if (mDebug) {
      for (const auto& device : devices) {
        auto choosen = false;
        auto score = deviceScore(device);
        if (device == mPhisicalDevice && score != 0)
          choosen = true;

        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);
        cout << (choosen ? "*" : " ") << "Vulkan device [" << properties.deviceType
             << "]: " << properties.deviceName << " score " << score << endl;
      }
    }

    if (deviceScore(mPhisicalDevice) == 0) {
      throw runtime_error{"No suitable GPU device found"};
    }

    auto queues = enumerateQueueFamilyProperties(mPhisicalDevice);
    auto gpuQueue = findGPUQueue(begin(queues), end(queues));

    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = distance(begin(queues), gpuQueue);
    queueCreateInfo.queueCount = 1;
    const auto queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;
  }

  VkDebugUtilsMessengerCreateInfoEXT createDebugInfo() {
    VkDebugUtilsMessengerCreateInfoEXT debugInfo = {};
    debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        //                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugInfo.pfnUserCallback = debugCallback;
    debugInfo.pUserData = nullptr;
    return debugInfo;
  }

  void setupDebugMessenger() {
    auto debugInfo = createDebugInfo();
    if (vkCreateDebugUtilsMessengerEXT(mVulkan, &debugInfo, nullptr, &mDebugMessenger) !=
        VK_SUCCESS) {
      throw std::runtime_error{"Unable to initialize debug"};
    }
  }

  std::vector<const char*> getWindowsRequiredExtensions() {
    uint32_t count;
    auto glfwRequiredextensions = glfwGetRequiredInstanceExtensions(&count);

    std::vector<const char*> extensions{glfwRequiredextensions, glfwRequiredextensions + count};

    if (isValidationLayerEnabled())
      extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    return extensions;
  }

  std::vector<VkExtensionProperties> vulkanExtensionProperties() {
    std::vector<VkExtensionProperties> res;

    uint32_t count;
    vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);

    std::vector<VkExtensionProperties> extensions(count);
    vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data());

    return extensions;
  }

  std::vector<VkPhysicalDevice> enumerateDevices() {
    uint32_t count;
    vkEnumeratePhysicalDevices(mVulkan, &count, nullptr);
    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(mVulkan, &count, devices.data());
    return devices;
  }

  template <typename FwIter> FwIter findGPUQueue(FwIter first, FwIter last) {
    return find_if(first, last, [](const auto& family) {
      return (family.queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0;
    });
  }

  int deviceScore(VkPhysicalDevice device) {
    VkPhysicalDeviceFeatures feature;
    vkGetPhysicalDeviceFeatures(device, &feature);

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(device, &properties);

    int score = 0;

    if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
      score += 1000;
    if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
      score += 1000;

    auto families = enumerateQueueFamilyProperties(device);
    if (auto it = findGPUQueue(begin(families), end(families)); it == end(families)) {
      return 0;
    }

    if (feature.tessellationShader)
      score += 1000;

    return score;
  }

  std::vector<VkQueueFamilyProperties> enumerateQueueFamilyProperties(VkPhysicalDevice device) {
    uint32_t count;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
    std::vector<VkQueueFamilyProperties> queues(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, queues.data());
    return queues;
  }

  std::vector<VkLayerProperties> enumerateVulkanLayers() const {
    uint32_t count;
    vkEnumerateInstanceLayerProperties(&count, nullptr);

    std::vector<VkLayerProperties> layers(count);
    vkEnumerateInstanceLayerProperties(&count, layers.data());

    return layers;
  }

  bool isLayerSupport(const char* requestedLayer) const {
    auto layers = enumerateVulkanLayers();
    auto validationLayer = find_if(begin(layers), end(layers), [requestedLayer](const auto& layer) {
      return strcmp(requestedLayer, layer.layerName);
    });
    return validationLayer != end(layers);
  }

  bool isValidationLayerEnabled() {
    const char* VALIDATION_LAYER = "VK_LAYER_KHRONOS_validation";
    return isLayerSupport(VALIDATION_LAYER);
  }

private:
  static VKAPI_ATTR VkBool32 debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT /*severity*/,
                                           VkDebugUtilsMessageTypeFlagsEXT /*type*/,
                                           const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
                                           void* /*pData*/) {
    cerr << callbackData->pMessage << endl;
    return VK_FALSE;
  }

private:
  const char* mTitle;
  const int mWidth;
  const int mHeight;
  const bool mDebug;
  GLFWwindow* mWindow = nullptr;
  VkInstance mVulkan = nullptr;
  VkDebugUtilsMessengerEXT mDebugMessenger;
  VkPhysicalDevice mPhisicalDevice;
  VkDevice mDevice;
};

int main() {
  try {
    Template window{"template", 800, 600, true};
    return window.run();
  } catch (const std::exception& exc) {
    cerr << exc.what() << endl;
  }

  return 1;
}
}
}
