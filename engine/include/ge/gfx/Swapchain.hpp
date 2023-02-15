#pragma once

#include <memory>
#include <vector>

#include <ge/gfx/Common.hpp>

namespace GE
{
	namespace Gfx
	{
		class VulkanTexture;

		class VulkanSwapchain : public VulkanGfxElement
		{
		public:
			void Create(const VkSwapchainCreateInfoKHR& info);
			void Destroy();

			SwapchainImage& GetImage(uint32_t index);
			VulkanTexture& GetDepthImage(uint32_t index);

			inline operator VkSwapchainKHR () { return _swapchain; }
			inline operator VkSwapchainKHR* () { return &_swapchain; }

			inline const VkFormat& GetFormat() { return _format; }
			inline const VkExtent2D& GetExtent() { return _extent; }
			
			inline const uint32_t GetNumImages() { return static_cast<uint32_t>(_swapchainImages.size()); }
		private:
			VkSwapchainKHR _swapchain;
		
			VkFormat _format;
			VkExtent2D _extent;
			std::vector<SwapchainImage> _swapchainImages;
			std::vector<VulkanTexture*> _depthTextures;
		};
	}
}