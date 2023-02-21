#include "ge/gfx/Window.hpp"

#include "ge/core/Common.hpp"
#include "ge/core/Global.hpp"
#include <ge/utils/Common.hpp>

const size_t     g_GraphicsSystemMemorySize = 128 * 1024 * 1024;

namespace
{
#ifdef _DEBUG
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
		GE_ERROR("{}", pCallbackData->pMessage);
		return VK_FALSE;
	}
#endif

	void* Allocate(size_t size, size_t alignment, void*)
	{
		return _aligned_malloc(size, alignment); //Might need to align size to alignment?
	}
	void Free(void* addr, void*)
	{
		free(addr);
	}
	void* Reallocate(void* addr, size_t newSize, void*)
	{
		return realloc(addr, newSize);
	}
}

namespace GE
{
	namespace Gfx
	{
		RawWindow::operator GLFWwindow* () { return _window; }
		GLFWwindow*& RawWindow::operator=(GLFWwindow* window) { _window = window; return _window; }

		Window::Window(int width, int height)
			: _width(width)
			, _height(height)
		{
		}

		void Window::Create()
		{
			glfwInit();
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
			_window = glfwCreateWindow(_width, _height, "Sandbox", NULL, NULL);

			glfwSetWindowUserPointer(_window, this);
		}

		void Window::Destroy()
		{
			glfwDestroyWindow(_window);
		}

		bool Window::ShouldClose()
		{
			glfwPollEvents();
			return glfwWindowShouldClose(_window);
		}

		VulkanWindow::VulkanWindow(int width, int height)
			: Window(width, height)
			, System("WindowSystem")
		{
			GE::GlobalDispatcher().sink<vulkan_buffer_ready>().connect<&VulkanWindow::QueueCmdBuffer>(*this);
		}

		void VulkanWindow::Create()
		{
			auto& core = VulkanCore::Get();
			core.instance = &_instance;
			core.device = &_device;
			core.surface = &_surface;
			core.swapchain = &_swapchain;
			core.currentFrame = &_currentFrame;

			Window::Create();

			std::vector<VkExtensionProperties> instanceExtensionProps;
			std::vector<const char*> instanceExtensions;
			std::vector<VkLayerProperties> instanceLayerProps;
			std::vector<const char*> instanceLayers;

			uint32_t extensionsCount;
			const char** extensions = glfwGetRequiredInstanceExtensions(&extensionsCount);
			for (uint32_t i = 0; i < extensionsCount; i++)
			{
				instanceExtensions.push_back(extensions[i]);
			}

#ifdef _DEBUG
			instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
			instanceLayers.push_back("VK_LAYER_KHRONOS_validation");
#endif

			VkApplicationInfo appInfo = {};
			appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
			appInfo.pNext = nullptr;
			appInfo.pApplicationName = "VkWindow";
			appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
			appInfo.pEngineName = GE::ENGINE_NAME;
			appInfo.engineVersion = VK_MAKE_VERSION(GE::MAJOR_VERSION, GE::MINOR_VERSION, GE::PATCH_VERSION);
			appInfo.apiVersion = VK_API_VERSION_1_2;

			VkInstanceCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			createInfo.pNext = nullptr;
			createInfo.flags = 0;
			createInfo.pApplicationInfo = &appInfo;
			createInfo.enabledLayerCount = (uint32_t)instanceLayers.size();
			createInfo.ppEnabledLayerNames = instanceLayers.data();
			createInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
			createInfo.ppEnabledExtensionNames = instanceExtensions.data();
			
			VkResult result = vkCreateInstance(&createInfo, NULL, &_instance);
			GE_ASSERT(result == VK_SUCCESS, "Failed vkCreateInstance: {}", result);
			GE_UNUSED(result);

#ifdef _DEBUG
			{
				VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
				createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
				createInfo.pNext = nullptr;
				createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
				createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
				createInfo.pfnUserCallback = debugCallback;
				PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkCreateDebugUtilsMessengerEXT");
				GE_ASSERT(vkCreateDebugUtilsMessengerEXT);
				vkCreateDebugUtilsMessengerEXT(_instance, &createInfo, nullptr, &_debugMessenger);
			}
#endif

			_surface.Create(_window);
			_device.Create(_surface);

			_swapchain = _device.CreateSwapchain(_window, _surface);

			_imagesAvailable.resize(static_cast<int32_t>(_swapchain.GetNumImages()));
			_rendersFinished.resize(static_cast<int32_t>(_swapchain.GetNumImages()));
			_drawFences.resize(static_cast<int32_t>(_swapchain.GetNumImages()));
			for (uint32_t i = 0; i < _swapchain.GetNumImages(); ++i)
			{
				_imagesAvailable[i] = _device.CreateSemaphore();
				_rendersFinished[i] = _device.CreateSemaphore();
				_drawFences[i] = _device.CreateFence();
			}
		}

