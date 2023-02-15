#include <ge/gfx/Device.hpp>

#include <algorithm>
#include <memory>
#include <set>

#include <ge/utils/Common.hpp>

namespace GE
{
	namespace Gfx
	{
		VulkanDevice::VulkanDevice()
		{
			_extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		}

		void VulkanDevice::Create(const VkSurfaceKHR& surface)
		{
			uint32_t deviceCount = 0;
			vkEnumeratePhysicalDevices(*_core.instance, &deviceCount, nullptr);
			GE_ASSERT(deviceCount != 0);
			
			std::vector<VkPhysicalDevice> physicalDeviceList(deviceCount);
			vkEnumeratePhysicalDevices(*_core.instance, &deviceCount, physicalDeviceList.data());

			for (auto& device : physicalDeviceList)
			{
				if (CheckDeviceSuitable(device, surface))
				{
					_physicalDevice = device;
					break;
				}
			}

			QueueFamilyIndices indices = GetQueueFamiles(_physicalDevice, surface);

			std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
			std::set<int> queueFamilyIndices = { indices.graphicsFamily, indices.presentationFamily };

			for (int queueFamilyIndex : queueFamilyIndices)
			{
				VkDeviceQueueCreateInfo queueCreateInfo = {};
				queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
				queueCreateInfo.queueCount = 1;
				float priority = 1.f;
				queueCreateInfo.pQueuePriorities = &priority;

				queueCreateInfos.push_back(queueCreateInfo);
			}

			VkDeviceCreateInfo deviceCreateInfo = {};
			deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
			deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
			deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(_extensions.size());
			deviceCreateInfo.ppEnabledExtensionNames = _extensions.data();
			deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(_layers.size());
			deviceCreateInfo.ppEnabledLayerNames = _layers.data();

			// Bindless Textures
			VkPhysicalDeviceDescriptorIndexingFeatures indexing_features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT, nullptr };
			indexing_features.descriptorBindingPartiallyBound = VK_TRUE;
			indexing_features.runtimeDescriptorArray = VK_TRUE;
			indexing_features.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
			indexing_features.descriptorBindingVariableDescriptorCount = VK_TRUE;

			VkPhysicalDeviceFeatures2 physical_features2 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
			vkGetPhysicalDeviceFeatures2(_core.device->_physicalDevice, &physical_features2);
			deviceCreateInfo.pNext = &physical_features2;

			indexing_features.descriptorBindingPartiallyBound = VK_TRUE;
			indexing_features.runtimeDescriptorArray = VK_TRUE;
			physical_features2.pNext = &indexing_features;

			VkPhysicalDeviceFeatures deviceFeatures = {};
			deviceCreateInfo.pEnabledFeatures = NULL;// & deviceFeatures;
			deviceFeatures.samplerAnisotropy = VK_TRUE;

			VkResult result = vkCreateDevice(_physicalDevice, &deviceCreateInfo, nullptr, &_device);
			GE_ASSERT(result == VK_SUCCESS, "Failed vkCreateDevice: {}", result);
			GE_UNUSED(result);

			vkGetDeviceQueue(_device, indices.graphicsFamily, 0, &_graphicQueue);
			vkGetDeviceQueue(_device, indices.presentationFamily, 0, &_presentationQueue);
		}

		void VulkanDevice::Destroy()
		{
			vkDestroyDevice(_device, nullptr);
		}

