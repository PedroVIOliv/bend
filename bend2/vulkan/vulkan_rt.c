/**
 * Vulkan Compute Runtime for Bend 2.0.
 *
 * Provides dynamic loading for vulkan-1.dll / libvulkan.so.1, device initialization,
 * 64-bit Buffer Device Address (BDA) memory allocation, pipeline binding,
 * and compute dispatch coordination for Bend programs.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include "bend_vk.h"

// Dynamic Vulkan Dispatch Table
typedef struct {
#ifdef _WIN32
  HMODULE lib;
#else
  void* lib;
#endif
  VkInstance instance;
  VkPhysicalDevice phys_dev;
  VkDevice dev;
  VkQueue queue;
  uint32_t queue_family;
  VkCommandPool cmd_pool;
  VkCommandBuffer cmd_buf;

  // Core Function Pointers
  PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
  PFN_vkCreateInstance vkCreateInstance;
  PFN_vkDestroyInstance vkDestroyInstance;
  PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices;
  PFN_vkGetPhysicalDeviceProperties2 vkGetPhysicalDeviceProperties2;
  PFN_vkGetPhysicalDeviceFeatures2 vkGetPhysicalDeviceFeatures2;
  PFN_vkGetPhysicalDeviceMemoryProperties vkGetPhysicalDeviceMemoryProperties;
  PFN_vkCreateDevice vkCreateDevice;
  PFN_vkDestroyDevice vkDestroyDevice;
  PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;
  PFN_vkGetDeviceQueue vkGetDeviceQueue;

  // Device Function Pointers
  PFN_vkCreateBuffer vkCreateBuffer;
  PFN_vkDestroyBuffer vkDestroyBuffer;
  PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements;
  PFN_vkAllocateMemory vkAllocateMemory;
  PFN_vkFreeMemory vkFreeMemory;
  PFN_vkBindBufferMemory vkBindBufferMemory;
  PFN_vkMapMemory vkMapMemory;
  PFN_vkUnmapMemory vkUnmapMemory;
  PFN_vkGetBufferDeviceAddress vkGetBufferDeviceAddress;

  PFN_vkCreateShaderModule vkCreateShaderModule;
  PFN_vkDestroyShaderModule vkDestroyShaderModule;
  PFN_vkCreatePipelineLayout vkCreatePipelineLayout;
  PFN_vkDestroyPipelineLayout vkDestroyPipelineLayout;
  PFN_vkCreateComputePipelines vkCreateComputePipelines;
  PFN_vkDestroyPipeline vkDestroyPipeline;

  PFN_vkCreateCommandPool vkCreateCommandPool;
  PFN_vkDestroyCommandPool vkDestroyCommandPool;
  PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers;
  PFN_vkBeginCommandBuffer vkBeginCommandBuffer;
  PFN_vkEndCommandBuffer vkEndCommandBuffer;
  PFN_vkCmdBindPipeline vkCmdBindPipeline;
  PFN_vkCmdPushConstants vkCmdPushConstants;
  PFN_vkCmdDispatch vkCmdDispatch;
  PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier;
  PFN_vkQueueSubmit vkQueueSubmit;
  PFN_vkQueueWaitIdle vkQueueWaitIdle;
} BendVkContext;

static void* vk_load_sym(void* lib, const char* name) {
#ifdef _WIN32
  return (void*)GetProcAddress((HMODULE)lib, name);
#else
  return dlsym(lib, name);
#endif
}

bool bend_vk_init(BendVkContext* ctx) {
  memset(ctx, 0, sizeof(*ctx));
#ifdef _WIN32
  ctx->lib = LoadLibraryA("vulkan-1.dll");
#else
  ctx->lib = dlopen("libvulkan.so.1", RTLD_NOW | RTLD_LOCAL);
  if (!ctx->lib) ctx->lib = dlopen("libvulkan.so", RTLD_NOW | RTLD_LOCAL);
#endif
  if (!ctx->lib) {
    fprintf(stderr, "bend_vk: cannot load Vulkan driver library\n");
    return false;
  }

  ctx->vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)vk_load_sym(ctx->lib, "vkGetInstanceProcAddr");
  if (!ctx->vkGetInstanceProcAddr) {
    fprintf(stderr, "bend_vk: cannot find vkGetInstanceProcAddr\n");
    return false;
  }

  ctx->vkCreateInstance = (PFN_vkCreateInstance)ctx->vkGetInstanceProcAddr(NULL, "vkCreateInstance");

  VkApplicationInfo appInfo = {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pApplicationName = "BendRT",
    .applicationVersion = VK_MAKE_VERSION(2, 0, 0),
    .pEngineName = "BendRT",
    .engineVersion = VK_MAKE_VERSION(2, 0, 0),
    .apiVersion = VK_API_VERSION_1_2
  };

  VkInstanceCreateInfo instInfo = {
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pApplicationInfo = &appInfo
  };

  VkResult res = ctx->vkCreateInstance(&instInfo, NULL, &ctx->instance);
  if (res != VK_SUCCESS) {
    fprintf(stderr, "bend_vk: vkCreateInstance failed (error %d)\n", res);
    return false;
  }

  #define LOAD_INST(fn) ctx->fn = (PFN_##fn)ctx->vkGetInstanceProcAddr(ctx->instance, #fn)
  LOAD_INST(vkDestroyInstance);
  LOAD_INST(vkEnumeratePhysicalDevices);
  LOAD_INST(vkGetPhysicalDeviceProperties2);
  LOAD_INST(vkGetPhysicalDeviceFeatures2);
  LOAD_INST(vkGetPhysicalDeviceMemoryProperties);
  LOAD_INST(vkCreateDevice);
  LOAD_INST(vkDestroyDevice);
  LOAD_INST(vkGetDeviceProcAddr);
  #undef LOAD_INST

  // Select Discrete GPU (or fallback to first device)
  uint32_t dev_count = 0;
  ctx->vkEnumeratePhysicalDevices(ctx->instance, &dev_count, NULL);
  if (dev_count == 0) {
    fprintf(stderr, "bend_vk: no Vulkan physical devices found\n");
    return false;
  }

  VkPhysicalDevice* devs = (VkPhysicalDevice*)malloc(dev_count * sizeof(VkPhysicalDevice));
  ctx->vkEnumeratePhysicalDevices(ctx->instance, &dev_count, devs);

  ctx->phys_dev = devs[0];
  for (uint32_t i = 0; i < dev_count; i++) {
    VkPhysicalDeviceProperties2 props2 = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
    ctx->vkGetPhysicalDeviceProperties2(devs[i], &props2);
    if (props2.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      ctx->phys_dev = devs[i];
      break;
    }
  }
  free(devs);

  // Enable Buffer Device Address & Int64 features
  VkPhysicalDeviceVulkan12Features feats12 = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
    .bufferDeviceAddress = VK_TRUE
  };
  VkPhysicalDeviceFeatures2 feats2 = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
    .pNext = &feats12,
    .features = {
      .shaderInt64 = VK_TRUE
    }
  };

  float queuePriority = 1.0f;
  VkDeviceQueueCreateInfo qInfo = {
    .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
    .queueFamilyIndex = 0,
    .queueCount = 1,
    .pQueuePriorities = &queuePriority
  };

  const char* devExts[] = {
    VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME
  };

  VkDeviceCreateInfo devInfo = {
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .pNext = &feats2,
    .queueCreateInfoCount = 1,
    .pQueueCreateInfos = &qInfo,
    .enabledExtensionCount = 1,
    .ppEnabledExtensionNames = devExts
  };

  res = ctx->vkCreateDevice(ctx->phys_dev, &devInfo, NULL, &ctx->dev);
  if (res != VK_SUCCESS) {
    fprintf(stderr, "bend_vk: vkCreateDevice failed (error %d)\n", res);
    return false;
  }

  #define LOAD_DEV(fn) ctx->fn = (PFN_##fn)ctx->vkGetDeviceProcAddr(ctx->dev, #fn)
  LOAD_DEV(vkGetDeviceQueue);
  LOAD_DEV(vkCreateBuffer);
  LOAD_DEV(vkDestroyBuffer);
  LOAD_DEV(vkGetBufferMemoryRequirements);
  LOAD_DEV(vkAllocateMemory);
  LOAD_DEV(vkFreeMemory);
  LOAD_DEV(vkBindBufferMemory);
  LOAD_DEV(vkMapMemory);
  LOAD_DEV(vkUnmapMemory);
  LOAD_DEV(vkGetBufferDeviceAddress);
  LOAD_DEV(vkCreateShaderModule);
  LOAD_DEV(vkDestroyShaderModule);
  LOAD_DEV(vkCreatePipelineLayout);
  LOAD_DEV(vkDestroyPipelineLayout);
  LOAD_DEV(vkCreateComputePipelines);
  LOAD_DEV(vkDestroyPipeline);
  LOAD_DEV(vkCreateCommandPool);
  LOAD_DEV(vkDestroyCommandPool);
  LOAD_DEV(vkAllocateCommandBuffers);
  LOAD_DEV(vkBeginCommandBuffer);
  LOAD_DEV(vkEndCommandBuffer);
  LOAD_DEV(vkCmdBindPipeline);
  LOAD_DEV(vkCmdPushConstants);
  LOAD_DEV(vkCmdDispatch);
  LOAD_DEV(vkCmdPipelineBarrier);
  LOAD_DEV(vkQueueSubmit);
  LOAD_DEV(vkQueueWaitIdle);
  #undef LOAD_DEV

  ctx->vkGetDeviceQueue(ctx->dev, 0, 0, &ctx->queue);

  VkCommandPoolCreateInfo poolInfo = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    .queueFamilyIndex = 0
  };
  ctx->vkCreateCommandPool(ctx->dev, &poolInfo, NULL, &ctx->cmd_pool);

  VkCommandBufferAllocateInfo allocInfo = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .commandPool = ctx->cmd_pool,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = 1
  };
  ctx->vkAllocateCommandBuffers(ctx->dev, &allocInfo, &ctx->cmd_buf);

  return true;
}

uint32_t bend_vk_find_memory_type(BendVkContext* ctx, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties memProperties;
  ctx->vkGetPhysicalDeviceMemoryProperties(ctx->phys_dev, &memProperties);
  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
      return i;
    }
  }
  return 0xFFFFFFFFu;
}

bool bend_vk_alloc_corpus(BendVkContext* ctx, size_t bytes, VkBuffer* out_buf, VkDeviceMemory* out_mem, uint64_t* out_bda, void** out_map) {
  VkBufferCreateInfo bufInfo = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size = bytes,
    .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
  };
  VkResult res = ctx->vkCreateBuffer(ctx->dev, &bufInfo, NULL, out_buf);
  if (res != VK_SUCCESS) {
    fprintf(stderr, "bend_vk: vkCreateBuffer failed: %d\n", res);
    return false;
  }

  VkMemoryRequirements memReqs;
  ctx->vkGetBufferMemoryRequirements(ctx->dev, *out_buf, &memReqs);

  VkPhysicalDeviceMemoryProperties memProperties;
  ctx->vkGetPhysicalDeviceMemoryProperties(ctx->phys_dev, &memProperties);

  VkMemoryPropertyFlags props = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
  uint32_t memType = bend_vk_find_memory_type(ctx, memReqs.memoryTypeBits, props | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  if (memType != 0xFFFFFFFFu) {
    uint32_t heapIdx = memProperties.memoryTypes[memType].heapIndex;
    if (memProperties.memoryHeaps[heapIdx].size < memReqs.size) {
      memType = 0xFFFFFFFFu;
    }
  }

  if (memType == 0xFFFFFFFFu) {
    memType = bend_vk_find_memory_type(ctx, memReqs.memoryTypeBits, props);
  }

  if (memType == 0xFFFFFFFFu) {
    fprintf(stderr, "bend_vk: could not find suitable memory type for bits 0x%x\n", memReqs.memoryTypeBits);
    return false;
  }

  VkMemoryAllocateFlagsInfo allocFlags = {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
    .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT
  };

  VkMemoryAllocateInfo allocInfo = {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .pNext = &allocFlags,
    .allocationSize = memReqs.size,
    .memoryTypeIndex = memType
  };

  res = ctx->vkAllocateMemory(ctx->dev, &allocInfo, NULL, out_mem);
  if (res != VK_SUCCESS) {
    fprintf(stderr, "bend_vk: vkAllocateMemory failed (code %d)\n", res);
    return false;
  }
  ctx->vkBindBufferMemory(ctx->dev, *out_buf, *out_mem, 0);

  VkBufferDeviceAddressInfo bdaInfo = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
    .buffer = *out_buf
  };
  *out_bda = ctx->vkGetBufferDeviceAddress(ctx->dev, &bdaInfo);

  if (out_map) {
    ctx->vkMapMemory(ctx->dev, *out_mem, 0, bytes, 0, out_map);
  }
  return true;
}

bool bend_vk_create_pipeline(BendVkContext* ctx, const uint32_t* spv_code, size_t spv_bytes, VkPipeline* out_pipe, VkPipelineLayout* out_layout) {
  VkShaderModuleCreateInfo smInfo = {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .codeSize = spv_bytes,
    .pCode = spv_code
  };
  VkShaderModule mod;
  if (ctx->vkCreateShaderModule(ctx->dev, &smInfo, NULL, &mod) != VK_SUCCESS) return false;

  VkPushConstantRange pushRange = {
    .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
    .offset = 0,
    .size = sizeof(uint64_t) + sizeof(uint32_t) * 2 // H, pass, grids
  };

  VkPipelineLayoutCreateInfo layoutInfo = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .pushConstantRangeCount = 1,
    .pPushConstantRanges = &pushRange
  };
  if (ctx->vkCreatePipelineLayout(ctx->dev, &layoutInfo, NULL, out_layout) != VK_SUCCESS) return false;

  VkComputePipelineCreateInfo pipeInfo = {
    .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
    .stage = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_COMPUTE_BIT,
      .module = mod,
      .pName = "main"
    },
    .layout = *out_layout
  };
  VkResult res = ctx->vkCreateComputePipelines(ctx->dev, VK_NULL_HANDLE, 1, &pipeInfo, NULL, out_pipe);
  ctx->vkDestroyShaderModule(ctx->dev, mod, NULL);
  return res == VK_SUCCESS;
}

// Forward declarations from Bend TEMPLATE
static const char* gpu_path(void);
static void gpu_note(const char* path);
static void gpu_run(uint32_t f);

// Bend Vulkan Runtime Interface
static BendVkContext    gpu_vk_ctx;
static VkBuffer         gpu_vk_buf;
static VkDeviceMemory   gpu_vk_mem;
static uint64_t         gpu_vk_bda;
static VkPipeline       gpu_vk_pipe;
static VkPipelineLayout gpu_vk_layout;
static bool             gpu_vk_open = false;

static bool gpu_probe(void) {
  static int probe_ok = -1;
  if (probe_ok != -1) return probe_ok == 1;
  probe_ok = bend_vk_init(&gpu_vk_ctx) ? 1 : 0;
  return probe_ok == 1;
}

static uint64_t gpu_span(void) {
  if (!gpu_probe()) return 0;
  VkPhysicalDeviceMemoryProperties memProps;
  gpu_vk_ctx.vkGetPhysicalDeviceMemoryProperties(gpu_vk_ctx.phys_dev, &memProps);
  uint64_t total = 0;
  for (uint32_t i = 0; i < memProps.memoryHeapCount; i++) {
    if (memProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
      if (memProps.memoryHeaps[i].size > total) {
        total = memProps.memoryHeaps[i].size;
      }
    }
  }
  return total < (2ull << 30) ? total : (2ull << 30);
}

static void* gpu_map(uint64_t bytes) {
  void* host_map = NULL;
  if (!bend_vk_alloc_corpus(&gpu_vk_ctx, bytes, &gpu_vk_buf, &gpu_vk_mem, &gpu_vk_bda, &host_map)) {
    fprintf(stderr, "bend_vk: corpus allocation of %llu bytes failed\n", (unsigned long long)bytes);
    exit(1);
  }
  return host_map;
}

static bool gpu_make(const char* path) {
  FILE* f = fopen(path, "rb");
  if (f) {
    fclose(f);
    return true;
  }
  return false;
}

static void gpu_load(uint64_t bytes) {
  const char* path = gpu_path();
  FILE* f = fopen(path, "rb");
  if (!f) {
    gpu_note(path);
    if (!gpu_make(path)) {
      fprintf(stderr, "bend_vk: cannot find or compile GPU program: %s\n", path);
      exit(1);
    }
    f = fopen(path, "rb");
    if (!f) {
      fprintf(stderr, "bend_vk: cannot open GPU program: %s\n", path);
      exit(1);
    }
  }
  fseek(f, 0, SEEK_END);
  size_t len = ftell(f);
  fseek(f, 0, SEEK_SET);
  uint32_t* spv = (uint32_t*)malloc(len);
  if (!spv || fread(spv, 1, len, f) != len) {
    if (spv) free(spv);
    fclose(f);
    fprintf(stderr, "bend_vk: cannot read GPU program: %s\n", path);
    exit(1);
  }
  fclose(f);
  if (!bend_vk_create_pipeline(&gpu_vk_ctx, spv, len, &gpu_vk_pipe, &gpu_vk_layout)) {
    free(spv);
    fprintf(stderr, "bend_vk: failed to create Vulkan compute pipeline\n");
    exit(1);
  }
  free(spv);
}

static void gpu_kernel(uint32_t pass, uint32_t groups) {
  VkCommandBufferBeginInfo beginInfo = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
  };
  gpu_vk_ctx.vkBeginCommandBuffer(gpu_vk_ctx.cmd_buf, &beginInfo);
  VkMemoryBarrier host_in = {
    .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
    .srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_SHADER_WRITE_BIT,
    .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT
  };
  gpu_vk_ctx.vkCmdPipelineBarrier(gpu_vk_ctx.cmd_buf,
    VK_PIPELINE_STAGE_HOST_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    0, 1, &host_in, 0, NULL, 0, NULL);
  gpu_vk_ctx.vkCmdBindPipeline(gpu_vk_ctx.cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, gpu_vk_pipe);
  struct {
    uint64_t H;
    uint32_t pass;
    uint32_t grids;
  } push = { gpu_vk_bda, pass, groups };
  gpu_vk_ctx.vkCmdPushConstants(gpu_vk_ctx.cmd_buf, gpu_vk_layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(push), &push);
  gpu_vk_ctx.vkCmdDispatch(gpu_vk_ctx.cmd_buf, groups, 1, 1);
  VkMemoryBarrier out_barrier = {
    .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
    .srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
    .dstAccessMask = VK_ACCESS_HOST_READ_BIT | VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT
  };
  gpu_vk_ctx.vkCmdPipelineBarrier(gpu_vk_ctx.cmd_buf,
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    VK_PIPELINE_STAGE_HOST_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    0, 1, &out_barrier, 0, NULL, 0, NULL);
  gpu_vk_ctx.vkEndCommandBuffer(gpu_vk_ctx.cmd_buf);
  VkSubmitInfo submitInfo = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    .commandBufferCount = 1,
    .pCommandBuffers = &gpu_vk_ctx.cmd_buf
  };
  gpu_vk_ctx.vkQueueSubmit(gpu_vk_ctx.queue, 1, &submitInfo, VK_NULL_HANDLE);
  gpu_vk_ctx.vkQueueWaitIdle(gpu_vk_ctx.queue);
}

static void gpu_pass(uint32_t f) {
  gpu_run(f);
}
