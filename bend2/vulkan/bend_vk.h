#ifndef BEND_VK_H
#define BEND_VK_H

#include <stdint.h>
#include <stdbool.h>

#ifdef _WIN32
#define VKAPI_PTR __stdcall
#else
#define VKAPI_PTR
#endif

// Core Types
typedef uint32_t VkFlags;
typedef uint32_t VkBool32;
typedef uint64_t VkDeviceSize;
typedef uint64_t VkDeviceAddress;

typedef enum VkResult {
  VK_SUCCESS = 0,
  VK_NOT_READY = 1,
  VK_TIMEOUT = 2,
  VK_ERROR_OUT_OF_HOST_MEMORY = -1,
  VK_ERROR_OUT_OF_DEVICE_MEMORY = -2,
  VK_ERROR_INITIALIZATION_FAILED = -3,
  VK_ERROR_DEVICE_LOST = -4,
  VK_ERROR_EXTENSION_NOT_PRESENT = -7,
  VK_ERROR_FEATURE_NOT_PRESENT = -8,
} VkResult;

#define VK_NULL_HANDLE 0
#define VK_TRUE 1
#define VK_FALSE 0

#define VK_MAKE_VERSION(major, minor, patch) \
  ((((uint32_t)(major)) << 22) | (((uint32_t)(minor)) << 12) | ((uint32_t)(patch)))
#define VK_API_VERSION_1_2 VK_MAKE_VERSION(1, 2, 0)
#define VK_API_VERSION_1_3 VK_MAKE_VERSION(1, 3, 0)

#define VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME "VK_KHR_buffer_device_address"

typedef enum VkStructureType {
  VK_STRUCTURE_TYPE_APPLICATION_INFO = 0,
  VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO = 1,
  VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO = 2,
  VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO = 3,
  VK_STRUCTURE_TYPE_SUBMIT_INFO = 4,
  VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO = 5,
  VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO = 12,
  VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO = 16,
  VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO = 18,
  VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO = 29,
  VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO = 30,
  VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO = 39,
  VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO = 40,
  VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO = 42,
  VK_STRUCTURE_TYPE_MEMORY_BARRIER = 46,
  VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 = 1000059000,
  VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 = 1000059001,
  VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES = 1000094000,
  VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO = 1000060000,
  VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES = 1000257000,
  VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO = 1000257001,
} VkStructureType;

typedef enum VkPhysicalDeviceType {
  VK_PHYSICAL_DEVICE_TYPE_OTHER = 0,
  VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU = 1,
  VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU = 2,
  VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU = 3,
  VK_PHYSICAL_DEVICE_TYPE_CPU = 4,
} VkPhysicalDeviceType;

#define VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT  0x00000001
#define VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT   0x00000002
#define VK_MEMORY_PROPERTY_HOST_COHERENT_BIT  0x00000004
#define VK_MEMORY_PROPERTY_HOST_CACHED_BIT    0x00000008

#define VK_MEMORY_HEAP_DEVICE_LOCAL_BIT       0x00000001

#define VK_BUFFER_USAGE_STORAGE_BUFFER_BIT         0x00000020
#define VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT  0x00020000

#define VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT      0x00000002

#define VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT       0x00000800
#define VK_PIPELINE_STAGE_HOST_BIT                 0x00004000

#define VK_ACCESS_SHADER_READ_BIT                  0x00000020
#define VK_ACCESS_SHADER_WRITE_BIT                 0x00000040
#define VK_ACCESS_HOST_READ_BIT                    0x00001000
#define VK_ACCESS_HOST_WRITE_BIT                   0x00002000

#define VK_SHADER_STAGE_COMPUTE_BIT                0x00000020
#define VK_PIPELINE_BIND_POINT_COMPUTE             1

#define VK_COMMAND_BUFFER_LEVEL_PRIMARY            0
#define VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT 0x00000001
#define VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT 0x00000002

typedef VkFlags VkMemoryPropertyFlags;

// Handles
#define VK_DEFINE_HANDLE(object) typedef struct object##_T* object;
#define VK_DEFINE_NON_DISPATCHABLE_HANDLE(object) typedef uint64_t object;

VK_DEFINE_HANDLE(VkInstance)
VK_DEFINE_HANDLE(VkPhysicalDevice)
VK_DEFINE_HANDLE(VkDevice)
VK_DEFINE_HANDLE(VkQueue)
VK_DEFINE_HANDLE(VkCommandBuffer)

VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkBuffer)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkDeviceMemory)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkCommandPool)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkPipelineLayout)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkPipeline)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkShaderModule)

// Structs
typedef struct VkApplicationInfo {
  VkStructureType sType;
  const void*     pNext;
  const char*     pApplicationName;
  uint32_t        applicationVersion;
  const char*     pEngineName;
  uint32_t        engineVersion;
  uint32_t        apiVersion;
} VkApplicationInfo;

typedef struct VkInstanceCreateInfo {
  VkStructureType          sType;
  const void*              pNext;
  VkFlags                  flags;
  const VkApplicationInfo* pApplicationInfo;
  uint32_t                 enabledLayerCount;
  const char* const*       ppEnabledLayerNames;
  uint32_t                 enabledExtensionCount;
  const char* const*       ppEnabledExtensionNames;
} VkInstanceCreateInfo;

typedef struct VkDeviceQueueCreateInfo {
  VkStructureType sType;
  const void*     pNext;
  VkFlags         flags;
  uint32_t        queueFamilyIndex;
  uint32_t        queueCount;
  const float*    pQueuePriorities;
} VkDeviceQueueCreateInfo;

typedef struct VkPhysicalDeviceFeatures {
  VkBool32 robustBufferAccess;
  VkBool32 fullDrawIndexUint32;
  VkBool32 imageCubeArray;
  VkBool32 independentBlend;
  VkBool32 geometryShader;
  VkBool32 tessellationShader;
  VkBool32 sampleRateShading;
  VkBool32 dualSrcBlend;
  VkBool32 logicOp;
  VkBool32 multiDrawIndirect;
  VkBool32 drawIndirectFirstInstance;
  VkBool32 depthClamp;
  VkBool32 depthBiasClamp;
  VkBool32 fillModeNonSolid;
  VkBool32 depthBounds;
  VkBool32 wideLines;
  VkBool32 largePoints;
  VkBool32 alphaToOne;
  VkBool32 multiViewport;
  VkBool32 samplerAnisotropy;
  VkBool32 textureCompressionETC2;
  VkBool32 textureCompressionASTC_LDR;
  VkBool32 textureCompressionBC;
  VkBool32 occlusionQueryPrecise;
  VkBool32 pipelineStatisticsQuery;
  VkBool32 vertexPipelineStoresAndAtomics;
  VkBool32 fragmentStoresAndAtomics;
  VkBool32 shaderTessellationAndGeometryPointSize;
  VkBool32 shaderImageGatherExtended;
  VkBool32 shaderStorageImageExtendedFormats;
  VkBool32 shaderStorageImageMultisample;
  VkBool32 shaderStorageImageReadWithoutFormat;
  VkBool32 shaderStorageImageWriteWithoutFormat;
  VkBool32 shaderUniformBufferArrayDynamicIndexing;
  VkBool32 shaderSampledImageArrayDynamicIndexing;
  VkBool32 shaderStorageBufferArrayDynamicIndexing;
  VkBool32 shaderStorageImageArrayDynamicIndexing;
  VkBool32 shaderClipDistance;
  VkBool32 shaderCullDistance;
  VkBool32 shaderFloat64;
  VkBool32 shaderInt64;
  VkBool32 shaderInt16;
  VkBool32 shaderResourceResidency;
  VkBool32 shaderResourceMinLod;
  VkBool32 sparseBinding;
  VkBool32 sparseResidencyBuffer;
  VkBool32 sparseResidencyImage2D;
  VkBool32 sparseResidencyImage3D;
  VkBool32 sparseResidency2Samples;
  VkBool32 sparseResidency4Samples;
  VkBool32 sparseResidency8Samples;
  VkBool32 sparseResidency16Samples;
  VkBool32 sparseResidencyAliased;
  VkBool32 variableMultisampleRate;
  VkBool32 inheritedQueries;
} VkPhysicalDeviceFeatures;

