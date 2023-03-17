#include "pch.hpp"
#include "VulkanRenderer.hpp"
#include "Manager.hpp"
#include <vulkan/vulkan.h>
#include "fonts.hpp"

#if defined( VKB_DEBUG )

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData )
{
  if ( messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT )
  {
    L_ERROR << messageType << ": " << pCallbackData->pMessage;
  }
  else if ( messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT )
  {
    L_WARNING << messageType << ": " << pCallbackData->pMessage;
  }
  else
  {
    L_INFO << messageType << ": " << pCallbackData->pMessage;
  }
  return VK_FALSE;
}
#endif

VulkanRenderer::VulkanRenderer() : mVideoSink{ std::make_shared<VideoSink>() }
{
}

VulkanRenderer::~VulkanRenderer()
{
  terminate();
}

std::shared_ptr<IVideoSink> VulkanRenderer::getVideoSink()
{
  return mVideoSink;
}

void VulkanRenderer::setTitle( std::string title )
{
  glfwSetWindowTitle( mMainWindow, title.c_str() );
}

void VulkanRenderer::setupVulkan( const char **extensions, uint32_t extensions_count )
{
  vkb::InstanceBuilder builder;

  auto inst_ret = builder.set_app_name( FELIX_NAME )
                    .request_validation_layers( true )
#if defined( VKB_DEBUG )
                    .enable_extension( VK_EXT_DEBUG_UTILS_EXTENSION_NAME )
                    .use_default_debug_messenger()
                    .add_debug_messenger_type( VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT )
                    .add_debug_messenger_severity( VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT )
                    .set_debug_callback( debug_callback )
                    .add_validation_feature_enable( VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT )
                    .add_validation_feature_enable( VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT )
#endif
                    .require_api_version( 1, 0, 0 )
                    .build();

  if ( !inst_ret )
  {
    L_ERROR << "Failed to create Vulkan instance. Error: " << inst_ret.error().message();
    exit( 1 );
  }

  auto vkb_inst = inst_ret.value();

  mInstance = vkb_inst.instance;
  mDebugReport = vkb_inst.debug_messenger;

  VK_CHECK( glfwCreateWindowSurface( mInstance, mMainWindow, mAllocationCallbacks, &mSurface ) );

  vkb::PhysicalDeviceSelector selector{ vkb_inst };
  auto phys_ret = selector.set_minimum_version( 1, 0 ).set_surface( mSurface ).select();
  if ( !phys_ret )
  {
    L_ERROR << "Failed to select Vulkan Physical Device. Error: " << phys_ret.error().message();
    exit( 1 );
  }
  vkb::PhysicalDevice physicalDevice = phys_ret.value();

  vkb::DeviceBuilder deviceBuilder{ physicalDevice };
  auto device_ret = deviceBuilder.build();
  if ( !device_ret )
  {
    L_ERROR << "Failed to create Vulkan device. Error: " << device_ret.error().message() << "\n";
    exit( 1 );
  }
  mVkbDevice = device_ret.value();

  mDevice = mVkbDevice.device;
  mPhysicalDevice = physicalDevice.physical_device;

  VkPhysicalDeviceProperties props{};
  vkGetPhysicalDeviceProperties( mPhysicalDevice, &props );

  L_INFO << "Will be using Vulkan device: " << props.deviceName;

  auto qr = mVkbDevice.get_queue( vkb::QueueType::graphics );
  if ( !qr )
  {
    L_ERROR << "Failed to get graphics queue. Error: " << qr.error().message() << "\n";
    exit( 1 );
  }
  mQueue = qr.value();
  mQueueFamily = mVkbDevice.get_queue_index( vkb::QueueType::graphics ).value();

  auto qrc = mVkbDevice.get_queue( vkb::QueueType::compute );
  if ( !qrc )
  {
    L_DEBUG << "Failed to get compute queue. Error: " << qrc.error().message() << "\n";
    mCompute.queue = mQueue;
    mCompute.queueFamily = mQueueFamily;
  }
  else
  {
    mCompute.queue = qrc.value();
    mCompute.queueFamily = mVkbDevice.get_queue_index( vkb::QueueType::compute ).value();
  }

  VmaAllocatorCreateInfo allocatorInfo = {};
  allocatorInfo.physicalDevice = mPhysicalDevice;
  allocatorInfo.device = mDevice;
  allocatorInfo.instance = mInstance;
  vmaCreateAllocator( &allocatorInfo, &mAllocator );

  vkGetPhysicalDeviceProperties( mPhysicalDevice, &mPhysicalDeviceProperties );

  {
    VkDescriptorPoolSize pool_sizes[] = { { VK_DESCRIPTOR_TYPE_SAMPLER, 10 },
                                          { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 },
                                          { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 10 },
                                          { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 10 },
                                          { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 10 },
                                          { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 10 },
                                          { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
                                          { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10 },
                                          { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10 },
                                          { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 10 },
                                          { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 10 } };
    auto pool_info = vkinit::descriptor_pool_create_info( VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT, 1000 * IM_ARRAYSIZE( pool_sizes ), (uint32_t)IM_ARRAYSIZE( pool_sizes ), pool_sizes );
    VK_CHECK( vkCreateDescriptorPool( mDevice, &pool_info, mAllocationCallbacks, &mDescriptorPool ) );
  }

  VkCommandPoolCreateInfo cmdPoolInfo = vkinit::command_pool_create_info( mQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT );
  VK_CHECK( vkCreateCommandPool( mDevice, &cmdPoolInfo, nullptr, &mCommandPool ) );

  mCompute.screenViewShader = loadShader( "shaders/lynx_render.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT );
  mCompute.boardShader = loadShader( "shaders/board_render.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT );

  prepareCompute();
}

void VulkanRenderer::setupVulkanWindow( ImGui_ImplVulkanH_Window *wd, VkSurfaceKHR surface, int width, int height )
{
  wd->Surface = surface;

  VkBool32 res;
  vkGetPhysicalDeviceSurfaceSupportKHR( mPhysicalDevice, mQueueFamily, wd->Surface, &res );
  if ( res != VK_TRUE )
  {
    L_ERROR << "Error no WSI support on physical device 0";
    exit( -1 );
  }

  const VkFormat requestSurfaceImageFormat[] = { VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM };
  const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
  wd->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat( mPhysicalDevice, wd->Surface, requestSurfaceImageFormat, (size_t)IM_ARRAYSIZE( requestSurfaceImageFormat ), requestSurfaceColorSpace );

  VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_FIFO_KHR };
  wd->PresentMode = ImGui_ImplVulkanH_SelectPresentMode( mPhysicalDevice, wd->Surface, &present_modes[0], IM_ARRAYSIZE( present_modes ) );

  IM_ASSERT( mMinImageCount >= 2 );
  ImGui_ImplVulkanH_CreateOrResizeWindow( mInstance, mPhysicalDevice, mDevice, wd, mQueueFamily, mAllocationCallbacks, width, height, mMinImageCount );
}

void VulkanRenderer::cleanupVulkan()
{
  vkDeviceWaitIdle( mDevice );
  vkDestroySurfaceKHR( mInstance, mSurface, nullptr );
  vkDestroyDevice( mDevice, nullptr );
  vkb::destroy_debug_utils_messenger( mInstance, mDebugReport );
  vkDestroyInstance( mInstance, nullptr );
}

void VulkanRenderer::cleanupVulkanWindow()
{
  ImGui_ImplVulkanH_DestroyWindow( mInstance, mDevice, &mMainWindowData, mAllocationCallbacks );
}

void VulkanRenderer::frameRender( ImGui_ImplVulkanH_Window *wd, ImDrawData *draw_data )
{
  if ( mSwapChainRebuild )
  {
    return;
  }
  VkSemaphore image_acquired_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
  VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
  VkResult err = vkAcquireNextImageKHR( mDevice, wd->Swapchain, UINT64_MAX, image_acquired_semaphore, VK_NULL_HANDLE, &wd->FrameIndex );
  if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
  {
      mSwapChainRebuild = true;
      return;
  }

  ImGui_ImplVulkanH_Frame *fd = &wd->Frames[wd->FrameIndex];
  VK_CHECK( vkWaitForFences( mDevice, 1, &fd->Fence, VK_TRUE, UINT64_MAX ) );
  VK_CHECK( vkResetFences( mDevice, 1, &fd->Fence ) );

  VK_CHECK( vkResetCommandPool( mDevice, fd->CommandPool, 0 ) );

  auto cmdinfo = vkinit::command_buffer_begin_info( VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT );
  VK_CHECK( vkBeginCommandBuffer( fd->CommandBuffer, &cmdinfo ) );

  auto renderpassinfo = vkinit::renderpass_begin_info( wd->RenderPass, { (unsigned int)wd->Width, (unsigned int)wd->Height }, fd->Framebuffer );
  renderpassinfo.pClearValues = &wd->ClearValue;
  vkCmdBeginRenderPass( fd->CommandBuffer, &renderpassinfo, VK_SUBPASS_CONTENTS_INLINE );

  ImGui_ImplVulkan_RenderDrawData( draw_data, fd->CommandBuffer );

  vkCmdEndRenderPass( fd->CommandBuffer );

  VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

  std::vector<VkCommandBuffer> cmdbuffers{};

  for( auto& view : mViews )
  {
    if( !view.ready )
    {
      continue;
    }
    cmdbuffers.push_back( view.commandBuffer );
  }

  VkSubmitInfo computeSubmitInfo = vkinit::submit_info( cmdbuffers.data() );
  computeSubmitInfo.commandBufferCount = cmdbuffers.size();
  computeSubmitInfo.pWaitSemaphores = &image_acquired_semaphore;
  computeSubmitInfo.waitSemaphoreCount = 1;
  computeSubmitInfo.pWaitDstStageMask = &waitStageMask;
  computeSubmitInfo.signalSemaphoreCount = 1;
  computeSubmitInfo.pSignalSemaphores = &mCompute.semaphore;
  VK_CHECK( vkQueueSubmit( mCompute.queue, 1, &computeSubmitInfo, VK_NULL_HANDLE ) );
  VK_CHECK( vkQueueWaitIdle( mCompute.queue ) );

  VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  auto submitinfo = vkinit::submit_info( &fd->CommandBuffer );
  submitinfo.waitSemaphoreCount = 1;
  submitinfo.pWaitSemaphores = &mCompute.semaphore;
  submitinfo.pWaitDstStageMask = &wait_stage;
  submitinfo.signalSemaphoreCount = 1;
  submitinfo.pSignalSemaphores = &render_complete_semaphore;

  VK_CHECK( vkEndCommandBuffer( fd->CommandBuffer ) );
  VK_CHECK( vkQueueSubmit( mQueue, 1, &submitinfo, fd->Fence ) );
}

void VulkanRenderer::framePresent( ImGui_ImplVulkanH_Window *wd )
{
  if ( mSwapChainRebuild )
  {
    return;
  }
  VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
  auto info = vkinit::present_info();
  info.waitSemaphoreCount = 1;
  info.pWaitSemaphores = &render_complete_semaphore;
  info.swapchainCount = 1;
  info.pSwapchains = &wd->Swapchain;
  info.pImageIndices = &wd->FrameIndex;
  VkResult err = vkQueuePresentKHR( mQueue, &info );
  if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
  {
      mSwapChainRebuild = true;
      return;
  }
  wd->SemaphoreIndex = ( wd->SemaphoreIndex + 1 ) % wd->ImageCount;
}

ImVec2 VulkanRenderer::getDimensions()
{
  return mDimensions;
}

void VulkanRenderer::initialize()
{
  glfwSetErrorCallback( []( int error, const char *description ) { L_ERROR << "GLFW Error " << error << ": " << description; } );

  if ( !glfwInit() )
  {
    return;
  }

  glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
  mMainWindow = glfwCreateWindow( 1280, 720, "VulkanRenderer", NULL, NULL );
  if ( !glfwVulkanSupported() )
  {
    L_ERROR << "GLFW: Vulkan Not Supported";
    return;
  }

  glfwSetWindowUserPointer( mMainWindow, this );

  glfwSetDropCallback( mMainWindow, []( GLFWwindow *window, int count, const char **paths ) 
  {
    if ( count != 1 )
    {
      return;
    }

    auto self = static_cast<VulkanRenderer *>( glfwGetWindowUserPointer( window ) );
    if ( !self->mFileDropCallback )
    {
      return;
    }

    std::filesystem::path path( paths[0] );

    L_INFO << "VulkanRenderer - dropped '" << path << "'";

    if ( !std::filesystem::exists( path ) )
    {
      L_INFO << "VulkanRenderer - '" << path << "' doesn't exist...";
      return;
    }

    self->mFileDropCallback( path );
  } );

  glfwSetFramebufferSizeCallback( mMainWindow, []( GLFWwindow *window, int width, int height )
  {
    auto self = static_cast<VulkanRenderer *>( glfwGetWindowUserPointer( window ) );
    self->mDimensions = { static_cast<float>( width ), static_cast<float>( height )};
  });

  glfwSetKeyCallback( mMainWindow, []( GLFWwindow *window, int key, int scancode, int action, int mods ) {
    auto self = static_cast<VulkanRenderer *>( glfwGetWindowUserPointer( window ) );
    if ( !self->mKeyEventCallback )
    {
      return;
    }

    switch ( action )
    {
    case GLFW_PRESS:
      self->mKeyEventCallback( key, true );
      break;
    case GLFW_RELEASE:
      self->mKeyEventCallback( key, false );
      break;
    default:
      break;
    }
  } );

  uint32_t extensions_count = 0;
  const char **extensions = glfwGetRequiredInstanceExtensions( &extensions_count );
  setupVulkan( extensions, extensions_count );

  int w, h;
  glfwGetFramebufferSize( mMainWindow, &w, &h );

  mDimensions = { static_cast<float>( w ), static_cast<float>( h )};
  
  ImGui_ImplVulkanH_Window *wd = &mMainWindowData;
  setupVulkanWindow( wd, mSurface, w, h );

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

  ImGui::StyleColorsDark();
  // ImGui::StyleColorsLight();

  ImGui_ImplGlfw_InitForVulkan( mMainWindow, true );
  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance = mInstance;
  init_info.PhysicalDevice = mPhysicalDevice;
  init_info.Device = mDevice;
  init_info.QueueFamily = mQueueFamily;
  init_info.Queue = mQueue;
  init_info.PipelineCache = mPipelineCache;
  init_info.DescriptorPool = mDescriptorPool;
  init_info.Subpass = 0;
  init_info.MinImageCount = mMinImageCount;
  init_info.ImageCount = wd->ImageCount;
  init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  init_info.Allocator = mAllocationCallbacks;
  ImGui_ImplVulkan_Init( &init_info, wd->RenderPass );

  VkCommandPool command_pool = wd->Frames[wd->FrameIndex].CommandPool;
  VkCommandBuffer command_buffer = wd->Frames[wd->FrameIndex].CommandBuffer;

  VK_CHECK( vkResetCommandPool( mDevice, command_pool, 0 ) );
  ;
  VkCommandBufferBeginInfo begin_info = {};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  VK_CHECK( vkBeginCommandBuffer( command_buffer, &begin_info ) );

  ImGui_ImplVulkan_CreateFontsTexture( command_buffer );

  VkSubmitInfo end_info = {};
  end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  end_info.commandBufferCount = 1;
  end_info.pCommandBuffers = &command_buffer;
  VK_CHECK( vkEndCommandBuffer( command_buffer ) );
  VK_CHECK( vkQueueSubmit( mQueue, 1, &end_info, VK_NULL_HANDLE ) );

  VK_CHECK( vkDeviceWaitIdle( mDevice ) );
  ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void VulkanRenderer::terminate()
{
  VK_CHECK( vkDeviceWaitIdle( mDevice ) );

  for( auto& view : mViews )
  {
    destroyViewTexture( view );
  }

  destroyCompute(); 

  for ( auto &shaderModule : mShaderModules )
  {
    vkDestroyShaderModule( mDevice, shaderModule, nullptr );
  }
  
  vmaDestroyAllocator( mAllocator ); 

  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  cleanupVulkanWindow();
  cleanupVulkan();

  glfwDestroyWindow( mMainWindow );
  glfwTerminate();
}

int64_t VulkanRenderer::render( Manager& manager, UI &ui )
{
  renderImGui( ui );
  renderScreenViews( manager );
  renderBoards();
  return 1;
}

VkPipelineShaderStageCreateInfo VulkanRenderer::loadShader( std::string fileName, VkShaderStageFlagBits stage )
{
  std::ifstream is( fileName, std::ios::binary | std::ios::in | std::ios::ate );

  if ( !is.is_open() )
  {
    L_ERROR << "Error: Could not open shader file \"" << fileName << "\"";
    abort();
  }

  size_t size = is.tellg();
  is.seekg( 0, std::ios::beg );
  char *shaderCode = new char[size];
  is.read( shaderCode, size );
  is.close();

  assert( size > 0 );

  VkShaderModule shaderModule;
  VkShaderModuleCreateInfo moduleCreateInfo{};
  moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  moduleCreateInfo.codeSize = size;
  moduleCreateInfo.pCode = (uint32_t *)shaderCode;

  VK_CHECK( vkCreateShaderModule( mDevice, &moduleCreateInfo, NULL, &shaderModule ) );

  delete[] shaderCode;

  auto shaderStage = vkinit::pipeline_shader_stage_create_info( stage, shaderModule );
  assert( shaderStage.module != VK_NULL_HANDLE );

  mShaderModules.push_back( shaderStage.module );

  return shaderStage;
}

void VulkanRenderer::buildComputeCommandBuffer( VkTextureView& view )
{
  vkQueueWaitIdle( mCompute.queue );

  auto cmdBufAllocateInfo = vkinit::command_buffer_allocate_info( mCompute.commandPool, 1, VK_COMMAND_BUFFER_LEVEL_PRIMARY );
  VK_CHECK( vkAllocateCommandBuffers( mDevice, &cmdBufAllocateInfo, &view.commandBuffer ) );

  auto cmdBufInfo = vkinit::command_buffer_begin_info();

  VK_CHECK( vkBeginCommandBuffer( view.commandBuffer, &cmdBufInfo ) );

  vkCmdBindPipeline( view.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, view.pipeline );
  vkCmdBindDescriptorSets( view.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, view.pipelineLayout, 0, 1, &view.descriptorSet, 0, 0 );

  vkCmdDispatch( view.commandBuffer, view.texture.mWidth / view.groupXDiv, view.texture.mHeight / view.groupYDiv, 1 );

  vkEndCommandBuffer( view.commandBuffer );

  view.ready = true;
}

void VulkanRenderer::prepareViewCompute( VkTextureView& view, size_t bufferSize, VkPipelineShaderStageCreateInfo& shader, VkDescriptorSetLayout& descLayout )
{
  auto pPipelineLayoutCreateInfo = vkinit::pipeline_layout_create_info();
  pPipelineLayoutCreateInfo.setLayoutCount = 1;
  pPipelineLayoutCreateInfo.pSetLayouts = &descLayout;
  VK_CHECK( vkCreatePipelineLayout( mDevice, &pPipelineLayoutCreateInfo, nullptr, &view.pipelineLayout ) );

  auto allocInfo = vkinit::descriptorset_allocate_info( mDescriptorPool, &descLayout, 1 );
  VK_CHECK( vkAllocateDescriptorSets( mDevice, &allocInfo, &view.descriptorSet ) );
  
  VkBufferCreateInfo screenbufferInfo{};
  screenbufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  screenbufferInfo.pNext = nullptr;
  screenbufferInfo.size = bufferSize;
  screenbufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

  VmaAllocationCreateInfo screenvmaallocInfo{};
  screenvmaallocInfo.usage = VMA_MEMORY_USAGE_AUTO;
  screenvmaallocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

  VK_CHECK( vmaCreateBuffer( mAllocator, &screenbufferInfo, &screenvmaallocInfo, &view.buffer._buffer, &view.buffer._allocation, &view.allocationInfo ) );
  
  VkDescriptorBufferInfo screenbinfo;
  screenbinfo.buffer = view.buffer._buffer;
  screenbinfo.offset = 0;
  screenbinfo.range = bufferSize;

  auto descBuff = vkinit::write_descriptor_buffer( VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, view.descriptorSet, &screenbinfo, 0 );
  auto descImages = vkinit::write_descriptor_image( VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, view.descriptorSet, &view.texture.mDescriptor, 1 );

  std::vector<VkWriteDescriptorSet> computeWriteDescriptorSets { descBuff, descImages };

  vkUpdateDescriptorSets( mDevice, computeWriteDescriptorSets.size(), computeWriteDescriptorSets.data(), 0, NULL );

  auto computePipelineCreateInfo = vkinit::computepipeline_create_info( view.pipelineLayout, 0 );

  computePipelineCreateInfo.stage = shader;
  
  VK_CHECK( vkCreateComputePipelines( mDevice, mPipelineCache, 1, &computePipelineCreateInfo, nullptr, &view.pipeline ) );

  buildComputeCommandBuffer( view );
}

void VulkanRenderer::prepareCompute()
{
  std::vector<VkDescriptorSetLayoutBinding> screenViewSetLayoutBindings = 
  {
    vkinit::descriptorset_layout_binding( VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 0 ),
    vkinit::descriptorset_layout_binding( VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 1 ),
  };

  auto screenViewDescriptorLayout = vkinit::descriptorset_layout_create_info( screenViewSetLayoutBindings );
  VK_CHECK( vkCreateDescriptorSetLayout( mDevice, &screenViewDescriptorLayout, nullptr, &mCompute.screenViewDescriptorSetLayout ) );

  std::vector<VkDescriptorSetLayoutBinding> boardSetLayoutBindings = 
  {
    vkinit::descriptorset_layout_binding( VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 0 ),
    vkinit::descriptorset_layout_binding( VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 1 ),
  };

  auto boardDescriptorLayout = vkinit::descriptorset_layout_create_info( boardSetLayoutBindings );
  VK_CHECK( vkCreateDescriptorSetLayout( mDevice, &boardDescriptorLayout, nullptr, &mCompute.boardDescriptorSetLayout ) );

  VkCommandPoolCreateInfo cmdPoolInfo = {};
  cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cmdPoolInfo.queueFamilyIndex = mCompute.queueFamily;
  cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  VK_CHECK( vkCreateCommandPool( mDevice, &cmdPoolInfo, nullptr, &mCompute.commandPool ) );

  VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();
  VK_CHECK( vkCreateSemaphore( mDevice, &semaphoreCreateInfo, nullptr, &mCompute.semaphore ) );
}

void VulkanRenderer::destroyViewCompute( VkTextureView& view )
{
  view.ready = false;
  vkFreeCommandBuffers( mDevice, mCompute.commandPool, 1, &view.commandBuffer );
  vmaDestroyBuffer( mAllocator, view.buffer._buffer, view.buffer._allocation );
  vkDestroyPipelineLayout( mDevice, view.pipelineLayout, nullptr );
  vkFreeDescriptorSets( mDevice, mDescriptorPool, 1, &view.descriptorSet );
}

void VulkanRenderer::destroyCompute()
{
  vkDestroySemaphore( mDevice, mCompute.semaphore, nullptr );
  vkDestroyDescriptorSetLayout( mDevice, mCompute.screenViewDescriptorSetLayout, nullptr );
  vkDestroyDescriptorSetLayout( mDevice, mCompute.boardDescriptorSetLayout, nullptr );
  vkDestroyCommandPool( mDevice, mCompute.commandPool, nullptr );
}

VkCommandBuffer VulkanRenderer::createCommandBuffer( VkCommandBufferLevel level, VkCommandPool pool, bool begin )
{
  auto cmdBufAllocateInfo = vkinit::command_buffer_allocate_info( pool, 1, level );

  VkCommandBuffer cmdBuffer;
  VK_CHECK( vkAllocateCommandBuffers( mDevice, &cmdBufAllocateInfo, &cmdBuffer ) );
  if ( begin )
  {
    auto cmdBufInfo = vkinit::command_buffer_begin_info();
    VK_CHECK( vkBeginCommandBuffer( cmdBuffer, &cmdBufInfo ) );
  }
  return cmdBuffer;
}

void VulkanRenderer::flushCommandBuffer( VkCommandBuffer commandBuffer, VkQueue queue, VkCommandPool pool, bool free )
{
  if ( commandBuffer == VK_NULL_HANDLE )
  {
    return;
  }

  VK_CHECK( vkEndCommandBuffer( commandBuffer ) );

  VkSubmitInfo submitInfo = vkinit::submit_info( &commandBuffer );
  VkFenceCreateInfo fenceInfo = vkinit::fence_create_info();
  VkFence fence;
  VK_CHECK( vkCreateFence( mDevice, &fenceInfo, nullptr, &fence ) );
  VK_CHECK( vkQueueSubmit( queue, 1, &submitInfo, fence ) );
  VK_CHECK( vkWaitForFences( mDevice, 1, &fence, VK_TRUE, 1000000000 ) );
  vkDestroyFence( mDevice, fence, nullptr );
  if ( free )
  {
    vkFreeCommandBuffers( mDevice, pool, 1, &commandBuffer );
  }
}

void VulkanRenderer::setImageLayout( VkCommandBuffer cmdbuffer, VkImage image, VkImageAspectFlags aspectMask, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask )
{
  VkImageSubresourceRange subresourceRange = {};
  subresourceRange.aspectMask = aspectMask;
  subresourceRange.baseMipLevel = 0;
  subresourceRange.levelCount = 1;
  subresourceRange.layerCount = 1;

  VkImageMemoryBarrier imageMemoryBarrier = vkinit::image_memory_barrier( oldImageLayout, newImageLayout, image );
  imageMemoryBarrier.subresourceRange = subresourceRange;

  switch ( oldImageLayout )
  {
  case VK_IMAGE_LAYOUT_UNDEFINED:
    imageMemoryBarrier.srcAccessMask = 0;
    break;
  case VK_IMAGE_LAYOUT_PREINITIALIZED:
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    break;
  case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    break;
  case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    break;
  case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    break;
  case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    break;
  case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    break;
  default:
    break;
  }

  switch ( newImageLayout )
  {
  case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    break;
  case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    break;
  case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    break;
  case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
    imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    break;
  case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
    if ( imageMemoryBarrier.srcAccessMask == 0 )
    {
      imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    }
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    break;
  default:
    break;
  }

  vkCmdPipelineBarrier( cmdbuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier );
}

bool VulkanRenderer::shouldClose()
{
  return glfwWindowShouldClose( mMainWindow );
}

void VulkanRenderer::renderImGui( UI &ui )
{
  bool show_demo_window = true;
  bool show_another_window = false;
  ImVec4 clear_color = ImVec4( 0.45f, 0.55f, 0.60f, 1.00f );

  glfwPollEvents();

  if ( mSwapChainRebuild )
  {
    int width, height;
    glfwGetFramebufferSize( mMainWindow, &width, &height );
    if ( width > 0 && height > 0 )
    {
      ImGui_ImplVulkan_SetMinImageCount( mMinImageCount );
      ImGui_ImplVulkanH_CreateOrResizeWindow( mInstance, mPhysicalDevice, mDevice, &mMainWindowData, mQueueFamily, mAllocationCallbacks, width, height, mMinImageCount );
      mMainWindowData.FrameIndex = 0;
      mSwapChainRebuild = false;
    }
  }

  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  int width, height, xpos, ypos;
  glfwGetWindowSize( mMainWindow, &width, &height );
  glfwGetWindowPos( mMainWindow, &xpos, &ypos );

  ui.drawGui( xpos, ypos, xpos + width, ypos + height );

  ImGui::Render();
  ImDrawData *draw_data = ImGui::GetDrawData();
  const bool is_minimized = ( draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f );
  if ( !is_minimized )
  {
    ImGui_ImplVulkanH_Window *wd = &mMainWindowData;
    wd->ClearValue.color.float32[0] = clear_color.x * clear_color.w;
    wd->ClearValue.color.float32[1] = clear_color.y * clear_color.w;
    wd->ClearValue.color.float32[2] = clear_color.z * clear_color.w;
    wd->ClearValue.color.float32[3] = clear_color.w;
    frameRender( wd, draw_data );
    framePresent( wd );
  }
}

void VulkanRenderer::registerFileDropCallback( std::function<void( std::filesystem::path )> callback )
{
  mFileDropCallback = std::move( callback );
}

void VulkanRenderer::registerKeyEventCallback( std::function<void( int, bool )> callback )
{
  mKeyEventCallback = std::move( callback );
}

void VulkanRenderer::setRotation( ImageProperties::Rotation rotation )
{
  mRotation = rotation;

  vkQueueWaitIdle( mCompute.queue );

  for( auto& view : mViews )
  {
    if( view.type != RendererTextureType::RendererTextureType_ScreenView )
    {
      continue;
    }

    if( mRotation == ImageProperties::Rotation::NORMAL )
    {
      view.groupXDiv = 16;
      view.groupYDiv = 6;
    }
    else
    {
      view.groupXDiv = 6;
      view.groupYDiv = 16;
    }

    destroyViewTexture( view );
    destroyViewCompute( view );

    prepareViewTexture( view, VK_FORMAT_R8G8B8A8_UNORM );
    prepareViewCompute( view, sizeof( LynxScreenBuffer ), mCompute.screenViewShader, mCompute.screenViewDescriptorSetLayout );

    LynxScreenBuffer* screenbuffer = (LynxScreenBuffer*)view.allocationInfo.pMappedData;
    screenbuffer->mRotation = mRotation;
  }
}

void VulkanRenderer::renderScreenViews( Manager& manager )
{
  auto frame = mVideoSink->pullNextFrame();

  if( !frame )
  {
    return;
  }

  for ( auto& view : mViews )
  {
    if( view.type != RendererTextureType::RendererTextureType_ScreenView || !view.ready )
    {
      continue;
    }

    LynxScreenBuffer* screen = (LynxScreenBuffer*)view.allocationInfo.pMappedData;
    const uint8_t* srcBuffer;

    if( !view.data.screenview.baseAddress )
    {
      srcBuffer = frame->data();
    }
    else
    {
      srcBuffer = manager.mInstance->debugRAM() + view.data.screenview.baseAddress;
    }

    memcpy( screen->mBuffer, srcBuffer, SCREEN_BUFFER_SIZE );
    memcpy( screen->mPalette, mVideoSink->getPalettePointer(), 32 );
  }
}

ImTextureID VulkanRenderer::getTextureID( int viewId )
{
  auto view = std::find_if( mViews.begin(), mViews.end(), [viewId](const VkTextureView &v) { return v.id == viewId; } );

  if( view == mViews.end() || !view->ready )
  {
    return nullptr;  
  }
  
  return view->texture.mDS;  
}

void VulkanRenderer::setScreenViewBaseAddress( int screenId, uint16_t baseAddress )
{
  auto found = std::find_if( mViews.begin(), mViews.end(), [screenId](const VkTextureView &v) { return v.id == screenId; } );
  found->data.screenview.baseAddress = baseAddress;
}

void VulkanRenderer::prepareViewTexture( VkTextureView& view, VkFormat format )
{
  uint32_t w = SCREEN_WIDTH;
  uint32_t h = SCREEN_HEIGHT;

  if ( mRotation != ImageProperties::Rotation::NORMAL )
  {
    w = SCREEN_HEIGHT;
    h = SCREEN_WIDTH;
  }

  VkFormatProperties formatProperties;
  vkGetPhysicalDeviceFormatProperties( mPhysicalDevice, format, &formatProperties );
  assert( formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT );

  if( view.type == RendererTextureType_ScreenView )
  {
    view.texture.mWidth = w;
    view.texture.mHeight = h;
  }
  else if( view.type == RendererTextureType_Board )
  {
    view.texture.mWidth = view.data.board.columns * mFontWidth;
    view.texture.mHeight = view.data.board.rows * mFontHeight;
  }

  auto imageCreateInfo = vkinit::image_create_info( format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, { view.texture.mWidth, view.texture.mHeight, 1 } );

  std::vector<uint32_t> queueFamilyIndices;
  auto grQueue = mQueueFamily;
  auto cpQueue = mCompute.queueFamily;
  if ( grQueue != cpQueue )
  {
    queueFamilyIndices = { grQueue, cpQueue };
    imageCreateInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
    imageCreateInfo.queueFamilyIndexCount = 2;
    imageCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();
  }

  VmaAllocationCreateInfo allocCreateInfo = {};
  allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
  allocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
  allocCreateInfo.priority = 1.0f;

  vmaCreateImage( mAllocator, &imageCreateInfo, &allocCreateInfo, &view.texture.mImage, &view.allocation, nullptr );

  VkCommandBuffer layoutCmd = createCommandBuffer( VK_COMMAND_BUFFER_LEVEL_PRIMARY, mCommandPool, true );

  view.texture.mImageLayout = VK_IMAGE_LAYOUT_GENERAL;
  setImageLayout( layoutCmd, view.texture.mImage, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, view.texture.mImageLayout );

  flushCommandBuffer( layoutCmd, mQueue, mCommandPool, true );

  VkImageViewCreateInfo viewci = vkinit::imageview_create_info( format, view.texture.mImage, 0 );
  viewci.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
  VK_CHECK( vkCreateImageView( mDevice, &viewci, nullptr, &view.texture.mView ) );

  VkSamplerCreateInfo sampler = vkinit::sampler_create_info( VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER );
  sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  sampler.mipLodBias = 0.0f;
  sampler.anisotropyEnable = VK_FALSE;
  sampler.compareOp = VK_COMPARE_OP_NEVER;
  sampler.minLod = 0.0f;
  sampler.maxLod = VK_LOD_CLAMP_NONE;
  sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
  VK_CHECK( vkCreateSampler( mDevice, &sampler, nullptr, &view.texture.mSampler ) );

  view.texture.mDescriptor.imageLayout = view.texture.mImageLayout;
  view.texture.mDescriptor.imageView = view.texture.mView;
  view.texture.mDescriptor.sampler = view.texture.mSampler;
  view.texture.mDevice = mDevice;

  view.texture.mDS = ImGui_ImplVulkan_AddTexture( view.texture.mSampler, view.texture.mView, VK_IMAGE_LAYOUT_GENERAL );
}

bool VulkanRenderer::deleteView( int viewId )
{
  vkQueueWaitIdle( mCompute.queue );

  auto view = std::find_if( mViews.begin(), mViews.end(), [viewId](const VkTextureView& b) { return b.id == viewId; } );

  if( view == mViews.end() )
  {
    return false;
  }

  destroyViewTexture( *view );
  destroyViewCompute( *view );

  mSwapChainRebuild = true;

  std::erase_if( mViews, [viewId](const VkTextureView& v) { return v.id == viewId; } );
  
  return true;
}

void VulkanRenderer::destroyViewTexture( VkTextureView& view )
{
  view.ready = false;
  if( view.texture.mDS )
  {
    ImGui_ImplVulkan_RemoveTexture( view.texture.mDS );
    view.texture.mDS = nullptr;
  }
  vkDestroyImageView( mDevice, view.texture.mView, nullptr );
  vmaDestroyImage( mAllocator, view.texture.mImage, view.allocation );
}

int VulkanRenderer::addScreenView( uint16_t baseAddress )
{
  vkQueueWaitIdle( mCompute.queue );

  VktextureView v{};
  v.type = RendererTextureType_ScreenView;
  v.id = mViewId++;
  v.data.screenview.baseAddress = baseAddress;
  if( mRotation != ImageProperties::Rotation::NORMAL )
  {
    v.groupXDiv = 6;
    v.groupYDiv = 16;
  }

  prepareViewTexture( v, VK_FORMAT_R8G8B8A8_UNORM );
  prepareViewCompute( v, sizeof( LynxScreenBuffer ), mCompute.screenViewShader, mCompute.screenViewDescriptorSetLayout );  

  LynxScreenBuffer* screenbuffer = (LynxScreenBuffer*)v.allocationInfo.pMappedData;
  screenbuffer->mRotation = mRotation;

  mViews.push_back( v );

  return v.id;
}

void VulkanRenderer::renderBoards()
{
  for ( auto& board : mViews )
  {
    if( board.type != RendererTextureType_Board || !board.ready )
    {
      continue;
    }

    BoardBuffer* boardBuffer = (BoardBuffer*)board.allocationInfo.pMappedData;
    memcpy( boardBuffer->content, board.data.board.content, board.data.board.columns * board.data.board.rows );
  }
}

int VulkanRenderer::addBoard( int width, int height, const char* content )
{
  vkQueueWaitIdle( mCompute.queue );

  VkTextureView b{};
  b.type = RendererTextureType_Board;
  b.id = mViewId++;
  b.data.board.columns = width;
  b.data.board.rows = height;
  b.data.board.content = content;

  prepareViewTexture( b, VK_FORMAT_R8G8B8A8_UNORM );
  prepareViewCompute( b, sizeof( BoardBuffer ), mCompute.boardShader, mCompute.boardDescriptorSetLayout );  

  BoardBuffer* boardBuffer = (BoardBuffer*)b.allocationInfo.pMappedData;
  memcpy( boardBuffer->font, font_SWISSBX2, sizeof( font_SWISSBX2 ) );

  mViews.push_back( b );
  return b.id;
}