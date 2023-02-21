#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

namespace GE
{
	namespace Gfx
	{
		class VulkanDevice;
		class VulkanSurface;
		class VulkanSwapchain;

		struct VulkanCore
		{
			static VulkanCore& Get()
			{
				static VulkanCore core;
				return core;
			}

			VkInstance* instance;
			VulkanDevice* device;
			VulkanSurface* surface;
			VulkanSwapchain* swapchain;
			uint32_t* currentFrame;
		};

		uint32_t GetCurrentFrame();
		uint32_t GetNumFrames();
		uint32_t GetFrameWidth();
		uint32_t GetFrameHeight();

		struct vulkan_buffer_ready
		{
			VkCommandBuffer buffer;
		};

		struct ResizeEvent
		{
			bool recreate;
		};

		struct vulkan_core_request_event
		{
			bool test;
		};

#define FORMAT(x) VkFormat(x)

		enum Format
		{
			FLOAT = VK_FORMAT_R32_SFLOAT,
			VEC2 = VK_FORMAT_R32G32_SFLOAT,
			VEC3 = VK_FORMAT_R32G32B32_SFLOAT,
			VEC4 = VK_FORMAT_R32G32B32A32_SFLOAT
		};

		struct QueueFamilyIndices {
			int graphicsFamily = -1; // Location of Graphics Queue Family
			int presentationFamily = -1; // Location of Presentation Queue Family
			bool isValid() { return graphicsFamily > -1 && presentationFamily > -1; }
		};

		struct SwapchainDetails {
			VkSurfaceCapabilitiesKHR surfaceCapabilities;
			std::vector<VkSurfaceFormatKHR> formats;
			std::vector<VkPresentModeKHR> presentationModes;
		};

		struct SwapchainImage {
			SwapchainImage(VkImage image, VkImageView imageView)
				: image(image), imageView(imageView)
			{}
			VkImage image;
			VkImageView imageView;
		};

		class VulkanGfxElement
		{
		public:
			VulkanGfxElement();
			virtual ~VulkanGfxElement() = default;

			VulkanGfxElement& operator=(const VulkanGfxElement& old);

			VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
			VkFormat FindDepthFormat();

			VulkanCore& _core;
		};
	}
}