		void VulkanWindow::Destroy()
		{
			vkDeviceWaitIdle(_device);

			for (uint32_t i = 0; i < _swapchain.GetNumImages(); ++i)
			{
				vkDestroySemaphore(_device, _imagesAvailable[i], nullptr);
				vkDestroySemaphore(_device, _rendersFinished[i], nullptr);
				vkDestroyFence(_device, _drawFences[i], nullptr);
			}

			_swapchain.Destroy();
			_device.Destroy();
			_surface.Destroy();

#ifdef _DEBUG
			PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkDestroyDebugUtilsMessengerEXT");
			GE_ASSERT(vkDestroyDebugUtilsMessengerEXT);
			vkDestroyDebugUtilsMessengerEXT(_instance, _debugMessenger, nullptr);
#endif

			vkDestroyInstance(_instance, nullptr);
			Window::Destroy();

			auto& core = VulkanCore::Get();
			core.instance = nullptr;
			core.device = nullptr;
			core.surface = nullptr;
			core.swapchain = nullptr;
			core.currentFrame = nullptr;
		}

		void VulkanWindow::ResizeWindow(bool force) 
		{
			int width = 0, height = 0;			
			glfwGetFramebufferSize(_window, &width, &height);
			while (width == 0 || height == 0) {
				glfwGetFramebufferSize(_window, &width, &height);
				glfwWaitEvents();
			}

			TIMED_TRACE();

			vkDeviceWaitIdle(_device);

			VulkanSwapchain oldSwapchain = _swapchain;
			_swapchain = _device.CreateSwapchain(_window._window, _surface, oldSwapchain);

			oldSwapchain.Destroy();

			_currentFrame = GetNumFrames() - 1;

			auto& core = VulkanCore::Get();
			core.instance = &_instance;
			core.device = &_device;
			core.surface = &_surface;
			core.swapchain = &_swapchain;
			core.currentFrame = &_currentFrame;

			GE::GlobalDispatcher().trigger<Gfx::ResizeEvent>(Gfx::ResizeEvent{ force });
		}

		void VulkanWindow::Begin()
		{
			vkWaitForFences(_device, 1, &_drawFences[_currentFrame], VK_TRUE, UINT64_MAX);

			VkResult result = vkAcquireNextImageKHR(_device, _swapchain, std::numeric_limits<uint64_t>::max(), _imagesAvailable[_currentFrame], VK_NULL_HANDLE, &_imageIndex);
			if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
			{
				ResizeWindow((result == VK_ERROR_OUT_OF_DATE_KHR));
				return;
			}
			vkResetFences(_device, 1, &_drawFences[_currentFrame]);
		}

		void VulkanWindow::End()
		{
			if (_buffers.size() == 0)
				return;

			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = &_imagesAvailable[_currentFrame];
			VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
			submitInfo.pWaitDstStageMask = waitStages;
			submitInfo.commandBufferCount = static_cast<uint32_t>(_buffers.size());
			submitInfo.pCommandBuffers = _buffers.data();
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = &_rendersFinished[_currentFrame];

			//vkQueueWaitIdle(_device._graphicQueue);
			VkResult result = vkQueueSubmit(_device._graphicQueue, 1, &submitInfo, _drawFences[_currentFrame]);
			GE_ASSERT(result == VK_SUCCESS);

			VkPresentInfoKHR presentInfo = {};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			presentInfo.waitSemaphoreCount = 1;
			presentInfo.pWaitSemaphores = &_rendersFinished[_currentFrame];
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = _swapchain;
			presentInfo.pImageIndices = &_imageIndex;

			result = vkQueuePresentKHR(_device.GetPresentationQueue(), &presentInfo);
			if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
			{
				ResizeWindow((result == VK_ERROR_OUT_OF_DATE_KHR));
			}

			_currentFrame = (_currentFrame + 1) % static_cast<int32_t>(_swapchain.GetNumImages());
			_buffers.clear();
		}

		void VulkanWindow::QueueCmdBuffer(const vulkan_buffer_ready& cmdBufferReady)
		{
			_buffers.push_back(cmdBufferReady.buffer);
		}
	}
}