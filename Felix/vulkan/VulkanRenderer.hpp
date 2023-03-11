#pragma once

#include <sstream>
#include <stdio.h>
#include <stdlib.h>

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include "VkBootstrap.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "vk_initializers.hpp"
#include "vk_mem_alloc.h"
#include "vk_texture.hpp"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "IVideoSink.hpp"
#include "Log.hpp"

#include "../IRenderer.hpp"
#include "../UI.hpp"
#include "../VideoSink.hpp"
#include "../version.hpp"
#include "ImageProperties.hpp"
#include "Utility.hpp"

#define VK_CHECK( x )                                                                                                                                                                                                                                                                                                                                                  \
  do                                                                                                                                                                                                                                                                                                                                                                   \
  {                                                                                                                                                                                                                                                                                                                                                                    \
    VkResult err = x;                                                                                                                                                                                                                                                                                                                                                  \
    if ( err )                                                                                                                                                                                                                                                                                                                                                         \
    {                                                                                                                                                                                                                                                                                                                                                                  \
      L_ERROR << "Detected Vulkan error: " << err;                                                                                                                                                                                                                                                                                                                     \
      abort();                                                                                                                                                                                                                                                                                                                                                         \
    }                                                                                                                                                                                                                                                                                                                                                                  \
  } while ( 0 )

struct DeletionQueue
{
  std::deque<std::function<void()>> deletors;

  void push_function( std::function<void()> &&function )
  {
    deletors.push_back( function );
  }

  void flush()
  {
    for ( auto it = deletors.rbegin(); it != deletors.rend(); it++ )
    {
      ( *it )();
    }
    deletors.clear();
  }
};

typedef struct LynxScreen
{
  uint8_t mBuffer[SCREEN_BUFFER_SIZE];
  uint8_t mPalette[32];
  ImageProperties::Rotation mRotation;
} LynxScreen;

class VulkanRenderer : public IRenderer
{
public:
  VulkanRenderer();
  ~VulkanRenderer() override;
  void initialize() override;
  int64_t render( UI &ui ) override;
  void terminate() override;
  bool shouldClose() override;
  void setTitle( std::string title ) override;
  std::shared_ptr<IVideoSink> getVideoSink() override;
  void registerFileDropCallback( std::function<void( std::filesystem::path )> callback ) override;
  void registerKeyEventCallback( std::function<void( int, bool )> callback ) override;
  void setRotation( ImageProperties::Rotation rotation ) override;

private:
  void setupVulkan( const char **extensions, uint32_t extensions_count );
  void setupVulkanWindow( ImGui_ImplVulkanH_Window *wd, VkSurfaceKHR surface, int width, int height );
  void cleanupVulkanWindow();
  void cleanupVulkan();

  VkCommandBuffer createCommandBuffer( VkCommandBufferLevel level, VkCommandPool pool, bool begin );
  void flushCommandBuffer( VkCommandBuffer commandBuffer, VkQueue queue, VkCommandPool pool, bool free );
  void setImageLayout( VkCommandBuffer cmdbuffer, VkImage image, VkImageAspectFlags aspectMask, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT );
  void prepareTextureTarget( VulkanTexture *tex, VkFormat format );
  void destroyTexture( VulkanTexture *tex );

  void prepareCompute();
  void destroyCompute();
  void buildComputeCommandBuffer();
  void destroyComputeCommandBuffer();
  VkPipelineShaderStageCreateInfo loadShader( std::string fileName, VkShaderStageFlagBits stage );

#if defined( VKB_DEBUG )
  VkDebugUtilsMessengerEXT debugUtilsMessenger{ VK_NULL_HANDLE };
  VkDebugReportCallbackEXT debugReportCallback{ VK_NULL_HANDLE };
#endif

  void frameRender( ImGui_ImplVulkanH_Window *wd, ImDrawData *draw_data );
  void framePresent( ImGui_ImplVulkanH_Window *wd );
  void renderImGui( UI &ui );
  void renderMainScreen();
  ImTextureID getMainScreenTextureID() override;

  std::function<void( std::filesystem::path )> mFileDropCallback = {};
  std::function<void( int, bool )> mKeyEventCallback = {};

  vkb::Device mVkbDevice;
  VkInstance mInstance = VK_NULL_HANDLE;
  VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
  VkPhysicalDeviceProperties mPhysicalDeviceProperties = {};
  VkDevice mDevice = VK_NULL_HANDLE;
  uint32_t mQueueFamily = (uint32_t)-1;
  VkQueue mQueue = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT mDebugReport = VK_NULL_HANDLE;
  VkSurfaceKHR mSurface = VK_NULL_HANDLE;
  VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;
  VkPipelineCache mPipelineCache = VK_NULL_HANDLE;
  VkCommandPool mCommandPool = VK_NULL_HANDLE;

  VmaAllocator mAllocator = {};
  VkAllocationCallbacks *mAllocationCallbacks = {};

  DeletionQueue mMainDeletionQueue;
  std::vector<VkShaderModule> mShaderModules;

  GLFWwindow *mMainWindow;
  ImGui_ImplVulkanH_Window mMainWindowData;
  int mMinImageCount = 2;
  bool mSwapChainRebuild = false;

  std::shared_ptr<VideoSink> mVideoSink;
  VmaAllocationInfo mLynxScreenAllocationInfo = {};
  VmaAllocation mLynxScreenAllocation = {};

  VulkanTexture mMainScreenTexture = {};
  AllocatedBuffer mMainScreenBuffer = {};
  LynxScreen mMainScreen = {};
  ImageProperties::Rotation mRotation = {};

  struct Compute
  {
    VkQueue queue;
    uint32_t queueFamily;
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    VkSemaphore semaphore;
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSet descriptorSet;
    VkPipelineLayout pipelineLayout;
    std::vector<VkPipeline> pipelines;
    int32_t pipelineIndex = 0;
  } mCompute;

  // struct
  //{
  //   VkDescriptorSetLayout   descriptorSetLayout;
  //   VkDescriptorSet         descriptorSetPreCompute;
  //   VkDescriptorSet         descriptorSetPostCompute;
  //   VkPipeline              pipeline;
  //   VkPipelineLayout        pipelineLayout;
  //   VkSemaphore             semaphore;
  // } mGraphics;
};
