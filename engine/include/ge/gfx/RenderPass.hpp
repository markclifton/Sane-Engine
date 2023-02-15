#pragma once

#include <memory>
#include <vector>

#include <ge/gfx/Common.hpp>

#include <ge/gfx/Framebuffer.hpp>
#include <ge/gfx/Pipeline.hpp>
#include <ge/gfx/Texture.hpp>

namespace GE
{
	namespace Gfx
	{
		class RenderPass : public VulkanGfxElement
		{
		public:
			virtual void Create() = 0;
			virtual void Destroy();
			void Configure(uint32_t numFrames, uint32_t width, uint32_t height);

			virtual void Resize(uint32_t width, uint32_t height);
			void Resize(VkExtent2D extent);

			inline VkRenderPass Raw() { 
				GE_ASSERT(_pipeline != nullptr, "Invalid pipeline");
				return _pipeline->_renderPass;
			}

			inline VkPipelineLayout Layout() {
				GE_ASSERT(_pipeline != nullptr, "Invalid pipeline");
				return _pipeline->_layout; 
			}

			//operator VkPipelineLayout();
			//operator VkRenderPass();

			void AddShader(const std::string& path);
			void AddShaderBinaryData(const std::string& data, VkShaderStageFlagBits type);
			void AddVertexAttribDescs(std::vector<VkVertexInputAttributeDescription> data);
			void AddVertexBufferDescs(std::vector<VkVertexInputBindingDescription> data);
			void AddDescriptorsSetLayouts(std::vector<VkDescriptorSetLayout> data);
			void AddPushConstants(std::vector<VkPushConstantRange> data);
			void SetClearFlag(bool value);

			virtual void Begin(VkCommandBuffer cmdBuffer) = 0;
			virtual void End(VkCommandBuffer cmdBuffer);

		public:
			uint32_t _numFrames{ 0 };
			uint32_t _width{ 0 };
			uint32_t _height{ 0 };

			PipelineCreationData _pipelineCreationData;
			std::unique_ptr<VulkanPipeline> _pipeline;
			VulkanFramebuffer _framebuffer;
		};

		class ScreenSpaceRenderPass : public RenderPass
		{
		public:
			virtual void Create() override;
			virtual void Destroy() override;

			void Configure();
			virtual void Begin(VkCommandBuffer cmdBuffer) override;
		};

		class FrameBufferRenderPass : public RenderPass
		{
		public:
			virtual void Create() override;
			virtual void Destroy() override;

			virtual void Begin(VkCommandBuffer cmdBuffer) override;

			VulkanTexture& GetColorTexture(uint32_t index);
			VulkanTexture& GetDepthTexture(uint32_t index);

		private:
			std::vector<VulkanTexture> _colorTextures;
			std::vector<VulkanTexture> _depthTextures;
		};

		class MSAAFrameBufferRenderPass : public RenderPass
		{
		public:
			virtual void Create() override;
			virtual void Destroy() override;

			virtual void Begin(VkCommandBuffer cmdBuffer) override;

			VulkanTexture& GetColorTexture(uint32_t index);
			VulkanTexture& GetDepthTexture(uint32_t index);

		private:
			std::vector<VulkanTexture> _colorTextures;
			std::vector<VulkanTexture> _resolveTextures;
			std::vector<VulkanTexture> _depthTextures;
		};

		class OmniDirectionShadowRenderPass : public RenderPass
		{
		public:
			virtual void Create() override;
			virtual void Destroy() override;

			virtual void Begin(VkCommandBuffer cmdBuffer) override;

			VulkanTexture& GetShadowTexture(uint32_t index);
			VulkanTexture& GetColorTexture(uint32_t index);
			VulkanTexture& GetDepthTexture(uint32_t index);

		public:
			std::vector<VulkanTexture> _shadowMaps;
			std::vector<VulkanTexture> _colorTextures;
			std::vector<VulkanTexture> _depthTextures;
		};

		class GBufferRenderPass : public RenderPass
		{
		public:
			virtual void Create() override;
			virtual void Destroy() override;

			virtual void Begin(VkCommandBuffer cmdBuffer) override;

			struct GBuffer
			{
				VulkanTexture positon;
				VulkanTexture normal;
				VulkanTexture albedo;
				VulkanTexture depth;
			};
			GBuffer* GetGBuffer(uint32_t index);
		private:
			std::vector<GBuffer> _gbuffers;
		};
	}
}
