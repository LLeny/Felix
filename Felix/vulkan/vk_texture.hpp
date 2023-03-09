#pragma once

#include <fstream>
#include <stdlib.h>
#include <string>
#include <vector>

#include "vulkan/vulkan.h"

class VulkanTexture
{
public:
	VkDevice							mDevice;
	VkImage               mImage;
	VkImageLayout         mImageLayout;
	VkDeviceMemory        mDeviceMemory;
	VkImageView           mView;
	uint32_t              mWidth, mHeight;
	uint32_t              mMipLevels;
	uint32_t              mLayerCount;
	VkDescriptorImageInfo mDescriptor;
	VkSampler             mSampler;
	VkDescriptorSet				mDS;

	void      updateDescriptor();
	void      destroy();
};
