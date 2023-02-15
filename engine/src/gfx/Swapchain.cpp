#include "ge/gfx/Swapchain.hpp"

#include <ge/gfx/Device.hpp>
#include <ge/gfx/Texture.hpp>
#include <ge/utils/Common.hpp>

namespace GE
{
	namespace Gfx
	{
		void VulkanSwapchain::Create(const VkSwapchainCreateInfoKHR& info)
		{
			VkResult result = vkCreateSwapchainKHR(*_core.device, &info, nullptr, &_swapchain);
			GE_ASSERT(result == VK_SUCCESS, "Failed vkCreateSwapchainKHR: {}", result);
			GE_UNUSED(result);

			_format = info.imageFormat;
			_extent = info.imageExtent;

			uint32_t swapchainImageCount = 0;
			vkGetSwapchainImagesKHR(*_core.device, _swapchain, &swapchainImageCount, nullptr);
			std::vector<VkImage> images(swapchainImageCount);
			vkGetSwapchainImagesKHR(*_core.device, _swapchain, &swapchainImageCount, images.data());

			for(VkImage image : images)
			{
				VkImageViewCreateInfo viewCreateInfo = {};
				viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				viewCreateInfo.image = image;
				viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
				viewCreateInfo.format = _format;
				viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
				viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
				viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
				viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

				viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				viewCreateInfo.subresourceRange.baseMipLevel = 0;
				viewCreateInfo.subresourceRange.levelCount = 1;
				viewCreateInfo.subresourceRange.baseArrayLayer = 0;
				viewCreateInfo.subresourceRange.layerCount = 1;

				VkImageView imageView;
				VkResult result = vkCreateImageView(*_core.device, &viewCreateInfo, nullptr, &imageView);
				GE_ASSERT(result == VK_SUCCESS, "Failed vkCreateImageView: {}", result);
				GE_UNUSED(result);

				_swapchainImages.emplace_back(image, imageView);
			}

			{
				VkImageCreateInfo imageInfo{};
				imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
				imageInfo.imageType = VK_IMAGE_TYPE_2D;
				imageInfo.extent.width = _extent.width;
				imageInfo.extent.height = _extent.height;
				imageInfo.extent.depth = 1;
				imageInfo.mipLevels = 1;
				imageInfo.arrayLayers = 1;
				imageInfo.format = FindDepthFormat();
				imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
				imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
				imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
				imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

				VkImageViewCreateInfo viewCreateInfo = {};
				viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
				viewCreateInfo.format = FindDepthFormat();
				viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
				viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
				viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
				viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
				viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				viewCreateInfo.subresourceRange.baseMipLevel = 0;
				viewCreateInfo.subresourceRange.levelCount = 1;
				viewCreateInfo.subresourceRange.baseArrayLayer = 0;
				viewCreateInfo.subresourceRange.layerCount = 1;

				for (uint32_t i = 0; i < swapchainImageCount; ++i)
				{
					auto depthTexture = new VulkanTexture();
					depthTexture->Create(imageInfo, viewCreateInfo);
					_depthTextures.push_back(depthTexture);
				}
			}
		}

		void VulkanSwapchain::Destroy()
		{
			for (auto& depthTexture : _depthTextures)
			{
				depthTexture->Destroy();
				delete depthTexture;
			}
			_depthTextures.clear();

			for (auto& image : _swapchainImages)
			{
				vkDestroyImageView(*_core.device, image.imageView, nullptr);
			}
			vkDestroySwapchainKHR(*_core.device, _swapchain, nullptr);
		}

		SwapchainImage& VulkanSwapchain::GetImage(uint32_t index)
		{
			GE_ASSERT(index < _swapchainImages.size(), "Index not valid!");
			return _swapchainImages[index];
		}

		VulkanTexture& VulkanSwapchain::GetDepthImage(uint32_t index)
		{
			GE_ASSERT(index < _depthTextures.size(), "Index not valid!");
			return *_depthTextures[index];
		}
	}
}
