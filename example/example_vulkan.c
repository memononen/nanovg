#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.h>

#include "nanovg.h"
#include "nanovg_vk.h"

#include "demo.h"
#include "perf.h"

#include "vulkan_util.h"

void errorcb(int error, const char *desc) {
  printf("GLFW error %d: %s\n", error, desc);
}

int blowup = 0;
int screenshot = 0;
int premult = 0;
bool resize_event = false;

static void key(GLFWwindow *window, int key, int scancode, int action, int mods) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);
  if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    blowup = !blowup;
  if (key == GLFW_KEY_S && action == GLFW_PRESS)
    screenshot = 1;
  if (key == GLFW_KEY_P && action == GLFW_PRESS)
    premult = !premult;
}

void prepareFrame(VkDevice device, VkCommandBuffer cmd_buffer, FrameBuffers *fb) {
  VkResult res;

  // Get the index of the next available swapchain image:
  res = vkAcquireNextImageKHR(device, fb->swap_chain, UINT64_MAX,
                              fb->present_complete_semaphore,
                              0,
                              &fb->current_buffer);
  if (res == VK_ERROR_OUT_OF_DATE_KHR)
  {
      resize_event = true;
      res = 0;
      return;
  }
  assert(res == VK_SUCCESS);

  const VkCommandBufferBeginInfo cmd_buf_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  res = vkBeginCommandBuffer(cmd_buffer, &cmd_buf_info);
  assert(res == VK_SUCCESS);

  VkClearValue clear_values[2];
  clear_values[0].color.float32[0] = 0.3f;
  clear_values[0].color.float32[1] = 0.3f;
  clear_values[0].color.float32[2] = 0.32f;
  clear_values[0].color.float32[3] = 1.0f;
  clear_values[1].depthStencil.depth = 1.0f;
  clear_values[1].depthStencil.stencil = 0;

  VkRenderPassBeginInfo rp_begin;
  rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  rp_begin.pNext = NULL;
  rp_begin.renderPass = fb->render_pass;
  rp_begin.framebuffer = fb->framebuffers[fb->current_buffer];
  rp_begin.renderArea.offset.x = 0;
  rp_begin.renderArea.offset.y = 0;
  rp_begin.renderArea.extent.width = fb->buffer_size.width;
  rp_begin.renderArea.extent.height = fb->buffer_size.height;
  rp_begin.clearValueCount = 2;
  rp_begin.pClearValues = clear_values;

  vkCmdBeginRenderPass(cmd_buffer, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport;
  viewport.width = fb->buffer_size.width;
  viewport.height = fb->buffer_size.height;
  viewport.minDepth = (float)0.0f;
  viewport.maxDepth = (float)1.0f;
  viewport.x = rp_begin.renderArea.offset.x;
  viewport.y = rp_begin.renderArea.offset.y;
  vkCmdSetViewport(cmd_buffer, 0, 1, &viewport);

  VkRect2D scissor = rp_begin.renderArea;
  vkCmdSetScissor(cmd_buffer, 0, 1, &scissor);
}
void submitFrame(VkDevice device, VkQueue queue, VkCommandBuffer cmd_buffer, FrameBuffers *fb) {
  VkResult res;

  vkCmdEndRenderPass(cmd_buffer);
  
  VkImageMemoryBarrier image_barrier = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT,
      .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = fb->swap_chain_buffers[fb->current_buffer].image,
      .subresourceRange = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .baseMipLevel = 0,
          .levelCount = 1,
          .baseArrayLayer = 0,
          .layerCount = 1,
      },
  };
  vkCmdPipelineBarrier(cmd_buffer,
            VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            0,
            0, NULL,
            0, NULL,
            1, &image_barrier);
  
  vkEndCommandBuffer(cmd_buffer);

  VkPipelineStageFlags pipe_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

  VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submit_info.pNext = NULL;
  submit_info.waitSemaphoreCount = 1;
  submit_info.pWaitSemaphores = &fb->present_complete_semaphore;
  submit_info.pWaitDstStageMask = &pipe_stage_flags;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &cmd_buffer;
  submit_info.signalSemaphoreCount = 1;
  submit_info.pSignalSemaphores = &fb->render_complete_semaphore;

  /* Queue the command buffer for execution */
  res = vkQueueSubmit(queue, 1, &submit_info, 0);
  assert(res == VK_SUCCESS);

  /* Now present the image in the window */

  VkPresentInfoKHR present = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
  present.pNext = NULL;
  present.swapchainCount = 1;
  present.pSwapchains = &fb->swap_chain;
  present.pImageIndices = &fb->current_buffer;
  present.waitSemaphoreCount = 1;
  present.pWaitSemaphores = &fb->render_complete_semaphore;

  res = vkQueuePresentKHR(queue, &present);
  if (res == VK_ERROR_OUT_OF_DATE_KHR)
  {
      res = vkQueueWaitIdle(queue);
      resize_event = true;
      res = 0;
      return;
  }
  assert(res == VK_SUCCESS);

  res = vkQueueWaitIdle(queue);
}