typedef struct VkDeviceCreateInfo {
  VkStructureType                sType;
  const void*                    pNext;
  VkFlags                        flags;
  uint32_t                       queueCreateInfoCount;
  const VkDeviceQueueCreateInfo* pQueueCreateInfos;
  uint32_t                       enabledLayerCount;
  const char* const*             ppEnabledLayerNames;
  uint32_t                       enabledExtensionCount;
  const char* const*             ppEnabledExtensionNames;
  const VkPhysicalDeviceFeatures* pEnabledFeatures;
} VkDeviceCreateInfo;

typedef struct VkPhysicalDeviceLimits {
  uint32_t maxImageDimension1D;
  uint32_t maxImageDimension2D;
  uint32_t maxImageDimension3D;
  uint32_t maxImageDimensionCube;
  uint32_t maxImageArrayLayers;
  uint32_t maxTexelBufferElements;
  uint32_t maxUniformBufferRange;
  uint32_t maxStorageBufferRange;
  uint32_t maxPushConstantsSize;
  uint32_t maxMemoryAllocationCount;
  uint32_t maxSamplerAllocationCount;
  VkDeviceSize bufferImageGranularity;
  VkDeviceSize sparseAddressSpaceSize;
  uint32_t maxBoundDescriptorSets;
  uint32_t maxPerStageDescriptorSamplers;
  uint32_t maxPerStageDescriptorUniformBuffers;
  uint32_t maxPerStageDescriptorStorageBuffers;
  uint32_t maxPerStageDescriptorSampledImages;
  uint32_t maxPerStageDescriptorStorageImages;
  uint32_t maxPerStageDescriptorInputAttachments;
  uint32_t maxPerStageResources;
  uint32_t maxDescriptorSetSamplers;
  uint32_t maxDescriptorSetUniformBuffers;
  uint32_t maxDescriptorSetUniformBuffersDynamic;
  uint32_t maxDescriptorSetStorageBuffers;
  uint32_t maxDescriptorSetStorageBuffersDynamic;
  uint32_t maxDescriptorSetSampledImages;
  uint32_t maxDescriptorSetStorageImages;
  uint32_t maxDescriptorSetInputAttachments;
  uint32_t maxVertexInputAttributes;
  uint32_t maxVertexInputBindings;
  uint32_t maxVertexInputAttributeOffset;
  uint32_t maxVertexInputBindingStride;
  uint32_t maxVertexOutputComponents;
  uint32_t maxTessellationGenerationLevel;
  uint32_t maxTessellationPatchSize;
  uint32_t maxTessellationControlPerVertexInputComponents;
  uint32_t maxTessellationControlPerVertexOutputComponents;
  uint32_t maxTessellationControlPerPatchOutputComponents;
  uint32_t maxTessellationControlTotalOutputComponents;
  uint32_t maxTessellationEvaluationInputComponents;
  uint32_t maxTessellationEvaluationOutputComponents;
  uint32_t maxGeometryShaderInvocations;
  uint32_t maxGeometryInputComponents;
  uint32_t maxGeometryOutputComponents;
  uint32_t maxGeometryOutputVertices;
  uint32_t maxGeometryTotalOutputComponents;
  uint32_t maxFragmentInputComponents;
  uint32_t maxFragmentOutputAttachments;
  uint32_t maxFragmentDualSrcAttachments;
  uint32_t maxFragmentCombinedOutputResources;
  uint32_t maxComputeSharedMemorySize;
  uint32_t maxComputeWorkGroupCount[3];
  uint32_t maxComputeWorkGroupInvocations;
  uint32_t maxComputeWorkGroupSize[3];
  uint32_t subPixelPrecisionBits;
  uint32_t subTexelPrecisionBits;
  uint32_t mipmapPrecisionBits;
  uint32_t maxDrawIndexedIndexValue;
  uint32_t maxDrawIndirectCount;
  float    maxSamplerLodBias;
  float    maxSamplerAnisotropy;
  uint32_t maxViewports;
  uint32_t maxViewportDimensions[2];
  float    viewportBoundsRange[2];
  uint32_t viewportSubPixelBits;
  size_t   minMemoryMapAlignment;
  VkDeviceSize minTexelBufferOffsetAlignment;
  VkDeviceSize minUniformBufferOffsetAlignment;
  VkDeviceSize minStorageBufferOffsetAlignment;
  int32_t  minTexelOffset;
  uint32_t maxTexelOffset;
  int32_t  minTexelGatherOffset;
  uint32_t maxTexelGatherOffset;
  float    minInterpolationOffset;
  float    maxInterpolationOffset;
  uint32_t subPixelInterpolationOffsetBits;
  uint32_t maxFramebufferWidth;
  uint32_t maxFramebufferHeight;
  uint32_t maxFramebufferLayers;
  VkFlags  framebufferColorSampleCounts;
  VkFlags  framebufferDepthSampleCounts;
  VkFlags  framebufferStencilSampleCounts;
  VkFlags  framebufferNoAttachmentsSampleCounts;
  uint32_t maxColorAttachments;
  VkFlags  sampledImageColorSampleCounts;
  VkFlags  sampledImageIntegerSampleCounts;
  VkFlags  sampledImageDepthSampleCounts;
  VkFlags  sampledImageStencilSampleCounts;
  VkFlags  storageImageSampleCounts;
  uint32_t maxSampleMaskWords;
  VkBool32 timestampComputeAndGraphics;
  float    timestampPeriod;
  uint32_t maxClipDistances;
  uint32_t maxCullDistances;
  uint32_t maxCombinedClipAndCullDistances;
  uint32_t discreteQueuePriorities;
  float    pointSizeRange[2];
  float    lineWidthRange[2];
  float    pointSizeGranularity;
  float    lineWidthGranularity;
  VkBool32 strictLines;
  VkBool32 standardSampleLocations;
  VkDeviceSize optimalBufferCopyOffsetAlignment;
  VkDeviceSize optimalBufferCopyRowPitchAlignment;
  VkDeviceSize nonCoherentAtomSize;
} VkPhysicalDeviceLimits;

