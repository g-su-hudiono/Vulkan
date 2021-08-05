#include "app.h"

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                    void* pUserData) {
    std::cerr << " \nValidation layer: \n" << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

std::vector<VkSurfaceFormatKHR> GetSurfaceFormatKHR( VkPhysicalDevice physicalDevice, VkSurfaceKHR surface ) {
    uint32_t count;
    vkGetPhysicalDeviceSurfaceFormatsKHR( physicalDevice, surface, &count, nullptr );
    std::vector<VkSurfaceFormatKHR> formats( count );
    vkGetPhysicalDeviceSurfaceFormatsKHR( physicalDevice, surface, &count, formats.data() );
    return formats;
}

std::vector<VkPresentModeKHR> GetSurfaceModeKHR( VkPhysicalDevice physicalDevice, VkSurfaceKHR surface ) {
    uint32_t count;
    vkGetPhysicalDeviceSurfacePresentModesKHR( physicalDevice, surface, &count, nullptr );
    std::vector<VkPresentModeKHR> modes( count );
    vkGetPhysicalDeviceSurfacePresentModesKHR( physicalDevice, surface, &count, modes.data() );
    return modes;
}

std::vector<VkQueueFamilyProperties> GetQueueFamilyProperties( VkPhysicalDevice physicalDevice ) {
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties( physicalDevice, &count, nullptr );
    std::vector<VkQueueFamilyProperties> queueFamilies( count );
    vkGetPhysicalDeviceQueueFamilyProperties( physicalDevice, &count, queueFamilies.data() );
    return queueFamilies;
}

int FindGraphicQueueIndex( VkPhysicalDevice physicalDevice ) {
    std::vector<VkQueueFamilyProperties> queueFamilies = GetQueueFamilyProperties( physicalDevice );
    for ( int i = 0; i < queueFamilies.size(); i++ ) {
        if ( queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT ) return i;
    }
    return -1;
}

int FindPresentQueueIndex( VkPhysicalDevice physicalDevice, VkSurfaceKHR surface ) {
    std::vector<VkQueueFamilyProperties> queueFamilies = GetQueueFamilyProperties( physicalDevice );
    for ( int i = 0; i < queueFamilies.size(); i++ ) {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR( physicalDevice, i, surface, &presentSupport );
        if ( presentSupport ) return i;
    }
    return -1;
}

bool CheckLayerSupport( std::vector<const char*> layers ) {
    uint32_t count;
    vkEnumerateInstanceLayerProperties( &count, nullptr );
    std::vector<VkLayerProperties> availableLayers( count );
    vkEnumerateInstanceLayerProperties( &count, availableLayers.data() );

    std::set<std::string> requiredLayers( layers.begin(), layers.end() );
    for ( const auto& layerProperties : availableLayers ) {
        requiredLayers.erase( layerProperties.layerName );
    }
    return requiredLayers.empty();
}

bool CheckDeviceExtensionSupport( VkPhysicalDevice physicalDevice, std::vector<const char*> extensions ) {
    uint32_t count;
    vkEnumerateDeviceExtensionProperties( physicalDevice, nullptr, &count, nullptr );
    std::vector<VkExtensionProperties> availableExtensions( count );
    vkEnumerateDeviceExtensionProperties( physicalDevice, nullptr, &count, availableExtensions.data() );

    std::set<std::string> requiredExtensions( extensions.begin(), extensions.end() );
    for ( const auto& extension : availableExtensions ) {
        requiredExtensions.erase( extension.extensionName );
    }
    return requiredExtensions.empty();
}

VkResult CreateDebugUtilsMessengerEXT( VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger ) {
    auto func = ( PFN_vkCreateDebugUtilsMessengerEXT )vkGetInstanceProcAddr( instance, "vkCreateDebugUtilsMessengerEXT" );
    if ( func != nullptr ) {
        return func( instance, pCreateInfo, pAllocator, pDebugMessenger );
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT( VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator ) {
    auto func = ( PFN_vkDestroyDebugUtilsMessengerEXT )vkGetInstanceProcAddr( instance, "vkDestroyDebugUtilsMessengerEXT" );
    if ( func != nullptr ) { func( instance, debugMessenger, pAllocator ); }
}


// ============================================================

void App::cleanupDevice() {
    vkDestroyDevice( m_device, nullptr );
    DestroyDebugUtilsMessengerEXT( m_instance, m_debugMessenger, nullptr );
    vkDestroyInstance( m_instance, nullptr );
}

void App::createInstance() {
    LOG("createInstance");
    VkDebugUtilsMessengerCreateInfoEXT debugInfo{};
    debugInfo.sType  = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugInfo.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT    |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugInfo.pfnUserCallback = DebugCallback;

    deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME
    };
    validationLayers = { "VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_monitor" };

    VkApplicationInfo appInfo{};
    appInfo.sType               = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName    = "Application";
    appInfo.applicationVersion  = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName         = "No Engine";
    appInfo.engineVersion       = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion          = VK_API_VERSION_1_2;


    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );
    std::vector<const char*> extensions( glfwExtensions, glfwExtensions + glfwExtensionCount );
    extensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );

    VkInstanceCreateInfo instanceInfo{};
    instanceInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo        = &appInfo;
    instanceInfo.enabledExtensionCount   = UINT32(extensions.size());
    instanceInfo.ppEnabledExtensionNames = extensions.data();
    instanceInfo.enabledLayerCount       = UINT32(validationLayers.size());
    instanceInfo.ppEnabledLayerNames     = validationLayers.data();
    instanceInfo.pNext                   = &debugInfo;
 
    VkResult result = vkCreateInstance( &instanceInfo, nullptr, &m_instance );
    CHECK_VKRESULT(result, "failed to create vulkan instance!");
    
    result = CreateDebugUtilsMessengerEXT( m_instance, &debugInfo, nullptr, &m_debugMessenger );
    CHECK_VKRESULT( result, "failed to set up debug messenger!" );

    result = glfwCreateWindowSurface( m_instance, m_window, nullptr, &m_surface );
    CHECK_VKRESULT( result, "failed to create surface!" );
}

