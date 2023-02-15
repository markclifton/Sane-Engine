#pragma once

#include <ge/gfx/Common.hpp>

namespace GE
{
	namespace Gfx
	{
		class VulkanBuffer : public VulkanGfxElement
		{
		public:
			void Create(VkBufferUsageFlags usage, std::size_t size);
			virtual void Destroy();

			virtual void Buffer(VkCommandBuffer& cmdBuffer, void* data, std::size_t size);

			operator VkBuffer() { return _buffer; }

		protected:
			uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
			void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

			VkBuffer _buffer;
			VkDeviceMemory _bufferMemory;

			VkBuffer _staging;
			VkDeviceMemory _stagingMemory;
		};

		class VulkanIndexBuffer : public VulkanBuffer
		{
		public:
			void Create(std::size_t size);
			void Bind(VkCommandBuffer& cmdBuffer);
		};

		class VulkanVertexBuffer : public VulkanBuffer
		{
		public:
			void Create(std::size_t size);
			void Bind(VkCommandBuffer& cmdBuffer);
		};

		class VulkanUniformBuffer : public VulkanBuffer
		{
		public:
			void Create(std::size_t size);
			virtual void Buffer(VkCommandBuffer& cmdBuffer, void* data, std::size_t size) override;
		};
	}
}