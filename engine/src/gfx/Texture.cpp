#include <ge/gfx/Texture.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <ge/gfx/Swapchain.hpp>
#include <ge/utils/Common.hpp>

namespace GE
{
	namespace Gfx
	{
		void SetImageLayout(VkCommandBuffer cmdbuffer, VkImage image, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkImageSubresourceRange subresourceRange, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask)
		{
			// Create an image barrier object
			VkImageMemoryBarrier imageMemoryBarrier{};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.oldLayout = oldImageLayout;
			imageMemoryBarrier.newLayout = newImageLayout;
			imageMemoryBarrier.image = image;
			imageMemoryBarrier.subresourceRange = subresourceRange;

			switch (oldImageLayout)
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

			switch (newImageLayout)
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
				if (imageMemoryBarrier.srcAccessMask == 0)
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;
			default:
				break;
			}

			vkCmdPipelineBarrier(cmdbuffer, srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
		}

		void SetImageLayout(VkCommandBuffer cmdbuffer, VkImage image, VkImageAspectFlags aspectMask, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask)
		{
			SetImageLayout(cmdbuffer, image, oldImageLayout, newImageLayout, { aspectMask , 0, 1, 0, 1 }, srcStageMask, dstStageMask);
		}

		void VulkanTexture::Create(VkImageCreateInfo imageCreateInfo, VkImageViewCreateInfo imageViewCreateInfo)
		{
			_width = imageCreateInfo.extent.width;
			_height = imageCreateInfo.extent.height;

			vkCreateImage(*_core.device, &imageCreateInfo, nullptr, &_image);

			VkMemoryRequirements memRequirements;
			vkGetImageMemoryRequirements(*_core.device, _image, &memRequirements);

			VkMemoryAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = memRequirements.size;
			allocInfo.memoryTypeIndex = static_cast<uint32_t>(-1);

			VkPhysicalDeviceMemoryProperties memProperties;
			vkGetPhysicalDeviceMemoryProperties(*_core.device, &memProperties);
			for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
				if ((memRequirements.memoryTypeBits & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
					allocInfo.memoryTypeIndex = i;
					break;
				}
			}

			VkResult result = vkAllocateMemory(*_core.device, &allocInfo, nullptr, &_memory);
			GE_ASSERT(result == VK_SUCCESS, "Failed to allocate memory");

			vkBindImageMemory(*_core.device, _image, _memory, 0);
			imageViewCreateInfo.image = _image;
			result = vkCreateImageView(*_core.device, &imageViewCreateInfo, nullptr, &_imageView);
			GE_ASSERT(result == VK_SUCCESS, "Failed to create image view");

			VkPhysicalDeviceProperties properties{};
			vkGetPhysicalDeviceProperties(*_core.device, &properties);

			VkSamplerCreateInfo samplerInfo{};
			samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerInfo.magFilter = VK_FILTER_NEAREST;
			samplerInfo.minFilter = VK_FILTER_NEAREST;
			samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			samplerInfo.anisotropyEnable = VK_TRUE;
			samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
			samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
			samplerInfo.unnormalizedCoordinates = VK_FALSE;
			samplerInfo.compareEnable = VK_FALSE;
			samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

			result = vkCreateSampler(*_core.device, &samplerInfo, nullptr, &_sampler);
			GE_ASSERT(result == VK_SUCCESS, "Failed to create sampler");
		}