typedef struct VkPhysicalDeviceProperties {
  uint32_t               apiVersion;
  uint32_t               driverVersion;
  uint32_t               vendorID;
  uint32_t               deviceID;
  VkPhysicalDeviceType   deviceType;
  char                   deviceName[256];
  uint8_t                pipelineCacheUUID[16];
  VkPhysicalDeviceLimits limits;
  uint8_t                sparseProperties[20];
} VkPhysicalDeviceProperties;

typedef struct VkPhysicalDeviceProperties2 {
  VkStructureType            sType;
  void*                      pNext;
  VkPhysicalDeviceProperties properties;
} VkPhysicalDeviceProperties2;

typedef struct VkMemoryType {
  VkMemoryPropertyFlags propertyFlags;
  uint32_t              heapIndex;
} VkMemoryType;

typedef struct VkMemoryHeap {
  VkDeviceSize size;
  VkFlags      flags;
} VkMemoryHeap;

typedef struct VkPhysicalDeviceMemoryProperties {
  uint32_t     memoryTypeCount;
  VkMemoryType memoryTypes[32];
  uint32_t     memoryHeapCount;
  VkMemoryHeap memoryHeaps[16];
} VkPhysicalDeviceMemoryProperties;

typedef struct VkPhysicalDeviceFeatures2 {
  VkStructureType          sType;
  void*                    pNext;
  VkPhysicalDeviceFeatures features;
} VkPhysicalDeviceFeatures2;

