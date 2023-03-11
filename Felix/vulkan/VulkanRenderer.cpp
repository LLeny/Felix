#include "pch.hpp"
#include "VulkanRenderer.hpp"
#include <vulkan/vulkan.h>

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
    L_INFO << "Failed to get compute queue. Error: " << qrc.error().message() << "\n";
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

  mMainDeletionQueue.push_function( [&]() { vmaDestroyAllocator( mAllocator ); } );

  mMainDeletionQueue.push_function( [&]() {
    for ( auto &shaderModule : mShaderModules )
    {
      vkDestroyShaderModule( mDevice, shaderModule, nullptr );
    }
  } );

  mMainDeletionQueue.push_function( [&]() { mMainScreenTexture.destroy(); } );
  mMainDeletionQueue.push_function( [&]() { destroyCompute(); } );
}

void VulkanRenderer::setupVulkanWindow( ImGui_ImplVulkanH_Window *wd, VkSurfaceKHR surface, int width, int height )
{
  wd->Surface = surface;

  VkBool32 res;
  vkGetPhysicalDeviceSurfaceSupportKHR( mPhysicalDevice, mQueueFamily, wd->Surface, &res );
  if ( res != VK_TRUE )
  {
    fprintf( stderr, "Error no WSI support on physical device 0\n" );
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

  mMainDeletionQueue.flush();

  vkDestroySurfaceKHR( mInstance, mSurface, nullptr );
  vkDestroyDevice( mDevice, nullptr );
  vkb::destroy_debug_utils_messenger( mInstance, mDebugReport );
  vkDestroyInstance( mInstance, nullptr );
}

void VulkanRenderer::cleanupVulkanWindow()
{
  ImGui_ImplVulkanH_DestroyWindow( mInstance, mDevice, &mMainWindowData, mAllocationCallbacks );
}

ImTextureID VulkanRenderer::getMainScreenTextureID()
{
  return mMainScreenTexture.mDS;
}

void VulkanRenderer::frameRender( ImGui_ImplVulkanH_Window *wd, ImDrawData *draw_data )
{
  VkSemaphore image_acquired_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
  VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
  VK_CHECK( vkAcquireNextImageKHR( mDevice, wd->Swapchain, UINT64_MAX, image_acquired_semaphore, VK_NULL_HANDLE, &wd->FrameIndex ) );

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

  VkSubmitInfo computeSubmitInfo = vkinit::submit_info( &mCompute.commandBuffer );
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
  VK_CHECK( vkQueuePresentKHR( mQueue, &info ) );
  wd->SemaphoreIndex = ( wd->SemaphoreIndex + 1 ) % wd->ImageCount;
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

  glfwSetDropCallback( mMainWindow, []( GLFWwindow *window, int count, const char **paths ) {
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
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  cleanupVulkanWindow();
  cleanupVulkan();

  glfwDestroyWindow( mMainWindow );
  glfwTerminate();
}

int64_t VulkanRenderer::render( UI &ui )
{
  renderImGui( ui );
  renderMainScreen();
  return 1;
}

void VulkanRenderer::renderMainScreen()
{
  if ( auto frame = mVideoSink->pullNextFrame() )
  {
    memcpy( ( (LynxScreen *)mLynxScreenAllocationInfo.pMappedData )->mBuffer, frame->data(), SCREEN_BUFFER_SIZE );
    memcpy( ( (LynxScreen *)mLynxScreenAllocationInfo.pMappedData )->mPalette, mVideoSink->getPalettePointer(), 32 );
    ( (LynxScreen *)mLynxScreenAllocationInfo.pMappedData )->mRotation = mRotation;
  }
}

VkPipelineShaderStageCreateInfo VulkanRenderer::loadShader( std::string fileName, VkShaderStageFlagBits stage )
{
  std::ifstream is( fileName, std::ios::binary | std::ios::in | std::ios::ate );

  if ( is.is_open() )
  {
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
  else
  {
    L_ERROR << "Error: Could not open shader file \"" << fileName << "\"";
    return {};
  }
}

void VulkanRenderer::buildComputeCommandBuffer()
{
  vkQueueWaitIdle( mCompute.queue );

  auto cmdBufInfo = vkinit::command_buffer_begin_info();

  VK_CHECK( vkBeginCommandBuffer( mCompute.commandBuffer, &cmdBufInfo ) );

  vkCmdBindPipeline( mCompute.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mCompute.pipelines[mCompute.pipelineIndex] );
  vkCmdBindDescriptorSets( mCompute.commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mCompute.pipelineLayout, 0, 1, &mCompute.descriptorSet, 0, 0 );

  vkCmdDispatch( mCompute.commandBuffer, SCREEN_WIDTH / 16, SCREEN_HEIGHT / 6, 1 );

  vkEndCommandBuffer( mCompute.commandBuffer );
}

void VulkanRenderer::destroyComputeCommandBuffer()
{
  vkFreeCommandBuffers( mDevice, mCompute.commandPool, 1, &mCompute.commandBuffer );
}

void VulkanRenderer::prepareCompute()
{
  std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
    vkinit::descriptorset_layout_binding( VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 0 ),
    vkinit::descriptorset_layout_binding( VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT, 1 ),
    vkinit::descriptorset_layout_binding( VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT, 2 ),
  };

  auto descriptorLayout = vkinit::descriptorset_layout_create_info( setLayoutBindings );
  VK_CHECK( vkCreateDescriptorSetLayout( mDevice, &descriptorLayout, nullptr, &mCompute.descriptorSetLayout ) );

  auto pPipelineLayoutCreateInfo = vkinit::pipeline_layout_create_info();
  pPipelineLayoutCreateInfo.setLayoutCount = 1;
  pPipelineLayoutCreateInfo.pSetLayouts = &mCompute.descriptorSetLayout;
  VK_CHECK( vkCreatePipelineLayout( mDevice, &pPipelineLayoutCreateInfo, nullptr, &mCompute.pipelineLayout ) );

  auto allocInfo = vkinit::descriptorset_allocate_info( mDescriptorPool, &mCompute.descriptorSetLayout, 1 );
  VK_CHECK( vkAllocateDescriptorSets( mDevice, &allocInfo, &mCompute.descriptorSet ) );

  VkBufferCreateInfo screenbufferInfo = {};
  screenbufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  screenbufferInfo.pNext = nullptr;
  screenbufferInfo.size = sizeof( LynxScreen );
  screenbufferInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  VmaAllocationCreateInfo screenvmaallocInfo = {};
  screenvmaallocInfo.usage = VMA_MEMORY_USAGE_AUTO;
  screenvmaallocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
  VK_CHECK( vmaCreateBuffer( mAllocator, &screenbufferInfo, &screenvmaallocInfo, &mMainScreenBuffer._buffer, &mMainScreenBuffer._allocation, &mLynxScreenAllocationInfo ) );
  VkDescriptorBufferInfo screenbinfo;
  screenbinfo.buffer = mMainScreenBuffer._buffer;
  screenbinfo.offset = 0;
  screenbinfo.range = sizeof( LynxScreen );

  std::vector<VkWriteDescriptorSet> computeWriteDescriptorSets = { vkinit::write_descriptor_buffer( VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, mCompute.descriptorSet, &screenbinfo, 0 ), vkinit::write_descriptor_image( VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, mCompute.descriptorSet, &mMainScreenTexture.mDescriptor, 1 ) };
  vkUpdateDescriptorSets( mDevice, computeWriteDescriptorSets.size(), computeWriteDescriptorSets.data(), 0, NULL );

  auto computePipelineCreateInfo = vkinit::computepipeline_create_info( mCompute.pipelineLayout, 0 );

  std::string shaderNames[]{ "lynx_render" };
  for ( auto &shaderName : shaderNames )
  {
    std::string fileName = "shaders/" + shaderName + ".comp.spv";
    computePipelineCreateInfo.stage = loadShader( fileName, VK_SHADER_STAGE_COMPUTE_BIT );
    VkPipeline pipeline;
    VK_CHECK( vkCreateComputePipelines( mDevice, mPipelineCache, 1, &computePipelineCreateInfo, nullptr, &pipeline ) );
    mCompute.pipelines.push_back( pipeline );
  }

  VkCommandPoolCreateInfo cmdPoolInfo = {};
  cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cmdPoolInfo.queueFamilyIndex = mCompute.queueFamily;
  cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  VK_CHECK( vkCreateCommandPool( mDevice, &cmdPoolInfo, nullptr, &mCompute.commandPool ) );

  auto cmdBufAllocateInfo = vkinit::command_buffer_allocate_info( mCompute.commandPool, 1, VK_COMMAND_BUFFER_LEVEL_PRIMARY );
  VK_CHECK( vkAllocateCommandBuffers( mDevice, &cmdBufAllocateInfo, &mCompute.commandBuffer ) );

  VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();
  VK_CHECK( vkCreateSemaphore( mDevice, &semaphoreCreateInfo, nullptr, &mCompute.semaphore ) );

  buildComputeCommandBuffer();
}

void VulkanRenderer::destroyCompute()
{
  destroyComputeCommandBuffer();

  vkDestroySemaphore( mDevice, mCompute.semaphore, nullptr );
  vmaDestroyBuffer( mAllocator, mMainScreenBuffer._buffer, mMainScreenBuffer._allocation );
  vkFreeDescriptorSets( mDevice, mDescriptorPool, 1, &mCompute.descriptorSet );
  vkDestroyPipelineLayout( mDevice, mCompute.pipelineLayout, nullptr );
  vkDestroyDescriptorSetLayout( mDevice, mCompute.descriptorSetLayout, nullptr );
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

void VulkanRenderer::prepareTextureTarget( VulkanTexture *tex, VkFormat format )
{
  uint32_t w = SCREEN_WIDTH;
  uint32_t h = SCREEN_HEIGHT;

  if ( (int)mRotation )
  {
    w = SCREEN_HEIGHT;
    h = SCREEN_WIDTH;
  }

  VkFormatProperties formatProperties;
  vkGetPhysicalDeviceFormatProperties( mPhysicalDevice, format, &formatProperties );
  assert( formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT );

  tex->mWidth = w;
  tex->mHeight = h;

  auto imageCreateInfo = vkinit::image_create_info( format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, { w, h, 1 } );

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

  vmaCreateImage( mAllocator, &imageCreateInfo, &allocCreateInfo, &tex->mImage, &mLynxScreenAllocation, nullptr );

  VkCommandBuffer layoutCmd = createCommandBuffer( VK_COMMAND_BUFFER_LEVEL_PRIMARY, mCommandPool, true );

  tex->mImageLayout = VK_IMAGE_LAYOUT_GENERAL;
  setImageLayout( layoutCmd, tex->mImage, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, tex->mImageLayout );

  flushCommandBuffer( layoutCmd, mQueue, mCommandPool, true );

  VkImageViewCreateInfo view = vkinit::imageview_create_info( format, tex->mImage, 0 );
  view.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
  VK_CHECK( vkCreateImageView( mDevice, &view, nullptr, &tex->mView ) );

  tex->mDescriptor.imageLayout = tex->mImageLayout;
  tex->mDescriptor.imageView = tex->mView;
  tex->mDevice = mDevice;

  tex->mDS = ImGui_ImplVulkan_AddTexture( nullptr, tex->mView, VK_IMAGE_LAYOUT_GENERAL );
}

void VulkanRenderer::destroyTexture( VulkanTexture *tex )
{
  ImGui_ImplVulkan_RemoveTexture( tex->mDS );
  vkDestroyImageView( mDevice, tex->mView, nullptr );
  vmaDestroyImage( mAllocator, tex->mImage, mLynxScreenAllocation );
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

  if ( mMainScreenTexture.mDS != nullptr )
  {
    destroyTexture( &mMainScreenTexture );
  }

  if ( mCompute.commandBuffer != nullptr )
  {
    destroyCompute();
  }

  prepareTextureTarget( &mMainScreenTexture, VK_FORMAT_R8G8B8A8_UNORM );
  prepareCompute();
}
