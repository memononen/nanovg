
#pragma once

typedef struct VulkanDevice {
  VkPhysicalDevice gpu;
  VkPhysicalDeviceProperties gpuProperties;
  VkPhysicalDeviceMemoryProperties memoryProperties;

  VkQueueFamilyProperties *queueFamilyProperties;
  uint32_t queueFamilyPropertiesCount;

  uint32_t graphicsQueueFamilyIndex;

  VkDevice device;

  VkCommandPool commandPool;
} VulkanDevice;

VulkanDevice *createVulkanDevice(VkPhysicalDevice gpu) {
  VulkanDevice *device = (VulkanDevice *)malloc(sizeof(VulkanDevice));
  memset(device, 0, sizeof(VulkanDevice));

  device->gpu = gpu;
  vkGetPhysicalDeviceMemoryProperties(gpu, &device->memoryProperties);
  vkGetPhysicalDeviceProperties(gpu, &device->gpuProperties);

  vkGetPhysicalDeviceQueueFamilyProperties(gpu, &device->queueFamilyPropertiesCount, NULL);
  assert(device->queueFamilyPropertiesCount >= 1);

  device->queueFamilyProperties = (VkQueueFamilyProperties *)malloc(device->queueFamilyPropertiesCount * sizeof(VkQueueFamilyProperties));

  vkGetPhysicalDeviceQueueFamilyProperties(gpu, &device->queueFamilyPropertiesCount, device->queueFamilyProperties);
  assert(device->queueFamilyPropertiesCount >= 1);

  device->graphicsQueueFamilyIndex = UINT32_MAX;
  for (uint32_t i = 0; i < device->queueFamilyPropertiesCount; ++i) {
    if ((device->queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
      device->graphicsQueueFamilyIndex = i;
    }
  }

  float queuePriorities[1] = {0.0};
  VkDeviceQueueCreateInfo queue_info = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
  queue_info.queueCount = 1;
  queue_info.pQueuePriorities = queuePriorities;
  queue_info.queueFamilyIndex = device->graphicsQueueFamilyIndex;

  const char *deviceExtensions[] = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
  };
  VkDeviceCreateInfo deviceInfo = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
  deviceInfo.queueCreateInfoCount = 1;
  deviceInfo.pQueueCreateInfos = &queue_info;
  deviceInfo.enabledExtensionCount = sizeof(deviceExtensions) / sizeof(deviceExtensions[0]);
  deviceInfo.ppEnabledExtensionNames = deviceExtensions;
  VkResult res = vkCreateDevice(gpu, &deviceInfo, NULL, &device->device);

  assert(res == VK_SUCCESS);

  /* Create a command pool to allocate our command buffer from */
  VkCommandPoolCreateInfo cmd_pool_info = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  cmd_pool_info.queueFamilyIndex = device->graphicsQueueFamilyIndex;
  cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  res = vkCreateCommandPool(device->device, &cmd_pool_info, NULL, &device->commandPool);
  assert(res == VK_SUCCESS);

  return device;
}
void destroyVulkanDevice(VulkanDevice *device) {
  free(device->queueFamilyProperties);

  if (device->commandPool) {
    vkDestroyCommandPool(device->device, device->commandPool, 0);
  }
  if (device->device) {
    vkDestroyDevice(device->device, 0);
  }
  free(device);
}

typedef struct SwapchainBuffers {
  VkImage image;
  VkImageView view;
} SwapchainBuffers;

typedef struct DepthBuffer {
  VkFormat format;

  VkImage image;
  VkDeviceMemory mem;
  VkImageView view;
} DepthBuffer;

typedef struct FrameBuffers {
  VkSwapchainKHR swap_chain;
  SwapchainBuffers *swap_chain_buffers;
  uint32_t swapchain_image_count;
  VkFramebuffer *framebuffers;

  uint32_t current_buffer;

  VkExtent2D buffer_size;

  VkRenderPass render_pass;

  VkFormat format;
  DepthBuffer depth;
  VkSemaphore present_complete_semaphore;
  VkSemaphore render_complete_semaphore;

} FrameBuffers;

