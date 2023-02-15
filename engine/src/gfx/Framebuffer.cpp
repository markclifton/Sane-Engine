#include <ge/gfx/Framebuffer.hpp>

#include <vector>

#include <ge/core/Common.hpp>
#include <ge/gfx/Device.hpp>
#include <ge/gfx/Texture.hpp>

namespace GE
{
	namespace Gfx
	{
		void VulkanFramebuffer::Create(VkRenderPass& renderPass, std::vector<VkImageView> imageViews, VkExtent2D extent)
		{
			_framebuffers.push_back({});

			VkFramebufferCreateInfo framebufferCreateInfo = {};
			framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferCreateInfo.renderPass = renderPass;
			framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(imageViews.size());
			framebufferCreateInfo.pAttachments = imageViews.data();
			framebufferCreateInfo.width = extent.width;
			framebufferCreateInfo.height = extent.height;
			framebufferCreateInfo.layers = 1;

			VkResult result = vkCreateFramebuffer(*_core.device, &framebufferCreateInfo, nullptr, &_framebuffers.back());
			GE_ASSERT(result == VK_SUCCESS, "Failed vkCreateFramebuffer: {}", result);
			GE_UNUSED(result);
		}

		void VulkanFramebuffer::Create(VkRenderPass& renderPass)
		{
			for (uint32_t i = 0; i < GetNumFrames(); ++i)
			{
				std::vector<VkImageView> attachments{ _core.swapchain->GetImage(i).imageView, _core.swapchain->GetDepthImage(i)._imageView };
				Create(renderPass, attachments, _core.swapchain->GetExtent());
			}
		}

		void VulkanFramebuffer::Destroy()
		{
			for (auto& framebuffer : _framebuffers)
			{
				vkDestroyFramebuffer(*_core.device, framebuffer, nullptr);
			}
			_framebuffers.clear();
		}

		VkFramebuffer& VulkanFramebuffer::Get(uint32_t index)
		{
			GE_ASSERT(index < _framebuffers.size(), "Exceeded bounds of framebuffer array");
			return _framebuffers[index];
		}

		uint32_t VulkanFramebuffer::Count()
		{
			return static_cast<uint32_t>(_framebuffers.size());
		}
	}
}