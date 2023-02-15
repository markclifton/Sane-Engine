#include <ge/gfx/Buffer.hpp>

#include <ge/gfx/Device.hpp>
#include <ge/utils/Common.hpp>

namespace GE
{
	namespace Gfx
	{
		uint32_t VulkanBuffer::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
		{
			VkPhysicalDeviceMemoryProperties memProperties;
			vkGetPhysicalDeviceMemoryProperties(*_core.device, &memProperties);

			for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
				if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
					return i;
				}
			}
			
			GE_ASSERT(0, "Failed to find memory type: {}", typeFilter);
			return -1;
		}

		void VulkanBuffer::Create(VkBufferUsageFlags usage, std::size_t size)
		{
			CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, _staging, _stagingMemory);
			CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _buffer, _bufferMemory);
		}

		void VulkanBuffer::Destroy()
		{
			vkDestroyBuffer(*_core.device, _buffer, nullptr);
			vkFreeMemory(*_core.device, _bufferMemory, nullptr);

			vkDestroyBuffer(*_core.device, _staging, nullptr);
			vkFreeMemory(*_core.device, _stagingMemory, nullptr);
		}

		void VulkanBuffer::Buffer(VkCommandBuffer& cmdBuffer, void* data, std::size_t size)
		{
			if (size == 0)
				return;

			void* stagingData;
			vkMapMemory(*_core.device, _stagingMemory, 0, size, 0, &stagingData);
			memcpy(stagingData, data, size);
			vkUnmapMemory(*_core.device, _stagingMemory);

			VkCommandBufferBeginInfo begin_info = {};
			begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			VkResult result = vkBeginCommandBuffer(cmdBuffer, &begin_info);
			GE_ASSERT(result == VK_SUCCESS, "Failed vkBeginCommandBuffer: {}", result);

			VkBufferCopy copyRegion{};
			copyRegion.size = size;
			vkCmdCopyBuffer(cmdBuffer, _staging, _buffer, 1, &copyRegion);

			VkSubmitInfo end_info = {};
			end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			end_info.commandBufferCount = 1;
			end_info.pCommandBuffers = &cmdBuffer;
			result = vkEndCommandBuffer(cmdBuffer);
			GE_ASSERT(result == VK_SUCCESS, "Failed vkEndCommandBuffer: {}", result);
			result = vkQueueSubmit(_core.device->GetGraphicsQueue(), 1, &end_info, VK_NULL_HANDLE);
			GE_ASSERT(result == VK_SUCCESS, "Failed vkQueueSubmit: {}", result);

			result = vkDeviceWaitIdle(*_core.device);
			GE_ASSERT(result == VK_SUCCESS, "Failed vkDeviceWaitIdle: {}", result);
		}

		void VulkanBuffer::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
			VkBufferCreateInfo bufferInfo{};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = size;
			bufferInfo.usage = usage;
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			if (vkCreateBuffer(*_core.device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
				GE_ERROR("Failed to create buffer!");
			}

			VkMemoryRequirements memRequirements;
			vkGetBufferMemoryRequirements(*_core.device, buffer, &memRequirements);

			VkMemoryAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocInfo.allocationSize = memRequirements.size;
			allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

			if (vkAllocateMemory(*_core.device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
				GE_ERROR("failed to allocate buffer memory!");
			}

			vkBindBufferMemory(*_core.device, buffer, bufferMemory, 0);
		}

		void VulkanIndexBuffer::Create(std::size_t size)
		{
			VulkanBuffer::Create(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, size);
		}

		void VulkanIndexBuffer::Bind(VkCommandBuffer& cmdBuffer)
		{
			vkCmdBindIndexBuffer(cmdBuffer, _buffer, 0, VK_INDEX_TYPE_UINT32);
		}

		void VulkanVertexBuffer::Create(std::size_t size)
		{
			VulkanBuffer::Create(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, size);
		}

		void VulkanVertexBuffer::Bind(VkCommandBuffer& cmdBuffer)
		{
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &_buffer, offsets);
		}


		void VulkanUniformBuffer::Create(std::size_t size)
		{
			CreateBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _buffer, _bufferMemory);
		}

		void VulkanUniformBuffer::Buffer(VkCommandBuffer& cmdBuffer, void* data, std::size_t size)
		{
			if (size == 0)
				return;

			VkCommandBufferBeginInfo begin_info = {};
			begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			VkResult result = vkBeginCommandBuffer(cmdBuffer, &begin_info);
			GE_ASSERT(result == VK_SUCCESS, "Failed vkBeginCommandBuffer: {}", result);

			vkCmdUpdateBuffer(cmdBuffer, _buffer, 0, size, data);

			VkSubmitInfo end_info = {};
			end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			end_info.commandBufferCount = 1;
			end_info.pCommandBuffers = &cmdBuffer;
			result = vkEndCommandBuffer(cmdBuffer);
			GE_ASSERT(result == VK_SUCCESS, "Failed vkEndCommandBuffer: {}", result);
			result = vkQueueSubmit(_core.device->GetGraphicsQueue(), 1, &end_info, VK_NULL_HANDLE);
			GE_ASSERT(result == VK_SUCCESS, "Failed vkQueueSubmit: {}", result);

			result = vkDeviceWaitIdle(*_core.device);
			GE_ASSERT(result == VK_SUCCESS, "Failed vkDeviceWaitIdle: {}", result);
		}
	}
}