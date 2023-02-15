#pragma once

#include <string>

#include <ge/gfx/Common.hpp>

namespace GE
{
	namespace Gfx
	{
		struct ShaderData
		{
			VkShaderStageFlagBits type;
			std::string data;
			std::string path;
		};

		VkShaderStageFlagBits GetShaderType(const std::string& path);

		struct PipelineCreationData
		{
			bool clearFlag = false;
			bool multisampled = false;
			std::vector<ShaderData> shaders;

			std::vector<VkVertexInputAttributeDescription> attribs;
			std::vector<VkVertexInputBindingDescription> bufferDescs;
			std::vector<VkPushConstantRange> pushConstants;
			std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
		};

		class VulkanPipeline : public VulkanGfxElement
		{
			friend class VulkanWindow;
			friend class VulkanFramebuffer;
			friend class VulkanCommandBuffers;
		public:
			VulkanPipeline() = default;
			virtual ~VulkanPipeline() = default;

			virtual void Create(PipelineCreationData& data) = 0;
			virtual void CreateRenderPass(PipelineCreationData& data) = 0;
			virtual void Destroy();


			VkPipeline Pipeline() { return _pipeline; }
			VkPipelineLayout PipelineLayout() { return _layout; }

			//operator VkPipeline() { return _pipeline; }
			//operator VkPipelineLayout() { return _layout; }
			VkDescriptorSetLayout _descriptorSetLayout;

			VkPipelineLayout _layout;
			VkRenderPass _renderPass;
			VkPipeline _pipeline;
		};

		class VulkanScreenSpacePipeline : public VulkanPipeline
		{
		public:
			virtual void Create(PipelineCreationData& data) override;
			virtual void CreateRenderPass(PipelineCreationData& data) override;
		};

		class VulkanOffscreenPipeline : public VulkanPipeline
		{
		public:
			virtual void Create(PipelineCreationData& data) override;
			virtual void CreateRenderPass(PipelineCreationData& data) override;
		};

		class VulkanMSAAOffscreenPipeline : public VulkanPipeline
		{
		public:
			virtual void Create(PipelineCreationData& data) override;
			virtual void CreateRenderPass(PipelineCreationData& data) override;
		};

		class VulkanShadowPipeline : public VulkanPipeline
		{
		public:
			virtual void Create(PipelineCreationData& data) override;
			virtual void CreateRenderPass(PipelineCreationData& data) override;
		};

		class VulkanMultipassShadowPipeline : public VulkanPipeline
		{
		public:
			virtual void Create(PipelineCreationData& data) override;
			virtual void CreateRenderPass(PipelineCreationData& data) override;
		};

		class VulkanGBufferPipeline : public VulkanPipeline
		{
		public:
			virtual void Create(PipelineCreationData& data) override;
			virtual void CreateRenderPass(PipelineCreationData& data) override;
		};
	}
}