static VkInstance createVkInstance(bool enable_debug_layer) {

  // initialize the VkApplicationInfo structure
  VkApplicationInfo app_info = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
  app_info.pApplicationName = "NanoVG";
  app_info.applicationVersion = 1;
  app_info.pEngineName = "NanoVG";
  app_info.engineVersion = 1;
  app_info.apiVersion = VK_API_VERSION_1_0;

  static const char *append_extensions[] = {
      VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
  };
  uint32_t append_extensions_count = sizeof(append_extensions) / sizeof(append_extensions[0]);
  if (!enable_debug_layer) {
    append_extensions_count = 0;
  }

  uint32_t extensions_count = 0;
  const char **glfw_extensions = glfwGetRequiredInstanceExtensions(&extensions_count);

  const char **extensions = (const char **)calloc(extensions_count + append_extensions_count, sizeof(char *));

  for (int i = 0; i < extensions_count; ++i) {
    extensions[i] = glfw_extensions[i];
  }
  for (int i = 0; i < append_extensions_count; ++i) {
    extensions[extensions_count++] = append_extensions[i];
  }

  // initialize the VkInstanceCreateInfo structure
  VkInstanceCreateInfo inst_info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
  inst_info.pApplicationInfo = &app_info;
  inst_info.enabledExtensionCount = extensions_count;
  inst_info.ppEnabledExtensionNames = extensions;
  if (enable_debug_layer) {
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, 0);
    VkLayerProperties *layerprop = (VkLayerProperties *)malloc(sizeof(VkLayerProperties) * layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, layerprop);
    printf("supported layers:");
    for (int i = 0; i < layerCount; ++i) {
      printf("%s ,", layerprop[i].layerName);
    }
    printf("\n");
    free(layerprop);

    static const char *instance_validation_layers[] = {
        "VK_LAYER_LUNARG_standard_validation"
        //      "VK_LAYER_GOOGLE_threading",
        //      "VK_LAYER_GOOGLE_unique_objects",
        //      "VK_LAYER_LUNARG_api_dump",
        //     "VK_LAYER_LUNARG_device_limits",
        //      "VK_LAYER_LUNARG_draw_state",
        //   "VK_LAYER_LUNARG_image",
        //  "VK_LAYER_LUNARG_mem_tracker",
        // "VK_LAYER_LUNARG_object_tracker",
        //   "VK_LAYER_LUNARG_param_checker",
        //  "VK_LAYER_LUNARG_screenshot",
        //  "VK_LAYER_LUNARG_swapchain",
    };
    inst_info.enabledLayerCount = sizeof(instance_validation_layers) / sizeof(instance_validation_layers[0]);
    inst_info.ppEnabledLayerNames = instance_validation_layers;
  }
  VkInstance inst;
  VkResult res;
  res = vkCreateInstance(&inst_info, NULL, &inst);

  free(extensions);

  if (res == VK_ERROR_INCOMPATIBLE_DRIVER) {
    printf("cannot find a compatible Vulkan ICD\n");
    exit(-1);
  } else if (res) {
    printf("unknown error\n");
    exit(-1);
  }
  return inst;
}

PFN_vkCreateDebugReportCallbackEXT _vkCreateDebugReportCallbackEXT;
PFN_vkDebugReportMessageEXT _vkDebugReportMessageEXT;
PFN_vkDestroyDebugReportCallbackEXT _vkDestroyDebugReportCallbackEXT;

VkDebugReportCallbackEXT debug_report_callback;
VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT report_object_type, uint64_t object,
                                             size_t location, int32_t messageCode, const char *pLayerPrefix, const char *pMessage, void *pUserData) {

  printf("%s\n", pMessage);
  return VK_FALSE;
}
static VkDebugReportCallbackEXT CreateDebugReport(VkInstance instance) {
  // load extensions
  _vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)(vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT"));
  _vkDebugReportMessageEXT = (PFN_vkDebugReportMessageEXT)(vkGetInstanceProcAddr(instance, "vkDebugReportMessageEXT"));
  _vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)(vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT"));
  VkDebugReportCallbackCreateInfoEXT callbackInfo = {VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT};
  callbackInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
  callbackInfo.pfnCallback = &debugCallback;
  VkDebugReportCallbackEXT debug_report_callback;
  _vkCreateDebugReportCallbackEXT(instance, &callbackInfo, 0, &debug_report_callback);
  return debug_report_callback;
}
static void DestroyDebugReport(VkInstance instance, VkDebugReportCallbackEXT debug_report_callback) {
  if (_vkDestroyDebugReportCallbackEXT) {
    _vkDestroyDebugReportCallbackEXT(instance, debug_report_callback, 0);
  }
}

