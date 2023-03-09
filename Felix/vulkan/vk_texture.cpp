#include "pch.hpp"
#include "vk_texture.hpp"

void VulkanTexture::updateDescriptor()
{
	mDescriptor.sampler = mSampler;
	mDescriptor.imageView = mView;
	mDescriptor.imageLayout = mImageLayout;
}

void VulkanTexture::destroy()
{
	vkDestroyImageView( mDevice, mView, nullptr );
	vkDestroyImage( mDevice, mImage, nullptr );
	if (mSampler)
	{
		vkDestroySampler( mDevice, mSampler, nullptr );
	}
	vkFreeMemory( mDevice, mDeviceMemory, nullptr );
}