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
  do                                                     \
  {                                                      \
    VkResult err = x;                                    \
    if ( err )                                           \
    {                                                    \
      L_ERROR << "Detected Vulkan error: " << err;       \
      abort();                                           \
    }                                                    \
  } while ( 0 )

typedef struct LynxScreenBuffer
{
  uint8_t mBuffer[SCREEN_BUFFER_SIZE];
  uint8_t mPalette[32];
  ImageProperties::Rotation mRotation;
} LynxScreenBuffer;

typedef struct BoardBuffer
{
  uint8_t content[ 256 * 256 ];
  uint8_t font[ 256 * 8 * 16 ];
} BoardBuffer;

enum RendererTextureType
{
  RendererTextureType_None = 0,
  RendererTextureType_ScreenView = 1,
  RendererTextureType_Board = 2,
};

typedef struct VkTextureView
{
  int id = 0;
  bool ready = false;
  RendererTextureType type{};
  VmaAllocation allocation{};
  VulkanTexture texture{};
  LynxScreenBuffer screenBuffer{};
  VmaAllocationInfo allocationInfo{};
  AllocatedBuffer buffer{};
  VkCommandBuffer commandBuffer{};
  VkDescriptorSet descriptorSet{};
  VkPipelineLayout pipelineLayout{};
  VkPipeline pipeline{};
  int groupXDiv = 16;
  int groupYDiv = 6;
  union
  {
    struct VkScreenView
    {
      uint16_t baseAddress{};
    } screenview;
    struct VkBoard
    {
      int columns = 20;
      int rows = 10;
      const char* content{};
    } board;
  } data;

} VktextureView;



class VulkanRenderer : public IRenderer
{
public:
  VulkanRenderer();
  ~VulkanRenderer() override;
  void initialize() override;
  int64_t render( Manager&manager, UI &ui ) override;
  void terminate() override;
  bool shouldClose() override;
  void setTitle( std::string title ) override;
  std::shared_ptr<IVideoSink> getVideoSink() override;
  void registerFileDropCallback( std::function<void( std::filesystem::path )> callback ) override;
  void registerKeyEventCallback( std::function<void( int, bool )> callback ) override;
  void setRotation( ImageProperties::Rotation rotation ) override;

private:
#if defined( VKB_DEBUG )
  VkDebugUtilsMessengerEXT debugUtilsMessenger{ VK_NULL_HANDLE };
  VkDebugReportCallbackEXT debugReportCallback{ VK_NULL_HANDLE };
#endif

  void setupVulkan( const char **extensions, uint32_t extensions_count );
  void setupVulkanWindow( ImGui_ImplVulkanH_Window *wd, VkSurfaceKHR surface, int width, int height );
  void cleanupVulkanWindow();
  void cleanupVulkan();

  VkCommandBuffer createCommandBuffer( VkCommandBufferLevel level, VkCommandPool pool, bool begin );
  void flushCommandBuffer( VkCommandBuffer commandBuffer, VkQueue queue, VkCommandPool pool, bool free );
  void setImageLayout( VkCommandBuffer cmdbuffer, VkImage image, VkImageAspectFlags aspectMask, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT );
  
  void prepareCompute();
  void destroyCompute();

  void prepareViewCompute( VkTextureView& view, size_t bufferSize, VkPipelineShaderStageCreateInfo& shader, VkDescriptorSetLayout& descLayout );
  void destroyViewCompute( VkTextureView& view );

  void destroyViewTexture( VkTextureView& view );
    
  void buildComputeCommandBuffer( VkTextureView& view );
  virtual ImTextureID getTextureID( int viewId ) override;
  
  VkPipelineShaderStageCreateInfo loadShader( std::string fileName, VkShaderStageFlagBits stage );

  void frameRender( ImGui_ImplVulkanH_Window *wd, ImDrawData *draw_data );
  void framePresent( ImGui_ImplVulkanH_Window *wd );
  void renderImGui( UI &ui );
  ImVec2 getDimensions() override;

  virtual bool deleteView( int view ) override;
  void prepareViewTexture( VkTextureView& view, VkFormat format );

  void renderScreenViews( Manager& manager );
  virtual int addScreenView( uint16_t baseAddress ) override;
  virtual void setScreenViewBaseAddress( int id, uint16_t baseAddress ) override;

  void renderBoards();
  virtual int addBoard( int width, int height, const char* content ) override;

  ImVec2 mDimensions{};

  std::function<void( std::filesystem::path )> mFileDropCallback{};
  std::function<void( int, bool )> mKeyEventCallback{};

  vkb::Device mVkbDevice;
  VkInstance mInstance = VK_NULL_HANDLE;
  VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
  VkPhysicalDeviceProperties mPhysicalDeviceProperties{};
  VkDevice mDevice = VK_NULL_HANDLE;
  uint32_t mQueueFamily = (uint32_t)-1;
  VkQueue mQueue = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT mDebugReport = VK_NULL_HANDLE;
  VkSurfaceKHR mSurface = VK_NULL_HANDLE;
  VkDescriptorPool mDescriptorPool = VK_NULL_HANDLE;
  VkPipelineCache mPipelineCache = VK_NULL_HANDLE;
  VkCommandPool mCommandPool = VK_NULL_HANDLE;

  VmaAllocator mAllocator{};
  VkAllocationCallbacks *mAllocationCallbacks{};

  std::vector<VkShaderModule> mShaderModules;

  GLFWwindow *mMainWindow;
  ImGui_ImplVulkanH_Window mMainWindowData;
  int mMinImageCount = 2;
  bool mSwapChainRebuild = false;

  std::shared_ptr<VideoSink> mVideoSink{};

  std::vector<VkTextureView> mViews{};

  int mViewId = 0;
  int mFontWidth = 8;
  int mFontHeight = 16;

  ImageProperties::Rotation mRotation{};

  struct Compute
  {
    VkQueue queue;
    uint32_t queueFamily;
    VkCommandPool commandPool;
    VkSemaphore semaphore;
    VkDescriptorSetLayout screenViewDescriptorSetLayout;
    VkDescriptorSetLayout boardDescriptorSetLayout;
    VkPipelineShaderStageCreateInfo screenViewShader;
    VkPipelineShaderStageCreateInfo boardShader;
  } mCompute;
};