VkCommandPool createCmdPool(VulkanDevice *device) {
  VkResult res;
  /* Create a command pool to allocate our command buffer from */
  VkCommandPoolCreateInfo cmd_pool_info = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  cmd_pool_info.queueFamilyIndex = device->graphicsQueueFamilyIndex;
  cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  VkCommandPool cmd_pool;
  res = vkCreateCommandPool(device->device, &cmd_pool_info, NULL, &cmd_pool);
  assert(res == VK_SUCCESS);
  return cmd_pool;
}
VkCommandBuffer createCmdBuffer(VkDevice device, VkCommandPool cmd_pool) {

  VkResult res;
  VkCommandBufferAllocateInfo cmd = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  cmd.commandPool = cmd_pool;
  cmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  cmd.commandBufferCount = 1;

  VkCommandBuffer cmd_buffer;
  res = vkAllocateCommandBuffers(device, &cmd, &cmd_buffer);
  assert(res == VK_SUCCESS);
  return cmd_buffer;
}

bool memory_type_from_properties(VkPhysicalDeviceMemoryProperties memoryProps, uint32_t typeBits, VkFlags requirements_mask, uint32_t *typeIndex) {
  // Search memtypes to find first index with those properties
  for (uint32_t i = 0; i < memoryProps.memoryTypeCount; i++) {
    if ((typeBits & 1) == 1) {
      // Type is available, does it match user properties?
      if ((memoryProps.memoryTypes[i].propertyFlags & requirements_mask) == requirements_mask) {
        *typeIndex = i;
        return true;
      }
    }
    typeBits >>= 1;
  }
  // No memory types matched, return failure
  return false;
}
DepthBuffer createDepthBuffer(const VulkanDevice *device, int width, int height) {
  VkResult res;
  DepthBuffer depth;
  depth.format = VK_FORMAT_D24_UNORM_S8_UINT;

  const VkFormat depth_format = depth.format;
  VkFormatProperties fprops;
  vkGetPhysicalDeviceFormatProperties(device->gpu, depth_format, &fprops);
  VkImageTiling image_tilling;
  if (fprops.linearTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
    image_tilling = VK_IMAGE_TILING_LINEAR;
  } else if (fprops.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
    image_tilling = VK_IMAGE_TILING_OPTIMAL;
  } else {
    /* Try other depth formats? */
    printf("depth_format %d Unsupported.\n", depth_format);
    exit(-1);
  }

  VkImageCreateInfo image_info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  image_info.imageType = VK_IMAGE_TYPE_2D;
  image_info.format = depth_format;
  image_info.tiling = image_tilling;
  image_info.extent.width = width;
  image_info.extent.height = height;
  image_info.extent.depth = 1;
  image_info.mipLevels = 1;
  image_info.arrayLayers = 1;
  image_info.samples = VK_SAMPLE_COUNT_1_BIT;
  image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  image_info.queueFamilyIndexCount = 0;
  image_info.pQueueFamilyIndices = NULL;
  image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  VkMemoryAllocateInfo mem_alloc = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};

  VkImageViewCreateInfo view_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
  view_info.format = depth_format;
  view_info.components.r = VK_COMPONENT_SWIZZLE_R;
  view_info.components.g = VK_COMPONENT_SWIZZLE_G;
  view_info.components.b = VK_COMPONENT_SWIZZLE_B;
  view_info.components.a = VK_COMPONENT_SWIZZLE_A;
  view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  view_info.subresourceRange.baseMipLevel = 0;
  view_info.subresourceRange.levelCount = 1;
  view_info.subresourceRange.baseArrayLayer = 0;
  view_info.subresourceRange.layerCount = 1;
  view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;

  if (depth_format == VK_FORMAT_D16_UNORM_S8_UINT || depth_format == VK_FORMAT_D24_UNORM_S8_UINT ||
      depth_format == VK_FORMAT_D32_SFLOAT_S8_UINT) {
    view_info.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
  }

  VkMemoryRequirements mem_reqs;

  /* Create image */
  res = vkCreateImage(device->device, &image_info, NULL, &depth.image);
  assert(res == VK_SUCCESS);

  vkGetImageMemoryRequirements(device->device, depth.image, &mem_reqs);

  mem_alloc.allocationSize = mem_reqs.size;
  /* Use the memory properties to determine the type of memory required */

  bool pass =
      memory_type_from_properties(device->memoryProperties, mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &mem_alloc.memoryTypeIndex);
  assert(pass);

  /* Allocate memory */
  res = vkAllocateMemory(device->device, &mem_alloc, NULL, &depth.mem);
  assert(res == VK_SUCCESS);

  /* Bind memory */
  res = vkBindImageMemory(device->device, depth.image, depth.mem, 0);
  assert(res == VK_SUCCESS);

  /* Create image view */
  view_info.image = depth.image;
  res = vkCreateImageView(device->device, &view_info, NULL, &depth.view);
  assert(res == VK_SUCCESS);

  return depth;
}

