#pragma once

#include <ge/gfx/Common.hpp>
#include <ge/gfx/Device.hpp>
#include <ge/gfx/Surface.hpp>
#include <ge/gfx/Swapchain.hpp>
#include <ge/systems/Systems.hpp>

namespace GE
{
	namespace Gfx
	{
		class RawWindow
		{
		public:
			GLFWwindow* _window;
			operator GLFWwindow* ();
			GLFWwindow*& operator=(GLFWwindow* window);
		};

		class Window
		{
		public:
			Window(int width = 1280, int height = 720);
			virtual ~Window() = default;

			virtual void Create();
			virtual void Destroy();

			virtual bool ShouldClose();
			virtual void ResizeWindow(bool force) = 0;

			RawWindow _window;

			bool _resizedWindow = false;

			int _width;
			int _height;
		};

		class VulkanWindow : public Window, public Sys::System
		{
		public:
			VulkanWindow(int width = 1280, int height = 720);
			~VulkanWindow() = default;

			virtual void Create() override;
			virtual void Destroy() override;
			virtual void ResizeWindow(bool force) override;

			virtual void Begin() override;
			virtual void End() override;

			void QueueCmdBuffer(const vulkan_buffer_ready& cmdBufferReady);

		private:
			VkInstance _instance{ VK_NULL_HANDLE };
#ifdef _DEBUG
			VkDebugUtilsMessengerEXT _debugMessenger{ VK_NULL_HANDLE };
#endif
			VulkanDevice _device;
			VulkanSurface _surface;
			VulkanSwapchain _swapchain;

			uint32_t _imageIndex = -1;
			uint32_t _currentFrame = 0;
			std::vector<VkSemaphore> _imagesAvailable;
			std::vector<VkSemaphore> _rendersFinished;
			std::vector<VkFence> _drawFences;

			std::vector<VkCommandBuffer> _buffers;
		};
	}
}