void App::pickPhysicalDevice() {
    uint32_t count;
    vkEnumeratePhysicalDevices( m_instance, &count, nullptr );
    std::vector<VkPhysicalDevice> physicalDevices( count );
    vkEnumeratePhysicalDevices( m_instance, &count, physicalDevices.data() );
    
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    std::vector<VkSurfaceFormatKHR> surfaceFormats;
    std::vector<VkPresentModeKHR>   presentModes;
    int graphicQueueIndex = 0;
    int presentQueueIndex = 0;

    for ( const auto& tempDevice : physicalDevices ) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties( tempDevice, &properties );
        LOG( properties.deviceName );

        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures( tempDevice, &supportedFeatures );

        physicalDevice = tempDevice;
        surfaceFormats    = GetSurfaceFormatKHR( tempDevice, m_surface );
        presentModes      = GetSurfaceModeKHR( tempDevice, m_surface );
        presentQueueIndex = FindPresentQueueIndex( tempDevice, m_surface );
        graphicQueueIndex = FindGraphicQueueIndex( tempDevice );

        bool swapchainAdequate  = !surfaceFormats.empty() && !presentModes.empty();
        bool hasFamilyIndex     = graphicQueueIndex > -1 && presentQueueIndex > -1;
        bool extensionSupported = CheckDeviceExtensionSupport( tempDevice, deviceExtensions );

        if ( swapchainAdequate && hasFamilyIndex && extensionSupported &&
            supportedFeatures.samplerAnisotropy ) break;
    }
    m_physicalDevice = physicalDevice;
    m_surfaceFormats = surfaceFormats;
    m_presentModes   = presentModes;
    m_graphicQueueIndex = graphicQueueIndex;
    m_presentQueueIndex = presentQueueIndex;
}

void App::createLogicalDevice() {
    std::set<uint32_t> queueFamilyIndices = { m_graphicQueueIndex, m_presentQueueIndex };

    float queuePriority = 1.f;
    std::vector<VkDeviceQueueCreateInfo> queueInfos;
    for ( uint32_t familyIndex : queueFamilyIndices ) {
        VkDeviceQueueCreateInfo queueInfo{};
        queueInfo.sType             = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueFamilyIndex  = familyIndex;
        queueInfo.queueCount        = 1;
        queueInfo.pQueuePriorities  = &queuePriority;
        queueInfos.push_back(queueInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    deviceFeatures.multiViewport     = VK_TRUE;

    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.queueCreateInfoCount     = UINT32(queueInfos.size());
    deviceInfo.pQueueCreateInfos        = queueInfos.data();
    deviceInfo.pEnabledFeatures         = &deviceFeatures;
    deviceInfo.enabledExtensionCount    = UINT32(deviceExtensions.size());
    deviceInfo.ppEnabledExtensionNames  = deviceExtensions.data();
    deviceInfo.enabledLayerCount        = UINT32(validationLayers.size());
    deviceInfo.ppEnabledLayerNames      = validationLayers.data();

    VkResult result = vkCreateDevice( m_physicalDevice, &deviceInfo, nullptr, &m_device );
    CHECK_VKRESULT( result, "failed to create logical device" );

    vkGetDeviceQueue( m_device, m_graphicQueueIndex, 0, &m_graphicQueue );
    vkGetDeviceQueue( m_device, m_presentQueueIndex, 0, &m_presentQueue );
}


VkSurfaceFormatKHR App::getSwapchainSurfaceFormat() {
    const std::vector<VkSurfaceFormatKHR>& availableFormats = m_surfaceFormats;
    for ( const auto& availableFormat : availableFormats ) {
        if ( availableFormat.format == VK_FORMAT_B8G8R8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR ) {
            return availableFormat;
        }
    }
    return availableFormats[0];
}

VkPresentModeKHR App::getSwapchainPresentMode() {
    const std::vector<VkPresentModeKHR>& availablePresentModes = m_presentModes;
    for ( const auto& availablePresentMode : availablePresentModes ) {
        if ( availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR )
            return availablePresentMode;
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}