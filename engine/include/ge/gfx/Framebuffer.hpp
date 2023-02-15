#pragma once

#include <vector>

#include <ge/gfx/Common.hpp>

namespace GE
{
	namespace Gfx
	{
		class VulkanFramebuffer : public VulkanGfxElement
		{
		public:
			VulkanFramebuffer() = default;
			virtual ~VulkanFramebuffer() = default;

			virtual void Create(VkRenderPass& renderPass, std::vector<VkImageView> imageViews, VkExtent2D extent);
			virtual void Create(VkRenderPass& renderPass);
			virtual void Destroy();

			VkFramebuffer& Get(uint32_t index);
			uint32_t Count();

		private:
			std::vector<VkFramebuffer> _framebuffers;
		};
	}
}