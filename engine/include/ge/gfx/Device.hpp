#pragma once

#include <vector>

#include <ge/gfx/Common.hpp>
#include <ge/gfx/Swapchain.hpp>

namespace GE
{
	namespace Gfx
	{
		class VulkanDevice : public VulkanGfxElement
		{
			friend class VulkanWindow;
		public:
			VulkanDevice();
			virtual ~VulkanDevice() = default;

			virtual void Create(const VkSurfaceKHR& surface);
			virtual void Destroy();

			inline operator VkDevice() { return _device; }
			inline operator VkDevice* () { return &_device; }
			inline operator VkPhysicalDevice() { return _physicalDevice; }
			inline operator VkPhysicalDevice*() { return &_physicalDevice; }

			inline VkQueue GetGraphicsQueue() { return _graphicQueue; }
			inline VkQueue GetPresentationQueue() { return _presentationQueue; }

			SwapchainDetails GetSwapChainDetails(const VkPhysicalDevice& device, const VkSurfaceKHR& surface);
#ifdef _WIN32
			VulkanSwapchain CreateSwapchain(GLFWwindow* window, const VkSurfaceKHR& surface, VkSwapchainKHR oldSwapChain = VK_NULL_HANDLE);
#elif defined(NN_BUILD_TARGET_PLATFORM_NX)
			VulkanSwapchain CreateSwapchain(const VkSurfaceKHR& surface, VkSwapchainKHR oldSwapChain = VK_NULL_HANDLE);
#endif

			VkSemaphore CreateSemaphore();
			VkFence CreateFence();

			virtual bool CheckDeviceSuitable(const VkPhysicalDevice& device, const VkSurfaceKHR& surface);
			virtual bool CheckDeviceExtensionSupport(const VkPhysicalDevice& device);
			QueueFamilyIndices GetQueueFamiles(const VkPhysicalDevice& device, const VkSurfaceKHR& surface);

		private:
			VkPhysicalDevice _physicalDevice;
			VkDevice _device;
			VkQueue _graphicQueue;
			VkQueue _presentationQueue;

			std::vector<const char*> _extensions;
			std::vector<const char*> _layers;
		};
	}
}