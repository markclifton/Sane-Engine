#pragma once

#include <ge/gfx/Buffer.hpp>

class Drawable
{
public:
	virtual bool Buffer(VkCommandBuffer cmdBuffer, VkDescriptorSet* descriptor, uint32_t frameIndex) = 0;
	virtual void Draw(VkCommandBuffer cmdBuffer, VkDescriptorSet* descriptor, uint32_t frameIndex) = 0;

	bool loaded = false;

	GE::Gfx::VulkanVertexBuffer verticesBuffer;
	GE::Gfx::VulkanVertexBuffer texCoordsBuffer;
	GE::Gfx::VulkanVertexBuffer normalsBuffer;
	GE::Gfx::VulkanVertexBuffer texIdsBuffer;

	uint32_t numVerts{ 0 };
};

struct Object
{
	Drawable* drawable;
};