#pragma once

#include <ge/gfx/Common.hpp>

namespace GE
{
	namespace Gfx
	{
		class VulkanSurface : public VulkanGfxElement
		{
		public:
			virtual void Create(GLFWwindow* window);
			virtual void Destroy();

			inline operator VkSurfaceKHR() { return _surface; }
	
		private:
			VkSurfaceKHR _surface;
		};
	}
}