typedef struct VkPhysicalDeviceVulkan12Features {
  VkStructureType sType;
  void*           pNext;
  VkBool32        samplerMirrorClampToEdge;
  VkBool32        drawIndirectCount;
  VkBool32        storageBuffer8BitAccess;
  VkBool32        uniformAndStorageBuffer8BitAccess;
  VkBool32        storagePushConstant8;
  VkBool32        shaderBufferInt64Atomics;
  VkBool32        shaderSharedInt64Atomics;
  VkBool32        shaderFloat16;
  VkBool32        shaderInt8;
  VkBool32        descriptorIndexing;
  VkBool32        shaderInputAttachmentArrayDynamicIndexing;
  VkBool32        shaderUniformTexelBufferArrayDynamicIndexing;
  VkBool32        shaderStorageTexelBufferArrayDynamicIndexing;
  VkBool32        shaderUniformBufferArrayNonUniformIndexing;
  VkBool32        shaderSampledImageArrayNonUniformIndexing;
  VkBool32        shaderStorageBufferArrayNonUniformIndexing;
  VkBool32        shaderStorageImageArrayNonUniformIndexing;
  VkBool32        shaderInputAttachmentArrayNonUniformIndexing;
  VkBool32        shaderUniformTexelBufferArrayNonUniformIndexing;
  VkBool32        shaderStorageTexelBufferArrayNonUniformIndexing;
  VkBool32        descriptorBindingUniformBufferUpdateAfterBind;
  VkBool32        descriptorBindingSampledImageUpdateAfterBind;
  VkBool32        descriptorBindingStorageImageUpdateAfterBind;
  VkBool32        descriptorBindingStorageBufferUpdateAfterBind;
  VkBool32        descriptorBindingUniformTexelBufferUpdateAfterBind;
  VkBool32        descriptorBindingStorageTexelBufferUpdateAfterBind;
  VkBool32        descriptorBindingUpdateUnusedWhilePending;
  VkBool32        descriptorBindingPartiallyBound;
  VkBool32        descriptorBindingVariableDescriptorCount;
  VkBool32        runtimeDescriptorArray;
  VkBool32        samplerFilterMinmax;
  VkBool32        scalarBlockLayout;
  VkBool32        imagelessFramebuffer;
  VkBool32        uniformBufferStandardLayout;
  VkBool32        shaderSubgroupExtendedTypes;
  VkBool32        separateDepthStencilLayouts;
  VkBool32        hostQueryReset;
  VkBool32        timelineSemaphore;
  VkBool32        bufferDeviceAddress;
  VkBool32        bufferDeviceAddressCaptureReplay;
  VkBool32        bufferDeviceAddressMultiDevice;
  VkBool32        vulkanMemoryModel;
  VkBool32        vulkanMemoryModelDeviceScope;
  VkBool32        vulkanMemoryModelAvailabilityVisibilityChains;
  VkBool32        shaderOutputViewportIndex;
  VkBool32        shaderOutputLayer;
  VkBool32        subgroupBroadcastDynamicId;
} VkPhysicalDeviceVulkan12Features;

typedef struct VkPhysicalDeviceSubgroupProperties {
  VkStructureType sType;
  void*           pNext;
  uint32_t        subgroupSize;
  VkFlags         supportedStages;
  VkFlags         supportedOperations;
  VkBool32        quadOperationsInAllStages;
} VkPhysicalDeviceSubgroupProperties;

typedef struct VkBufferCreateInfo {
  VkStructureType sType;
  const void*     pNext;
  VkFlags         flags;
  VkDeviceSize    size;
  VkFlags         usage;
  uint32_t        sharingMode;
  uint32_t        queueFamilyIndexCount;
  const uint32_t* pQueueFamilyIndices;
} VkBufferCreateInfo;

typedef struct VkMemoryRequirements {
  VkDeviceSize size;
  VkDeviceSize alignment;
  uint32_t     memoryTypeBits;
} VkMemoryRequirements;

typedef struct VkMemoryAllocateInfo {
  VkStructureType sType;
  const void*     pNext;
  VkDeviceSize    allocationSize;
  uint32_t        memoryTypeIndex;
} VkMemoryAllocateInfo;

typedef struct VkMemoryAllocateFlagsInfo {
  VkStructureType sType;
  const void*     pNext;
  VkFlags         flags;
  uint32_t        deviceMask;
} VkMemoryAllocateFlagsInfo;

typedef struct VkBufferDeviceAddressInfo {
  VkStructureType sType;
  const void*     pNext;
  VkBuffer        buffer;
} VkBufferDeviceAddressInfo;

typedef struct VkCommandPoolCreateInfo {
  VkStructureType sType;
  const void*     pNext;
  VkFlags         flags;
  uint32_t        queueFamilyIndex;
} VkCommandPoolCreateInfo;

typedef struct VkCommandBufferAllocateInfo {
  VkStructureType sType;
  const void*     pNext;
  VkCommandPool   commandPool;
  uint32_t        level;
  uint32_t        commandBufferCount;
} VkCommandBufferAllocateInfo;

typedef struct VkCommandBufferBeginInfo {
  VkStructureType sType;
  const void*     pNext;
  VkFlags         flags;
  const void*     pInheritanceInfo;
} VkCommandBufferBeginInfo;

typedef struct VkMemoryBarrier {
  VkStructureType sType;
  const void*     pNext;
  VkFlags         srcAccessMask;
  VkFlags         dstAccessMask;
} VkMemoryBarrier;

typedef struct VkPushConstantRange {
  VkFlags  stageFlags;
  uint32_t offset;
  uint32_t size;
} VkPushConstantRange;

