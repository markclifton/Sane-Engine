#include <ge/gfx/Common.hpp>

#include <ge/core/Global.hpp>
#include <ge/gfx/Device.hpp>
#include <ge/gfx/Swapchain.hpp>
#include <ge/utils/Common.hpp>

namespace GE
{
	namespace Gfx
	{
		uint32_t GetCurrentFrame()
		{
			return *VulkanCore::Get().currentFrame;
		}

		uint32_t GetNumFrames()
		{
			return VulkanCore::Get().swapchain->GetNumImages();
		}

		VulkanGfxElement::VulkanGfxElement()
			: _core(VulkanCore::Get())
		{}

		VulkanGfxElement& VulkanGfxElement::operator=(const VulkanGfxElement& old)
		{
			return *this;
		}

		VkFormat VulkanGfxElement::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
			for (VkFormat format : candidates) {
				VkFormatProperties props;
				vkGetPhysicalDeviceFormatProperties(*_core.device, format, &props);

				if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
					return format;
				}
				else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
					return format;
				}
			}

			GE_ERROR("Failed to find supported format");
			return VK_FORMAT_UNDEFINED;
		};

		VkFormat VulkanGfxElement::FindDepthFormat() {
			return FindSupportedFormat(
				{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
				VK_IMAGE_TILING_OPTIMAL,
				VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
			);
		};
	}
}