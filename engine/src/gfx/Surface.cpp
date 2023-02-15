#include "ge/gfx/Surface.hpp"

#include <memory>

#include <ge/utils/Common.hpp>

namespace GE
{
	namespace Gfx
	{
		void VulkanSurface::Create(GLFWwindow* window)
		{
			VkResult result = glfwCreateWindowSurface(*_core.instance, window, nullptr, &_surface);
			GE_ASSERT(result == VK_SUCCESS, "Failed glfwCreateWindowSurface: {}", result);
		}

		void VulkanSurface::Destroy()
		{
			vkDestroySurfaceKHR(*_core.instance, _surface, nullptr);
		}
	}
}