int main() {
  GLFWwindow *window;

  if (!glfwInit()) {
    printf("Failed to init GLFW.");
    return -1;
  }

  if (!glfwVulkanSupported()) {
    printf("vulkan dose not supported\n");
    return 1;
  }

  glfwSetErrorCallback(errorcb);

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  window = glfwCreateWindow(1000, 600, "NanoVG Example Vulkan", NULL, NULL);
  if (!window) {
    glfwTerminate();
    return -1;
  }

  glfwSetKeyCallback(window, key);

  glfwSetTime(0);

  bool enableValidationLayer = false;
#ifndef NDEBUG
  enableValidationLayer = true;
#endif

  VkInstance instance = createVkInstance(enableValidationLayer);

  VkResult res;
  VkSurfaceKHR surface;
  res = glfwCreateWindowSurface(instance, window, 0, &surface);
  if (VK_SUCCESS != res) {
    printf("glfwCreateWindowSurface failed\n");
    exit(-1);
  }
  
  uint32_t gpu_count = 0;

  res = vkEnumeratePhysicalDevices(instance, &gpu_count, NULL);
  if (VK_SUCCESS != res && res != VK_INCOMPLETE) {
    printf("vkEnumeratePhysicalDevices failed %d \n", res);
    exit(-1);
  }
  if (gpu_count < 1){
    printf("No Vulkan device found.\n");
    exit(-1);
  }

  VkPhysicalDevice gpu[32];
  res = vkEnumeratePhysicalDevices(instance, &gpu_count, gpu);
  if (res != VK_SUCCESS && res != VK_INCOMPLETE) {
    printf("vkEnumeratePhysicalDevices failed %d \n", res);
    exit(-1);
  }
  
  uint32_t idx = 0;
  bool use_idx = false;
  for (uint32_t i = 0; i < gpu_count && (!use_idx); i++)
  {
    uint32_t qfc = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu[i], &qfc, NULL);
    if (qfc < 1)continue;

    VkQueueFamilyProperties *queue_family_properties;
    queue_family_properties = malloc(qfc * sizeof(VkQueueFamilyProperties));

    vkGetPhysicalDeviceQueueFamilyProperties(gpu[i], &qfc, queue_family_properties);

    for (uint32_t j = 0; j < qfc; j++)
    {
      VkBool32 supports_present;
      vkGetPhysicalDeviceSurfaceSupportKHR(gpu[i], j, surface, &supports_present);

      if ((queue_family_properties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) && supports_present)
      {
        idx = i;
        use_idx = true;
        break;
      }
    }
    free(queue_family_properties);
  }
  if (!use_idx){
      printf("Not found suitable queue which supports graphics.\n");
      exit(-1);
  }

  printf("Using GPU device %lu\n", (unsigned long) idx);
  
  VulkanDevice *device = createVulkanDevice(gpu[idx]);

  int winWidth, winHeight;
  glfwGetWindowSize(window, &winWidth, &winHeight);

  VkQueue queue;
  vkGetDeviceQueue(device->device, device->graphicsQueueFamilyIndex, 0, &queue);
  FrameBuffers fb = createFrameBuffers(device, surface, queue, winWidth, winHeight, 0);

  VkCommandBuffer cmd_buffer = createCmdBuffer(device->device, device->commandPool);
  VKNVGCreateInfo create_info = {0};
  create_info.device = device->device;
  create_info.gpu = device->gpu;
  create_info.renderpass = fb.render_pass;
  create_info.cmdBuffer = cmd_buffer;

	int flags = 0;
#ifndef NDEBUG
	flags |= NVG_DEBUG; // unused in nanovg_vk
#endif
#if !defined(DEMO_MSAA) && defined(DEMO_ANTIALIAS)
	flags |= NVG_ANTIALIAS;
#endif
#ifdef DEMO_STENCIL_STROKES
	flags |= NVG_STENCIL_STROKES;
#endif

  NVGcontext *vg = nvgCreateVk(create_info, flags, queue);

  DemoData data;
  PerfGraph fps;//, cpuGraph, gpuGraph;
  if (loadDemoData(vg, &data) == -1)
    return -1;

  initGraph(&fps, GRAPH_RENDER_FPS, "Frame Time");
  //initGraph(&cpuGraph, GRAPH_RENDER_MS, "CPU Time");
  //initGraph(&gpuGraph, GRAPH_RENDER_MS, "GPU Time");
  double prevt = glfwGetTime();

  while (!glfwWindowShouldClose(window)) {
    float pxRatio;
    double mx, my, t, dt;

    int cwinWidth, cwinHeight;
    glfwGetWindowSize(window, &cwinWidth, &cwinHeight);
    if ((resize_event)||(winWidth != cwinWidth || winHeight != cwinHeight)) {
      winWidth = cwinWidth;
      winHeight = cwinHeight;
      destroyFrameBuffers(device, &fb);
      fb = createFrameBuffers(device, surface, queue, winWidth, winHeight, 0);
      resize_event=false;
    }else{

    prepareFrame(device->device, cmd_buffer, &fb);
    if(resize_event)continue;
    t = glfwGetTime();
    dt = t - prevt;
    prevt = t;
    updateGraph(&fps, (float)dt);
    pxRatio = (float)fb.buffer_size.width / (float)winWidth;

    glfwGetCursorPos(window, &mx, &my);

    nvgBeginFrame(vg, winWidth, winHeight, pxRatio);
    renderDemo(vg, (float)mx, (float)my, (float)winWidth, (float)winHeight, (float)t, blowup, &data);
    renderGraph(vg, 5, 5, &fps);

    nvgEndFrame(vg);
    
    submitFrame(device->device, queue, cmd_buffer, &fb);
}
    glfwPollEvents();
  }

  freeDemoData(vg, &data);
  nvgDeleteVk(vg);

  destroyFrameBuffers(device, &fb);

  destroyVulkanDevice(device);

  destroyDebugCallback(instance);

  vkDestroyInstance(instance, NULL);

  glfwDestroyWindow(window);
  
  printf("Average Frame Time: %.2f ms\n", getGraphAverage(&fps) * 1000.0f);
  //printf("          CPU Time: %.2f ms\n", getGraphAverage(&cpuGraph) * 1000.0f);
  //printf("          GPU Time: %.2f ms\n", getGraphAverage(&gpuGraph) * 1000.0f);
  
  glfwTerminate();
  return 0;
}
