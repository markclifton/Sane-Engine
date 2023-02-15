#pragma once

#include <vector>

#include <ge/gfx/Common.hpp>

namespace GE
{
	namespace Gfx
	{
		class VulkanCommandBuffers : public VulkanGfxElement
		{
			friend class VulkanWindow;
		public:
			void Create(uint32_t numFrames);
			void Destroy();

			VkCommandBuffer& GetBuffer();
			void SubmitBuffer();

			void CreateCommandPool();

			inline void Reset() { _nextBuffer = 0; }
						
			VkCommandPool _commandPool;
			std::vector<VkCommandBuffer> _commandBuffers;
			size_t _nextBuffer{ 0 };
		};
	}
}