		void VulkanTexture::Create(VkCommandBuffer& cmdBuffer)
		{
			GE_ASSERT(_pixels != nullptr, "Texture data not loaded from storage");

			VkDeviceSize imageSize = _width * _height * 4;
			_buffer.Create(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, static_cast<size_t>(imageSize));
			_buffer.Buffer(cmdBuffer, _pixels, static_cast<size_t>(imageSize));

			stbi_image_free(_pixels);
			_pixels = nullptr;

			VkImageCreateInfo imageInfo{};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.extent.width = static_cast<uint32_t>(_width);
			imageInfo.extent.height = static_cast<uint32_t>(_height);
			imageInfo.extent.depth = 1;
			imageInfo.mipLevels = 1;
			imageInfo.arrayLayers = 1;
			imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageInfo.flags = 0; // Optional

			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = 0;
			viewInfo.subresourceRange.layerCount = 1;

			Create(imageInfo, viewInfo);

			//Filtering
			{
			}

			BeginVkCmd(cmdBuffer);
			TransitionLayout(cmdBuffer, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			CopyBufferToImage(cmdBuffer);
			TransitionLayout(cmdBuffer, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			EndVkCmd(cmdBuffer);

			_loaded.Update(true);
		}
		
		void VulkanTexture::Destroy()
		{
			_loaded.Update(false);

			vkDestroySampler(*_core.device, _sampler, nullptr);
			vkDestroyImageView(*_core.device, _imageView, nullptr);

			_buffer.Destroy();
			vkDestroyImage(*_core.device, _image, nullptr);
			vkFreeMemory(*_core.device, _memory, nullptr);
		}

		void VulkanTexture::LoadFromStorage(const std::string& path)
		{
			GE_ASSERT(!_loaded.Get(), "Texture already loaded");

			stbi_set_flip_vertically_on_load(true);
			auto data = Utils::LoadFile(path.c_str());
			_pixels = stbi_load_from_memory((unsigned char*)data.data(), static_cast<int>(data.size()), &_width, &_height, &_channels, STBI_rgb_alpha);
		}

		void VulkanTexture::LoadFromEngineResources(const std::string& path)
		{
			GE_ASSERT(!_loaded.Get(), "Texture already loaded");

			auto dataPoint = Utils::EngineResourceParser::GetDataPoint(path);
			GE_ASSERT(dataPoint.size > 0, "Resource not found: {}", path);

			auto data = GE::Utils::LoadFile("EngineData.blob", dataPoint.startPoint, dataPoint.size);

			stbi_set_flip_vertically_on_load(true);
			_pixels = stbi_load_from_memory((unsigned char*)data.data(), static_cast<int>(dataPoint.size), &_width, &_height, &_channels, STBI_rgb_alpha);

			GE_ASSERT(dataPoint.size > 0, "Resource not found: {}", path);

		}

		void VulkanTexture::CopyBufferToImage(VkCommandBuffer& cmdBuffer)
		{
			VkBufferImageCopy region{};
			region.bufferOffset = 0;
			region.bufferRowLength = 0;
			region.bufferImageHeight = 0;

			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.mipLevel = 0;
			region.imageSubresource.baseArrayLayer = 0;
			region.imageSubresource.layerCount = 1;

			region.imageOffset = { 0, 0, 0 };
			region.imageExtent = { static_cast<unsigned int>(_width), static_cast<unsigned int>(_height), 1 };

			vkCmdCopyBufferToImage(cmdBuffer, _buffer, _image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
		}

		void VulkanTexture::TransitionLayout(VkCommandBuffer& cmdBuffer, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
		{
			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = oldLayout;
			barrier.newLayout = newLayout;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = _image;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;

			VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM;
			VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM;

			if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
				barrier.srcAccessMask = 0;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

				sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			}
			else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
				destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			}
			else {
				GE_ERROR("Unsupported layout transition");
			}

			vkCmdPipelineBarrier(cmdBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
		}

		void VulkanTexture::BeginVkCmd(VkCommandBuffer& cmdBuffer)
		{
			VkCommandBufferBeginInfo begin_info = {};
			begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			VkResult result = vkBeginCommandBuffer(cmdBuffer, &begin_info);
			GE_ASSERT(result == VK_SUCCESS, "Failed vkBeginCommandBuffer");
			GE_UNUSED(result);
		}
		
		void VulkanTexture::EndVkCmd(VkCommandBuffer& cmdBuffer)
		{
			VkSubmitInfo end_info = {};
			end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			end_info.commandBufferCount = 1;
			end_info.pCommandBuffers = &cmdBuffer;

			VkResult result = vkEndCommandBuffer(cmdBuffer);
			GE_ASSERT(result == VK_SUCCESS, "Failed vkEndCommandBuffer");

			result = vkQueueSubmit(_core.device->GetGraphicsQueue(), 1, &end_info, VK_NULL_HANDLE);
			GE_ASSERT(result == VK_SUCCESS, "Failed vkQueueSubmit");

			result = vkDeviceWaitIdle(*_core.device);
			GE_ASSERT(result == VK_SUCCESS, "Failed vkDeviceWaitIdle");
		}
	}
}