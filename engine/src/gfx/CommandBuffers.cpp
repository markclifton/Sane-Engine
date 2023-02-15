#include <ge/gfx/CommandBuffers.hpp>

#include <ge/core/Global.hpp>
#include <ge/gfx/Device.hpp>
#include <ge/gfx/Surface.hpp>
#include <ge/utils/Common.hpp>

namespace GE
{
	namespace Gfx
	{
		void VulkanCommandBuffers::Create(uint32_t numFrames)
		{
			CreateCommandPool();

			_commandBuffers.resize(numFrames);

			VkCommandBufferAllocateInfo cbAllocInfo = {};
			cbAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			cbAllocInfo.commandPool = _commandPool;
			cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			cbAllocInfo.commandBufferCount = static_cast<uint32_t>(_commandBuffers.size());

			VkResult result = vkAllocateCommandBuffers(*_core.device, &cbAllocInfo, _commandBuffers.data());
			GE_ASSERT(result == VK_SUCCESS, "Failed vkAllocateCommandBuffers: {}", result);
			GE_UNUSED(result);
		}

		void VulkanCommandBuffers::Destroy()
		{
			vkDestroyCommandPool(*_core.device, _commandPool, nullptr);
			_commandPool = VK_NULL_HANDLE;
		}

		VkCommandBuffer& VulkanCommandBuffers::GetBuffer()
		{
			return _commandBuffers[_nextBuffer];
		}

		void VulkanCommandBuffers::SubmitBuffer()
		{
			GlobalDispatcher().trigger(GE::Gfx::vulkan_buffer_ready{ _commandBuffers[_nextBuffer] });
			_nextBuffer = (++_nextBuffer) % _commandBuffers.size();
		}
		
		void VulkanCommandBuffers::CreateCommandPool()
		{
			VkCommandPoolCreateInfo poolInfo = {};
			poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			poolInfo.queueFamilyIndex = _core.device->GetQueueFamiles(*_core.device, *_core.surface).graphicsFamily;
			poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

			VkResult result = vkCreateCommandPool(*_core.device, &poolInfo, nullptr, &_commandPool);
			GE_ASSERT(result == VK_SUCCESS, "Failed vkCreateCommandPool: {}", result);
			GE_UNUSED(result);
		}
	}
}