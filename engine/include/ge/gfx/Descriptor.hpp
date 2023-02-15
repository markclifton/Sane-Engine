#pragma once

#include <memory>
#include <vector>

#include <ge/gfx/Common.hpp>

namespace GE
{
    namespace Gfx
	{
		struct VulkanDescriptorSetBinding
		{
			VkDescriptorSetLayoutBinding binding;
			VkDescriptorBindingFlags flags;
		};

		class VulkanDescriptorsPool : public GE::Gfx::VulkanGfxElement
		{
			struct DescriptorSetInfo
			{
				std::vector<VkDescriptorSet> sets;
				VkDescriptorSetLayout layout;

				std::vector<std::vector<VkWriteDescriptorSet>> pendingChanges;
			};

		public:
			void Create(std::vector<VkDescriptorPoolSize> sizes, VkDescriptorPoolCreateFlags flags, uint32_t maxSets = 1);
			int CreateNewDescriptorSet(std::vector<VulkanDescriptorSetBinding> bindings);
			void Destroy();

			void UpdateDescriptorBuffer(int setIndex, int index, uint32_t binding, VkDescriptorBufferInfo* bufferInfo);
			void UpdateDescriptorImage(int setIndex, int index, uint32_t binding, VkDescriptorImageInfo* imageInfo, int count);
			void WriteDescriptorSet(int setIndex, int index);

			VkDescriptorSet& GetDescriptorSet(int setIndex, int index);
			VkDescriptorSetLayout GetDescriptorSetLayout(int setIndex);

			inline operator VkDescriptorPool() { return _descriptorPool; }

		private:
			void LoadBindings();

			VkDescriptorPool _descriptorPool;
			VkDescriptorPoolCreateFlags _descriptorPoolCreateFlags;

			std::vector<DescriptorSetInfo> _descriptorSetsInfo;
		};
    }
}