static void setupImageLayout(VkCommandBuffer cmdbuffer, VkImage image,
                             VkImageAspectFlags aspectMask,
                             VkImageLayout old_image_layout,
                             VkImageLayout new_image_layout) {

  VkImageMemoryBarrier image_memory_barrier = {VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
  image_memory_barrier.oldLayout = old_image_layout;
  image_memory_barrier.newLayout = new_image_layout;
  image_memory_barrier.image = image;

  VkImageSubresourceRange subresourceRange = {aspectMask, 0, 1, 0, 1};
  image_memory_barrier.subresourceRange = subresourceRange;

  if (new_image_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    /* Make sure anything that was copying from this image has completed */
    image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  }

  if (new_image_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
    image_memory_barrier.dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  }

  if (new_image_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    image_memory_barrier.dstAccessMask =
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  }

  if (new_image_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    /* Make sure any Copy or CPU writes to image are flushed */
    image_memory_barrier.dstAccessMask =
        VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
  }

  VkImageMemoryBarrier *pmemory_barrier = &image_memory_barrier;

  VkPipelineStageFlags src_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  VkPipelineStageFlags dest_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

  vkCmdPipelineBarrier(cmdbuffer, src_stages, dest_stages, 0, 0, NULL,
                       0, NULL, 1, pmemory_barrier);
}

SwapchainBuffers createSwapchainBuffers(const VulkanDevice *device, VkFormat format, VkCommandBuffer cmdbuffer, VkImage image) {

  VkResult res;
  SwapchainBuffers buffer;
  VkImageViewCreateInfo color_attachment_view = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
  color_attachment_view.format = format;
  color_attachment_view.components.r = VK_COMPONENT_SWIZZLE_R;
  color_attachment_view.components.g = VK_COMPONENT_SWIZZLE_G;
  color_attachment_view.components.b = VK_COMPONENT_SWIZZLE_B;
  color_attachment_view.components.a = VK_COMPONENT_SWIZZLE_A;
  VkImageSubresourceRange subresourceRange = {0};
  subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  subresourceRange.baseMipLevel = 0;
  subresourceRange.levelCount = 1;
  subresourceRange.baseArrayLayer = 0;
  subresourceRange.layerCount = 1;

  color_attachment_view.subresourceRange = subresourceRange;
  color_attachment_view.viewType = VK_IMAGE_VIEW_TYPE_2D;

  buffer.image = image;

  setupImageLayout(
      cmdbuffer, image, VK_IMAGE_ASPECT_COLOR_BIT,
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

  color_attachment_view.image = buffer.image;

  res = vkCreateImageView(device->device, &color_attachment_view, NULL,
                          &buffer.view);
  assert(res == VK_SUCCESS);
  return buffer;
}

VkRenderPass createRenderPass(VkDevice device, VkFormat color_format, VkFormat depth_format) {
  VkAttachmentDescription attachments[2] = {{0}};
  attachments[0].format = color_format;
  attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
  attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  attachments[1].format = depth_format;
  attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
  attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference color_reference = {0};
  color_reference.attachment = 0;
  color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depth_reference = {0};
  depth_reference.attachment = 1;
  depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {0};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.flags = 0;
  subpass.inputAttachmentCount = 0;
  subpass.pInputAttachments = NULL;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &color_reference;
  subpass.pResolveAttachments = NULL;
  subpass.pDepthStencilAttachment = &depth_reference;
  subpass.preserveAttachmentCount = 0;
  subpass.pPreserveAttachments = NULL;

  VkRenderPassCreateInfo rp_info = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
  rp_info.attachmentCount = 2;
  rp_info.pAttachments = attachments;
  rp_info.subpassCount = 1;
  rp_info.pSubpasses = &subpass;
  VkRenderPass render_pass;
  VkResult res;
  res = vkCreateRenderPass(device, &rp_info, NULL, &render_pass);
  assert(res == VK_SUCCESS);
  return render_pass;
}
FrameBuffers createFrameBuffers(const VulkanDevice *device, VkSurfaceKHR surface, VkQueue queue, int winWidth, int winHeight, VkSwapchainKHR oldSwapchain) {

  VkResult res;

  VkBool32 supportsPresent;
  vkGetPhysicalDeviceSurfaceSupportKHR(device->gpu, device->graphicsQueueFamilyIndex, surface, &supportsPresent);
  if (!supportsPresent) {
    exit(-1); //does not supported.
  }
  VkCommandBuffer setup_cmd_buffer = createCmdBuffer(device->device, device->commandPool);

  const VkCommandBufferBeginInfo cmd_buf_info = {
      VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
  };
  vkBeginCommandBuffer(setup_cmd_buffer, &cmd_buf_info);

  VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
  VkColorSpaceKHR colorSpace;
  {
    // Get the list of VkFormats that are supported:
    uint32_t formatCount;
    res = vkGetPhysicalDeviceSurfaceFormatsKHR(device->gpu, surface, &formatCount, NULL);
    assert(res == VK_SUCCESS);
    VkSurfaceFormatKHR *surfFormats = (VkSurfaceFormatKHR *)malloc(formatCount * sizeof(VkSurfaceFormatKHR));
    res = vkGetPhysicalDeviceSurfaceFormatsKHR(device->gpu, surface, &formatCount, surfFormats);
    assert(res == VK_SUCCESS);
    // If the format list includes just one entry of VK_FORMAT_UNDEFINED,
    // the surface has no preferred format.  Otherwise, at least one
    // supported format will be returned.
    if (formatCount == 1 && surfFormats[0].format == VK_FORMAT_UNDEFINED) {
      colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
    } else {
      assert(formatCount >= 1);
      colorFormat = surfFormats[0].format;
    }
    colorSpace = surfFormats[0].colorSpace;
    free(surfFormats);
  }

  // Check the surface capabilities and formats
  VkSurfaceCapabilitiesKHR surfCapabilities;
  res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
      device->gpu, surface, &surfCapabilities);
  assert(res == VK_SUCCESS);

  VkExtent2D buffer_size;
  // width and height are either both -1, or both not -1.
  if (surfCapabilities.currentExtent.width == (uint32_t)-1) {
    buffer_size.width = winWidth;
    buffer_size.width = winHeight;
  } else {
    // If the surface size is defined, the swap chain size must match
    buffer_size = surfCapabilities.currentExtent;
  }

  DepthBuffer depth = createDepthBuffer(device, buffer_size.width, buffer_size.height);

  VkRenderPass render_pass = createRenderPass(device->device, colorFormat, depth.format);

  VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device->gpu, surface, &presentModeCount, NULL);
  assert(presentModeCount > 0);

  VkPresentModeKHR *presentModes = (VkPresentModeKHR *)malloc(sizeof(VkPresentModeKHR) * presentModeCount);
  vkGetPhysicalDeviceSurfacePresentModesKHR(device->gpu, surface, &presentModeCount, presentModes);

  for (size_t i = 0; i < presentModeCount; i++) {
    if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
      swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
      break;
    }
    if ((swapchainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR) && (presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)) {
      swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    }
  }
  free(presentModes);

  VkSurfaceTransformFlagBitsKHR preTransform;
  if (surfCapabilities.supportedTransforms &
      VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
    preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  } else {
    preTransform = surfCapabilities.currentTransform;
  }

  // Determine the number of VkImage's to use in the swap chain (we desire to
  // own only 1 image at a time, besides the images being displayed and
  // queued for display):
  uint32_t desiredNumberOfSwapchainImages =
      surfCapabilities.minImageCount + 1;
  if ((surfCapabilities.maxImageCount > 0) &&
      (desiredNumberOfSwapchainImages > surfCapabilities.maxImageCount)) {
    // Application must settle for fewer images than desired:
    desiredNumberOfSwapchainImages = surfCapabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR swapchainInfo = {VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
  swapchainInfo.surface = surface;
  swapchainInfo.minImageCount = desiredNumberOfSwapchainImages;
  swapchainInfo.imageFormat = colorFormat;
  swapchainInfo.imageColorSpace = colorSpace;
  swapchainInfo.imageExtent = buffer_size;
  swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  swapchainInfo.preTransform = preTransform;
  swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  swapchainInfo.imageArrayLayers = 1;
  swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  swapchainInfo.presentMode = swapchainPresentMode;
  swapchainInfo.oldSwapchain = oldSwapchain;
  swapchainInfo.clipped = true;

  VkSwapchainKHR swap_chain;
  res = vkCreateSwapchainKHR(device->device, &swapchainInfo, NULL,
                             &swap_chain);
  assert(res == VK_SUCCESS);

  if (oldSwapchain != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(device->device, oldSwapchain, NULL);
  }

  uint32_t swapchain_image_count;
  res = vkGetSwapchainImagesKHR(device->device, swap_chain,
                                &swapchain_image_count, NULL);
  assert(res == VK_SUCCESS);

  VkImage *swapchainImages =
      (VkImage *)malloc(swapchain_image_count * sizeof(VkImage));

  assert(swapchainImages);

  res = vkGetSwapchainImagesKHR(device->device, swap_chain,
                                &swapchain_image_count,
                                swapchainImages);
  assert(res == VK_SUCCESS);

  SwapchainBuffers *swap_chain_buffers = (SwapchainBuffers *)malloc(swapchain_image_count * sizeof(SwapchainBuffers));
  for (uint32_t i = 0; i < swapchain_image_count; i++) {
    swap_chain_buffers[i] = createSwapchainBuffers(device, colorFormat, setup_cmd_buffer, swapchainImages[i]);
  }
  free(swapchainImages);

  VkImageView attachments[2];
  attachments[1] = depth.view;

  VkFramebufferCreateInfo fb_info = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
  fb_info.renderPass = render_pass;
  fb_info.attachmentCount = 2;
  fb_info.pAttachments = attachments;
  fb_info.width = buffer_size.width;
  fb_info.height = buffer_size.height;
  fb_info.layers = 1;
  uint32_t i;

  VkFramebuffer *framebuffers = (VkFramebuffer *)malloc(swapchain_image_count *
                                                        sizeof(VkFramebuffer));
  assert(framebuffers);

  for (i = 0; i < swapchain_image_count; i++) {
    attachments[0] = swap_chain_buffers[i].view;
    res = vkCreateFramebuffer(device->device, &fb_info, NULL,
                              &framebuffers[i]);
    assert(res == VK_SUCCESS);
  }

  vkEndCommandBuffer(setup_cmd_buffer);
  VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &setup_cmd_buffer;

  vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(queue);

  vkFreeCommandBuffers(device->device, device->commandPool, 1, &setup_cmd_buffer);

  FrameBuffers buffer = {0};
  buffer.swap_chain = swap_chain;
  buffer.swap_chain_buffers = swap_chain_buffers;
  buffer.swapchain_image_count = swapchain_image_count;
  buffer.framebuffers = framebuffers;
  buffer.current_buffer = 0;
  buffer.format = colorFormat;
  buffer.buffer_size = buffer_size;
  buffer.render_pass = render_pass;
  buffer.depth = depth;

  VkSemaphoreCreateInfo presentCompleteSemaphoreCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
  res = vkCreateSemaphore(device->device, &presentCompleteSemaphoreCreateInfo, NULL, &buffer.present_complete_semaphore);
  assert(res == VK_SUCCESS);

  res = vkCreateSemaphore(device->device, &presentCompleteSemaphoreCreateInfo, NULL, &buffer.render_complete_semaphore);

  return buffer;
}
void destroyFrameBuffers(const VulkanDevice *device, FrameBuffers *buffer) {

  if (buffer->present_complete_semaphore != VK_NULL_HANDLE) {
    vkDestroySemaphore(device->device, buffer->present_complete_semaphore, NULL);
  }
  if (buffer->render_complete_semaphore != VK_NULL_HANDLE) {
    vkDestroySemaphore(device->device, buffer->render_complete_semaphore, NULL);
  }

  for (int i = 0; i < buffer->swapchain_image_count; ++i) {
    vkDestroyImageView(device->device, buffer->swap_chain_buffers[i].view, 0);
    vkDestroyFramebuffer(device->device, buffer->framebuffers[i], 0);
  }

  vkDestroyImageView(device->device, buffer->depth.view, 0);
  vkDestroyImage(device->device, buffer->depth.image, 0);
  vkFreeMemory(device->device, buffer->depth.mem, 0);

  vkDestroyRenderPass(device->device, buffer->render_pass, 0);
  vkDestroySwapchainKHR(device->device, buffer->swap_chain, 0);

  free(buffer->framebuffers);
  free(buffer->swap_chain_buffers);
}