typedef struct VkPipelineLayoutCreateInfo {
  VkStructureType            sType;
  const void*                pNext;
  VkFlags                    flags;
  uint32_t                   setLayoutCount;
  const void*                pSetLayouts;
  uint32_t                   pushConstantRangeCount;
  const VkPushConstantRange* pPushConstantRanges;
} VkPipelineLayoutCreateInfo;

typedef struct VkShaderModuleCreateInfo {
  VkStructureType sType;
  const void*     pNext;
  VkFlags         flags;
  size_t          codeSize;
  const uint32_t* pCode;
} VkShaderModuleCreateInfo;

typedef struct VkPipelineShaderStageCreateInfo {
  VkStructureType sType;
  const void*     pNext;
  VkFlags         flags;
  uint32_t        stage;
  VkShaderModule  module;
  const char*     pName;
  const void*     pSpecializationInfo;
} VkPipelineShaderStageCreateInfo;

typedef struct VkComputePipelineCreateInfo {
  VkStructureType                 sType;
  const void*                     pNext;
  VkFlags                         flags;
  VkPipelineShaderStageCreateInfo stage;
  VkPipelineLayout                layout;
  VkPipeline                      basePipelineHandle;
  int32_t                         basePipelineIndex;
} VkComputePipelineCreateInfo;

typedef struct VkSubmitInfo {
  VkStructureType          sType;
  const void*              pNext;
  uint32_t                 waitSemaphoreCount;
  const void*              pWaitSemaphores;
  const VkFlags*           pWaitDstStageMask;
  uint32_t                 commandBufferCount;
  const VkCommandBuffer*   pCommandBuffers;
  uint32_t                 signalSemaphoreCount;
  const void*              pSignalSemaphores;
} VkSubmitInfo;

// Function Pointer Typedefs
typedef void* (VKAPI_PTR *PFN_vkVoidFunction)(void);
typedef PFN_vkVoidFunction (VKAPI_PTR *PFN_vkGetInstanceProcAddr)(VkInstance instance, const char* pName);
typedef PFN_vkVoidFunction (VKAPI_PTR *PFN_vkGetDeviceProcAddr)(VkDevice device, const char* pName);

typedef VkResult (VKAPI_PTR *PFN_vkCreateInstance)(const VkInstanceCreateInfo* pCreateInfo, const void* pAllocator, VkInstance* pInstance);
typedef void (VKAPI_PTR *PFN_vkDestroyInstance)(VkInstance instance, const void* pAllocator);
typedef VkResult (VKAPI_PTR *PFN_vkEnumeratePhysicalDevices)(VkInstance instance, uint32_t* pPhysicalDeviceCount, VkPhysicalDevice* pPhysicalDevices);
typedef void (VKAPI_PTR *PFN_vkGetPhysicalDeviceProperties2)(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties2* pProperties);
typedef void (VKAPI_PTR *PFN_vkGetPhysicalDeviceMemoryProperties)(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties* pMemoryProperties);
typedef void (VKAPI_PTR *PFN_vkGetPhysicalDeviceFeatures2)(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures2* pFeatures);
typedef VkResult (VKAPI_PTR *PFN_vkCreateDevice)(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, const void* pAllocator, VkDevice* pDevice);
typedef void (VKAPI_PTR *PFN_vkDestroyDevice)(VkDevice device, const void* pAllocator);
typedef void (VKAPI_PTR *PFN_vkGetDeviceQueue)(VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue* pQueue);