		bool VulkanDevice::CheckDeviceSuitable(const VkPhysicalDevice& device, const VkSurfaceKHR& surface)
		{
			QueueFamilyIndices indices = GetQueueFamiles(device, surface);

			bool extensionsSupported = CheckDeviceExtensionSupport(device);
			bool swapChainValid = false;
			if (extensionsSupported)
			{
				SwapchainDetails swapChainDetails = GetSwapChainDetails(device, surface);
				swapChainValid = !swapChainDetails.presentationModes.empty() && !swapChainDetails.formats.empty();
			}

			VkPhysicalDeviceFeatures supportedFeatures;
			vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

			VkPhysicalDeviceDescriptorIndexingFeatures indexing_features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT, nullptr };
			VkPhysicalDeviceFeatures2 supportedFeatures2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, &indexing_features };
			vkGetPhysicalDeviceFeatures2(device, &supportedFeatures2);

			return indices.isValid() && extensionsSupported && swapChainValid && supportedFeatures.samplerAnisotropy && (indexing_features.descriptorBindingPartiallyBound && indexing_features.runtimeDescriptorArray);
		}

		bool VulkanDevice::CheckDeviceExtensionSupport(const VkPhysicalDevice& device)
		{
			uint32_t extensionCount = 0;
			vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
			if (extensionCount == 0)
				return false;

			std::vector<VkExtensionProperties> extensions(extensionCount);
			vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());

			for (const auto& deviceExtension : _extensions)
			{
				bool hasExtension = false;
				for (const auto& extension : extensions)
				{
					if (strcmp(extension.extensionName, deviceExtension) == 0)
					{
						hasExtension = true;
						break;
					}
				}
				if (!hasExtension)
					return false;
			}

			return true;
		}

		QueueFamilyIndices VulkanDevice::GetQueueFamiles(const VkPhysicalDevice& device, const VkSurfaceKHR& surface)
		{
			QueueFamilyIndices indices;

			uint32_t queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
			
			std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyList.data());

			int i = -1;
			for (const auto& queueFamily : queueFamilyList)
			{
				i++;
				if (queueFamily.queueCount == 0)
					continue;

				if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
					indices.graphicsFamily = i;

				VkBool32 presentationSupport = VK_FALSE;
				vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentationSupport);
				if (presentationSupport)
					indices.presentationFamily = i;

				if (indices.isValid())
					break;
			}
			return indices;
		}

		SwapchainDetails VulkanDevice::GetSwapChainDetails(const VkPhysicalDevice& device, const VkSurfaceKHR& surface)
		{
			SwapchainDetails swapChainDetails;

			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &swapChainDetails.surfaceCapabilities);

			uint32_t formatCount;
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
			if (formatCount > 0)
			{
				swapChainDetails.formats.resize(formatCount);
				vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, swapChainDetails.formats.data());
			}

			uint32_t presentationCount = 0;
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationCount, nullptr);
			if (presentationCount > 0)
			{
				swapChainDetails.presentationModes.resize(presentationCount);
				vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationCount, swapChainDetails.presentationModes.data());
			}

			return swapChainDetails;
		}

		VulkanSwapchain VulkanDevice::CreateSwapchain(GLFWwindow* window, const VkSurfaceKHR& surface, VkSwapchainKHR oldSwapChain)	
		{
			SwapchainDetails swapChainDetails = GetSwapChainDetails(_physicalDevice, surface);

			VkSurfaceFormatKHR surfaceFormat = swapChainDetails.formats[0];
			VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
			VkExtent2D extent;
			uint32_t imageCount;
			{
				// Surface Format
				if (swapChainDetails.formats.size() > 1 || swapChainDetails.formats[0].format != VK_FORMAT_UNDEFINED)
				{
					for (const auto& format : swapChainDetails.formats)
					{
						if ((format.format == VK_FORMAT_R8G8B8A8_UNORM || format.format == VK_FORMAT_B8G8R8A8_UNORM)
							&& format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
						{
							surfaceFormat = format;
						}
					}
				}

				// Presentation Mode
				for (const auto& presentationMode : swapChainDetails.presentationModes)
				{
					if (presentationMode == VK_PRESENT_MODE_MAILBOX_KHR) // VSYNC VK_PRESENT_MODE_FIFO_KHR
					{
						presentMode = presentationMode;
					}
				}

				// Swap Chain Resolution
				if (swapChainDetails.surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
				{
					extent = swapChainDetails.surfaceCapabilities.currentExtent;
				}
				else
				{
					int width, height;
					glfwGetFramebufferSize(nullptr, &width, &height);
					extent.width = static_cast<uint32_t>(width);
					extent.height = static_cast<uint32_t>(height);
				}

				extent.width = std::max(swapChainDetails.surfaceCapabilities.minImageExtent.width, std::min(swapChainDetails.surfaceCapabilities.maxImageExtent.width, extent.width));
				extent.height = std::max(swapChainDetails.surfaceCapabilities.minImageExtent.height, std::min(swapChainDetails.surfaceCapabilities.maxImageExtent.height, extent.height));

				imageCount = swapChainDetails.surfaceCapabilities.minImageCount + 1;
				if (swapChainDetails.surfaceCapabilities.maxImageCount > 0 && imageCount > swapChainDetails.surfaceCapabilities.maxImageCount)
				{
					imageCount = swapChainDetails.surfaceCapabilities.maxImageCount;
				}
			}

			QueueFamilyIndices indices = GetQueueFamiles(_physicalDevice, surface);

			VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
			swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			swapChainCreateInfo.imageFormat = surfaceFormat.format;
			swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
			swapChainCreateInfo.presentMode = presentMode;
			swapChainCreateInfo.imageExtent = extent;
			swapChainCreateInfo.minImageCount = imageCount;
			swapChainCreateInfo.imageArrayLayers = 1;
			swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			swapChainCreateInfo.preTransform = swapChainDetails.surfaceCapabilities.currentTransform;
			swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			swapChainCreateInfo.clipped = VK_TRUE;
			if (indices.graphicsFamily != indices.presentationFamily)
			{
				uint32_t queueFamilyIndices[] = { (uint32_t)indices.graphicsFamily, (uint32_t)indices.presentationFamily };
				swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
				swapChainCreateInfo.queueFamilyIndexCount = 2;
				swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
			}
			else
			{
				swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
				swapChainCreateInfo.queueFamilyIndexCount = 0;
				swapChainCreateInfo.pQueueFamilyIndices = nullptr;
			}
			swapChainCreateInfo.oldSwapchain = oldSwapChain;
			swapChainCreateInfo.surface = surface;

			VulkanSwapchain swapchain;
			swapchain.Create(swapChainCreateInfo);
			return swapchain;
		}

		VkSemaphore VulkanDevice::CreateSemaphore()
		{
			VkSemaphoreCreateInfo semaphoreCreateInfo = {};
			semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

			VkSemaphore semaphore;
			VkResult result = vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &semaphore);
			GE_ASSERT(result == VK_SUCCESS, "Failed vkCreateSemaphore: {}", result);
			GE_UNUSED(result);

			return semaphore;
		}

		VkFence VulkanDevice::CreateFence()
		{
			VkFenceCreateInfo fenceCreateInfo = {};
			fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

			VkFence fence;
			VkResult result = vkCreateFence(_device, &fenceCreateInfo, nullptr, &fence);
			GE_ASSERT(result == VK_SUCCESS, "Failed vkCreateFence: {}", result);
			GE_UNUSED(result);

			return fence;
		}
	}
}