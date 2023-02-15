#include <ge/gfx/Descriptor.hpp>

#include <cassert>
#include <stdexcept>
#include <vector>
#include <unordered_map>

#include <ge/gfx/Device.hpp>
#include <ge/utils/Common.hpp>

namespace GE
{
	namespace Gfx
	{
		void VulkanDescriptorsPool::Create(std::vector<VkDescriptorPoolSize> sizes, VkDescriptorPoolCreateFlags flags, uint32_t maxSets)
		{
			maxSets *= GetNumFrames();
			_descriptorPoolCreateFlags = flags;

			VkDescriptorPoolCreateInfo descriptorPoolInfo{};
			descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(sizes.size());
			descriptorPoolInfo.pPoolSizes = sizes.data();
			descriptorPoolInfo.maxSets = maxSets;
			descriptorPoolInfo.flags = flags;

			VkResult result = vkCreateDescriptorPool(*_core.device, &descriptorPoolInfo, nullptr, &_descriptorPool);
			GE_ASSERT(result == VK_SUCCESS, "Failed to create descriptor pool: {}", result);
			GE_UNUSED(result);
		}
		
		int VulkanDescriptorsPool::CreateNewDescriptorSet(std::vector<VulkanDescriptorSetBinding> data)
		{
			std::vector<VkDescriptorSetLayoutBinding> bindings;
			std::vector<VkDescriptorBindingFlags> flags;
			bool variableLength = false;

			for (auto& datapoint : data)
			{
				bindings.push_back(datapoint.binding);
				flags.push_back(datapoint.flags);

				variableLength |= static_cast<bool>(datapoint.flags & VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT);
			}

			_descriptorSetsInfo.push_back({});
			uint32_t index = static_cast<uint32_t>(_descriptorSetsInfo.size() - 1);

			VkDescriptorSetLayoutBindingFlagsCreateInfo extended_info{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT, nullptr };
			extended_info.bindingCount = static_cast<uint32_t>(flags.size());
			extended_info.pBindingFlags = flags.data();

			VkDescriptorSetLayoutCreateInfo layoutInfo{};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
			layoutInfo.pBindings = bindings.data();
			layoutInfo.flags = _descriptorPoolCreateFlags;
			layoutInfo.pNext = &extended_info;

			VkResult result = vkCreateDescriptorSetLayout(*_core.device, &layoutInfo, nullptr, &_descriptorSetsInfo[index].layout);
			GE_ASSERT(result == VK_SUCCESS, "Failed to create descriptor set layout: {}", result);

			VkDescriptorSetAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = _descriptorPool;
			allocInfo.descriptorSetCount = 1;
			allocInfo.pSetLayouts = &_descriptorSetsInfo[index].layout;

			uint32_t variableDescCounts = 0;
			VkDescriptorSetVariableDescriptorCountAllocateInfo variableDescriptorCountAllocInfo = {};
			if (variableLength && !bindings.empty())
			{
				variableDescCounts = bindings.back().descriptorCount;

				variableDescriptorCountAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
				variableDescriptorCountAllocInfo.descriptorSetCount = 1;
				variableDescriptorCountAllocInfo.pDescriptorCounts = &variableDescCounts;

				allocInfo.pNext = &variableDescriptorCountAllocInfo;
			}

			_descriptorSetsInfo[index].sets.resize(GetNumFrames());
			_descriptorSetsInfo[index].pendingChanges.resize(GetNumFrames());
			for (uint32_t i = 0; i < GetNumFrames(); ++i)
			{
				result = vkAllocateDescriptorSets(*_core.device, &allocInfo, &_descriptorSetsInfo[index].sets[i]);
				GE_ASSERT(result == VK_SUCCESS, "Failed to allocate descriptor sets: {}", result);
			}

			return index;
		}

		void VulkanDescriptorsPool::Destroy()
		{
			vkDestroyDescriptorPool(*_core.device, _descriptorPool, nullptr);
			for (auto& descriptorSetInfo : _descriptorSetsInfo)
			{
				vkDestroyDescriptorSetLayout(*_core.device, descriptorSetInfo.layout, nullptr);
			}
			_descriptorSetsInfo.clear();
		}

		void VulkanDescriptorsPool::UpdateDescriptorBuffer(int setIndex, int index, uint32_t binding, VkDescriptorBufferInfo* bufferInfo)
		{
			VkWriteDescriptorSet descriptorWrite{};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = _descriptorSetsInfo[setIndex].sets[index];
			descriptorWrite.dstBinding = binding;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite.pBufferInfo = bufferInfo;

			_descriptorSetsInfo[setIndex].pendingChanges[index].push_back(descriptorWrite);
		}

		void VulkanDescriptorsPool::UpdateDescriptorImage(int setIndex, int index, uint32_t binding, VkDescriptorImageInfo* imageInfo, int count)
		{
			VkWriteDescriptorSet descriptorWrite{};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = _descriptorSetsInfo[setIndex].sets[index];
			descriptorWrite.dstBinding = binding;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrite.descriptorCount = count;
			descriptorWrite.pImageInfo = imageInfo;

			_descriptorSetsInfo[setIndex].pendingChanges[index].push_back(descriptorWrite);
		}

		void VulkanDescriptorsPool::WriteDescriptorSet(int setIndex, int index)
		{
			vkUpdateDescriptorSets(*_core.device, static_cast<uint32_t>(_descriptorSetsInfo[setIndex].pendingChanges[index].size()), _descriptorSetsInfo[setIndex].pendingChanges[index].data(), 0, nullptr);
			_descriptorSetsInfo[setIndex].pendingChanges[index].clear();
		}

		VkDescriptorSet& VulkanDescriptorsPool::GetDescriptorSet(int setIndex, int index)
		{
			return _descriptorSetsInfo[setIndex].sets[index];
		}

		VkDescriptorSetLayout VulkanDescriptorsPool::GetDescriptorSetLayout(int setIndex)
		{
			return _descriptorSetsInfo[setIndex].layout;
		}
	}
}