typedef VkResult (VKAPI_PTR *PFN_vkCreateBuffer)(VkDevice device, const VkBufferCreateInfo* pCreateInfo, const void* pAllocator, VkBuffer* pBuffer);
typedef void (VKAPI_PTR *PFN_vkDestroyBuffer)(VkDevice device, VkBuffer buffer, const void* pAllocator);
typedef void (VKAPI_PTR *PFN_vkGetBufferMemoryRequirements)(VkDevice device, VkBuffer buffer, VkMemoryRequirements* pMemoryRequirements);
typedef VkResult (VKAPI_PTR *PFN_vkAllocateMemory)(VkDevice device, const VkMemoryAllocateInfo* pAllocateInfo, const void* pAllocator, VkDeviceMemory* pMemory);
typedef void (VKAPI_PTR *PFN_vkFreeMemory)(VkDevice device, VkDeviceMemory memory, const void* pAllocator);
typedef VkResult (VKAPI_PTR *PFN_vkBindBufferMemory)(VkDevice device, VkBuffer buffer, VkDeviceMemory memory, VkDeviceSize memoryOffset);
typedef VkDeviceAddress (VKAPI_PTR *PFN_vkGetBufferDeviceAddress)(VkDevice device, const VkBufferDeviceAddressInfo* pInfo);
typedef VkResult (VKAPI_PTR *PFN_vkMapMemory)(VkDevice device, VkDeviceMemory memory, VkDeviceSize offset, VkDeviceSize size, VkFlags flags, void** ppData);
typedef void (VKAPI_PTR *PFN_vkUnmapMemory)(VkDevice device, VkDeviceMemory memory);

typedef VkResult (VKAPI_PTR *PFN_vkCreateCommandPool)(VkDevice device, const VkCommandPoolCreateInfo* pCreateInfo, const void* pAllocator, VkCommandPool* pCommandPool);
typedef void (VKAPI_PTR *PFN_vkDestroyCommandPool)(VkDevice device, VkCommandPool commandPool, const void* pAllocator);
typedef VkResult (VKAPI_PTR *PFN_vkAllocateCommandBuffers)(VkDevice device, const VkCommandBufferAllocateInfo* pAllocateInfo, VkCommandBuffer* pCommandBuffers);

typedef VkResult (VKAPI_PTR *PFN_vkCreateShaderModule)(VkDevice device, const VkShaderModuleCreateInfo* pCreateInfo, const void* pAllocator, VkShaderModule* pShaderModule);
typedef void (VKAPI_PTR *PFN_vkDestroyShaderModule)(VkDevice device, VkShaderModule shaderModule, const void* pAllocator);
typedef VkResult (VKAPI_PTR *PFN_vkCreatePipelineLayout)(VkDevice device, const VkPipelineLayoutCreateInfo* pCreateInfo, const void* pAllocator, VkPipelineLayout* pPipelineLayout);
typedef void (VKAPI_PTR *PFN_vkDestroyPipelineLayout)(VkDevice device, VkPipelineLayout pipelineLayout, const void* pAllocator);
typedef VkResult (VKAPI_PTR *PFN_vkCreateComputePipelines)(VkDevice device, VkPipeline pipelineCache, uint32_t createInfoCount, const VkComputePipelineCreateInfo* pCreateInfos, const void* pAllocator, VkPipeline* pPipelines);
typedef void (VKAPI_PTR *PFN_vkDestroyPipeline)(VkDevice device, VkPipeline pipeline, const void* pAllocator);

typedef VkResult (VKAPI_PTR *PFN_vkBeginCommandBuffer)(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo* pBeginInfo);
typedef VkResult (VKAPI_PTR *PFN_vkEndCommandBuffer)(VkCommandBuffer commandBuffer);
typedef void (VKAPI_PTR *PFN_vkCmdBindPipeline)(VkCommandBuffer commandBuffer, uint32_t pipelineBindPoint, VkPipeline pipeline);
typedef void (VKAPI_PTR *PFN_vkCmdPushConstants)(VkCommandBuffer commandBuffer, VkPipelineLayout layout, VkFlags stageFlags, uint32_t offset, uint32_t size, const void* pValues);
typedef void (VKAPI_PTR *PFN_vkCmdPipelineBarrier)(VkCommandBuffer commandBuffer, VkFlags srcStageMask, VkFlags dstStageMask, VkFlags dependencyFlags, uint32_t memoryBarrierCount, const VkMemoryBarrier* pMemoryBarriers, uint32_t bufferMemoryBarrierCount, const void* pBufferMemoryBarriers, uint32_t imageMemoryBarrierCount, const void* pImageMemoryBarriers);
typedef void (VKAPI_PTR *PFN_vkCmdDispatch)(VkCommandBuffer commandBuffer, uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
typedef VkResult (VKAPI_PTR *PFN_vkQueueSubmit)(VkQueue queue, uint32_t submitCount, const VkSubmitInfo* pSubmits, void* fence);
typedef VkResult (VKAPI_PTR *PFN_vkQueueWaitIdle)(VkQueue queue);

#endif // BEND_VK_H
