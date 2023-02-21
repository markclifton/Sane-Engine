#include <ge/gfx/RenderPass.hpp>

#include <array>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

namespace GE
{
	namespace Gfx
	{
		void RenderPass::Configure(uint32_t numFrames, uint32_t width, uint32_t height)
		{
			_numFrames = numFrames;
			_width = width;
			_height = height;
		}

		void RenderPass::Destroy()
		{
			_framebuffer.Destroy();

			_pipeline->Destroy();
			_pipeline = nullptr;
		}

		void RenderPass::Resize(uint32_t width, uint32_t height)
		{
			_width = width;
			_height = height;
			
			Destroy();
			Create();
		}

		void RenderPass::Resize(VkExtent2D extent)
		{
			Resize(extent.width, extent.height);
		}

/*
		RenderPass::operator VkPipelineLayout() 
		{
			GE_ASSERT(_pipeline != nullptr, "Invalid pipeline");
			return _pipeline->_layout; 
		}

		RenderPass::operator VkRenderPass()
		{
			GE_ASSERT(_pipeline != nullptr, "Invalid pipeline");
			return _pipeline->_renderPass;
		}
*/
		void RenderPass::AddShader(const std::string& path)
		{
			GE_ASSERT(!path.empty(), "Invalid shader path");

			ShaderData newShader{};
			newShader.path = path;
			newShader.type = GetShaderType(path);
			_pipelineCreationData.shaders.push_back(newShader);
		}

		void RenderPass::AddShaderBinaryData(const std::string& data, VkShaderStageFlagBits type)
		{
			GE_ASSERT(!data.empty(), "Invalid shader data");

			ShaderData newShader{};
			newShader.data = data;
			newShader.type = type;

			_pipelineCreationData.shaders.push_back(newShader);
		}

		void RenderPass::AddVertexAttribDescs(std::vector<VkVertexInputAttributeDescription> data)
		{
			_pipelineCreationData.attribs = data;
		}

		void RenderPass::AddVertexBufferDescs(std::vector<VkVertexInputBindingDescription> data)
		{
			_pipelineCreationData.bufferDescs = data;
		}

		void RenderPass::AddDescriptorsSetLayouts(std::vector<VkDescriptorSetLayout> data)
		{
			_pipelineCreationData.descriptorSetLayouts = data;
		}

		void RenderPass::AddPushConstants(std::vector<VkPushConstantRange> data)
		{
			_pipelineCreationData.pushConstants = data;
		}
		
		void RenderPass::SetClearFlag(bool value)
		{
			_pipelineCreationData.clearFlag = value;
		}

		void RenderPass::End(VkCommandBuffer cmdBuffer)
		{
			vkCmdEndRenderPass(cmdBuffer);

			VkResult result = vkEndCommandBuffer(cmdBuffer);
			GE_ASSERT(result == VK_SUCCESS, "Failed vkEndCommandBuffer: {}", result);
			GE_UNUSED(result);
		}

		void ScreenSpaceRenderPass::Create()
		{
			_pipeline = std::make_unique<VulkanScreenSpacePipeline>();
			_pipeline->Create(_pipelineCreationData);
			_framebuffer.Create(_pipeline.get()->_renderPass);
		}

		void ScreenSpaceRenderPass::Destroy()
		{
			_framebuffer.Destroy();

			_pipeline->Destroy();
			_pipeline = nullptr;
		}

		void ScreenSpaceRenderPass::Configure()
		{
			auto extent = _core.swapchain->GetExtent();
			RenderPass::Configure(GetNumFrames(), extent.width, extent.height);
		}

		void ScreenSpaceRenderPass::Begin(VkCommandBuffer cmdBuffer)
		{
			vkResetCommandBuffer(cmdBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

			VkCommandBufferBeginInfo bufferBeginInfo = {};
			bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

			VkResult result = vkBeginCommandBuffer(cmdBuffer, &bufferBeginInfo);
			GE_ASSERT(result == VK_SUCCESS, "Failed vkBeginCommandBuffer: {}", result);
			GE_UNUSED(result);

			std::array<VkClearValue, 2> clearValues{};
			clearValues[0].color = { {0.2f, 0.3f, 0.8f, 1.0f} };
			clearValues[1].depthStencil = { 1.0f, 0 };

			VkRenderPassBeginInfo renderPassBeginInfo = {};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.renderPass = _pipeline->_renderPass;
			renderPassBeginInfo.renderArea.offset = { 0,0 };
			renderPassBeginInfo.renderArea.extent = { _width, _height };
			renderPassBeginInfo.pClearValues = clearValues.data();
			renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
			renderPassBeginInfo.framebuffer = _framebuffer.Get(GetCurrentFrame());

			vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline->Pipeline());

			VkViewport viewport{};
			viewport.x = 0.f;
			viewport.y = static_cast<float>(_height);
			viewport.width = static_cast<float>(_width);
			viewport.height = -static_cast<float>(_height);
			viewport.minDepth = 0.f;
			viewport.maxDepth = 1.f;
			VkRect2D scissor{ {0,0}, {_width, _height} };

			vkCmdSetViewport(cmdBuffer, 0, 1, &viewport); 
			vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
		}

		void ScreenSpaceRenderPass::SetInput(VulkanDescriptorsPool& pool, int setIndex, int index, VulkanTexture& inputTexture) {
			VkDescriptorImageInfo gColor{ inputTexture.Sampler(), inputTexture.ImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
			pool.UpdateDescriptorImage(setIndex, index, 0u, &gColor, 1);
			pool.WriteDescriptorSet(setIndex, index);
		};

		void FrameBufferRenderPass::Create()
		{
			_pipeline = std::make_unique<VulkanOffscreenPipeline>();
			_pipeline->Create(_pipelineCreationData);

			VkImageCreateInfo colorImageInfo{};
			{
				colorImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
				colorImageInfo.imageType = VK_IMAGE_TYPE_2D;
				colorImageInfo.extent = { static_cast<uint32_t>(_width), static_cast<uint32_t>(_height), 1 };
				colorImageInfo.mipLevels = 1;
				colorImageInfo.arrayLayers = 1;
				colorImageInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
				colorImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
				colorImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				colorImageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
				colorImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
				colorImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
				colorImageInfo.flags = 0; // Optional
			}

			VkImageViewCreateInfo viewCreateInfo = {};
			{
				viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
				viewCreateInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
				viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				viewCreateInfo.subresourceRange.baseMipLevel = 0;
				viewCreateInfo.subresourceRange.levelCount = 1;
				viewCreateInfo.subresourceRange.baseArrayLayer = 0;
				viewCreateInfo.subresourceRange.layerCount = 1;
			}

			VkImageCreateInfo depthImageInfo{};
			{
				depthImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
				depthImageInfo.imageType = VK_IMAGE_TYPE_2D;
				depthImageInfo.extent.width = _width;
				depthImageInfo.extent.height = _height;
				depthImageInfo.extent.depth = 1;
				depthImageInfo.mipLevels = 1;
				depthImageInfo.arrayLayers = 1;
				depthImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
				depthImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
				depthImageInfo.format = FindDepthFormat();
				depthImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
				depthImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				depthImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
				depthImageInfo.flags = 0;
			}

			VkImageViewCreateInfo depthViewCreateInfo = {};
			{
				depthViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				depthViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
				depthViewCreateInfo.format = FindDepthFormat();
				depthViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY };
				depthViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				depthViewCreateInfo.subresourceRange.baseMipLevel = 0;
				depthViewCreateInfo.subresourceRange.levelCount = 1;
				depthViewCreateInfo.subresourceRange.baseArrayLayer = 0;
				depthViewCreateInfo.subresourceRange.layerCount = 1;
				depthViewCreateInfo.flags = 0;
			}

			for (uint32_t i = 0; i < _numFrames; ++i)
			{
				_colorTextures.push_back({});
				_depthTextures.push_back({});

				_colorTextures.back().Create(colorImageInfo, viewCreateInfo);
				_depthTextures.back().Create(depthImageInfo, depthViewCreateInfo);

				_framebuffer.Create(_pipeline.get()->_renderPass, { _colorTextures.back().ImageView(), _depthTextures.back().ImageView() }, { _width, _height });
			}
		}

		void FrameBufferRenderPass::Destroy()
		{
			for (uint32_t i = 0; i < _numFrames; ++i)
			{
				_colorTextures[i].Destroy();
				_depthTextures[i].Destroy();
			}
			_colorTextures.clear();
			_depthTextures.clear();

			RenderPass::Destroy();
		}

		void FrameBufferRenderPass::Begin(VkCommandBuffer cmdBuffer)
		{
			vkResetCommandBuffer(cmdBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

			VkCommandBufferBeginInfo bufferBeginInfo = {};
			bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

			VkResult result = vkBeginCommandBuffer(cmdBuffer, &bufferBeginInfo);
			GE_ASSERT(result == VK_SUCCESS, "Failed vkBeginCommandBuffer: {}", result);
			GE_UNUSED(result);

			std::array<VkClearValue, 2> clearValues{};
			clearValues[0].color = { {0.f, 0.f, 0.f, 0.f} };
			clearValues[1].depthStencil = { 1.0f, 0 };

			VkRenderPassBeginInfo renderPassBeginInfo = {};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.renderPass = _pipeline->_renderPass;
			renderPassBeginInfo.renderArea.offset = { 0,0 };
			renderPassBeginInfo.renderArea.extent = { _width, _height };
			renderPassBeginInfo.pClearValues = clearValues.data();
			renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
			renderPassBeginInfo.framebuffer = _framebuffer.Get(GetCurrentFrame());

			vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline->Pipeline());

			VkViewport viewport{};
			viewport.x = 0.f;
			viewport.y = static_cast<float>(_height);
			viewport.width = static_cast<float>(_width);
			viewport.height = -static_cast<float>(_height);
			viewport.minDepth = 0.f;
			viewport.maxDepth = 1.f;
			VkRect2D scissor{ {0,0}, {_width, _height} };

			vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
			vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
		}

		VulkanTexture& FrameBufferRenderPass::GetColorTexture(uint32_t index)
		{
			GE_ASSERT(index < _colorTextures.size(), "Invalid index");
			return _colorTextures[index];
		}

		VulkanTexture& FrameBufferRenderPass::GetDepthTexture(uint32_t index)
		{
			GE_ASSERT(index < _depthTextures.size(), "Invalid index");
			return _depthTextures[index];
		}

		void MSAAFrameBufferRenderPass::Create()
		{
			_pipeline = std::make_unique<VulkanMSAAOffscreenPipeline>();
			_pipelineCreationData.multisampled = true;
			_pipeline->Create(_pipelineCreationData);

			VkImageCreateInfo colorImageInfo{};
			{
				colorImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
				colorImageInfo.imageType = VK_IMAGE_TYPE_2D;
				colorImageInfo.extent = { static_cast<uint32_t>(_width), static_cast<uint32_t>(_height), 1 };
				colorImageInfo.mipLevels = 1;
				colorImageInfo.arrayLayers = 1;
				colorImageInfo.format = _core.swapchain->GetFormat();
				colorImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
				colorImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				colorImageInfo.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
				colorImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
				colorImageInfo.samples = VK_SAMPLE_COUNT_8_BIT;
				colorImageInfo.flags = 0; // Optional
			}

			VkImageViewCreateInfo viewCreateInfo = {};
			{
				viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
				viewCreateInfo.format = colorImageInfo.format;
				viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
				viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
				viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
				viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;
				viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				viewCreateInfo.subresourceRange.baseMipLevel = 0;
				viewCreateInfo.subresourceRange.levelCount = 1;
				viewCreateInfo.subresourceRange.baseArrayLayer = 0;
				viewCreateInfo.subresourceRange.layerCount = 1;
			}

			VkImageCreateInfo resolveImageInfo{};
			{
				resolveImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
				resolveImageInfo.imageType = VK_IMAGE_TYPE_2D;
				resolveImageInfo.extent = { static_cast<uint32_t>(_width), static_cast<uint32_t>(_height), 1 };
				resolveImageInfo.mipLevels = 1;
				resolveImageInfo.arrayLayers = 1;
				resolveImageInfo.format = _core.swapchain->GetFormat();
				resolveImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
				resolveImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				resolveImageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
				resolveImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
				resolveImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
				resolveImageInfo.flags = 0; // Optional
			}

			VkImageViewCreateInfo resolveViewCreateInfo = {};
			{
				resolveViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				resolveViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
				resolveViewCreateInfo.format = colorImageInfo.format;
				resolveViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
				resolveViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
				resolveViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
				resolveViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;
				resolveViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				resolveViewCreateInfo.subresourceRange.baseMipLevel = 0;
				resolveViewCreateInfo.subresourceRange.levelCount = 1;
				resolveViewCreateInfo.subresourceRange.baseArrayLayer = 0;
				resolveViewCreateInfo.subresourceRange.layerCount = 1;
			}

			VkImageCreateInfo depthImageInfo{};
			{
				depthImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
				depthImageInfo.imageType = VK_IMAGE_TYPE_2D;
				depthImageInfo.extent.width = _width;
				depthImageInfo.extent.height = _height;
				depthImageInfo.extent.depth = 1;
				depthImageInfo.mipLevels = 1;
				depthImageInfo.arrayLayers = 1;
				depthImageInfo.samples = VK_SAMPLE_COUNT_8_BIT;
				depthImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
				depthImageInfo.format = FindDepthFormat();
				depthImageInfo.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
				depthImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				depthImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
				depthImageInfo.flags = 0;
			}

			VkImageViewCreateInfo depthViewCreateInfo = {};
			{
				depthViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				depthViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
				depthViewCreateInfo.format = FindDepthFormat();
				depthViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
				depthViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
				depthViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
				depthViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;
				depthViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				depthViewCreateInfo.subresourceRange.baseMipLevel = 0;
				depthViewCreateInfo.subresourceRange.levelCount = 1;
				depthViewCreateInfo.subresourceRange.baseArrayLayer = 0;
				depthViewCreateInfo.subresourceRange.layerCount = 1;
				depthViewCreateInfo.flags = 0;
			}

			for (uint32_t i = 0; i < _numFrames; ++i)
			{
				_colorTextures.push_back({});
				_resolveTextures.push_back({});
				_depthTextures.push_back({});

				_resolveTextures.back().Create(colorImageInfo, viewCreateInfo);
				_colorTextures.back().Create(resolveImageInfo, resolveViewCreateInfo);
				_depthTextures.back().Create(depthImageInfo, depthViewCreateInfo);

				_framebuffer.Create(_pipeline.get()->_renderPass, { _resolveTextures.back().ImageView(), _colorTextures.back().ImageView(), _depthTextures.back().ImageView() }, { _width, _height });
			}
		}

		void MSAAFrameBufferRenderPass::Destroy()
		{
			for (uint32_t i = 0; i < _numFrames; ++i)
			{
				_colorTextures[i].Destroy();
				_resolveTextures[i].Destroy();
				_depthTextures[i].Destroy();
			}
			_colorTextures.clear();
			_resolveTextures.clear();
			_depthTextures.clear();

			RenderPass::Destroy();
		}

		void MSAAFrameBufferRenderPass::Begin(VkCommandBuffer cmdBuffer)
		{
			vkResetCommandBuffer(cmdBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

			VkCommandBufferBeginInfo bufferBeginInfo = {};
			bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

			VkResult result = vkBeginCommandBuffer(cmdBuffer, &bufferBeginInfo);
			GE_ASSERT(result == VK_SUCCESS, "Failed vkBeginCommandBuffer: {}", result);
			GE_UNUSED(result);

			std::array<VkClearValue, 3> clearValues{};
			clearValues[0].color = { {0.f, 0.f, 0.f, 0.f} };
			clearValues[1].color = { {0.f, 0.f, 0.f, 0.f} };
			clearValues[2].depthStencil = { 1.0f, 0 };

			VkRenderPassBeginInfo renderPassBeginInfo = {};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.renderPass = _pipeline->_renderPass;
			renderPassBeginInfo.renderArea.offset = { 0,0 };
			renderPassBeginInfo.renderArea.extent = { _width, _height };
			renderPassBeginInfo.pClearValues = clearValues.data();
			renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
			renderPassBeginInfo.framebuffer = _framebuffer.Get(GetCurrentFrame());

			vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline->Pipeline());

			VkViewport viewport{};
			viewport.x = 0.f;
			viewport.y = static_cast<float>(_height);
			viewport.width = static_cast<float>(_width);
			viewport.height = -static_cast<float>(_height);
			viewport.minDepth = 0.f;
			viewport.maxDepth = 1.f;
			VkRect2D scissor{ {0,0}, {_width, _height} };

			vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
			vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
		}

		VulkanTexture& MSAAFrameBufferRenderPass::GetColorTexture(uint32_t index)
		{
			GE_ASSERT(index < _colorTextures.size(), "Invalid index");
			return _colorTextures[index];
		}

		VulkanTexture& MSAAFrameBufferRenderPass::GetDepthTexture(uint32_t index)
		{
			GE_ASSERT(index < _depthTextures.size(), "Invalid index");
			return _depthTextures[index];
		}

		void OmniDirectionShadowRenderPass::Create()
		{
			_pipeline = std::make_unique<VulkanMultipassShadowPipeline>();
			_pipeline->Create(_pipelineCreationData);

			VkImageCreateInfo shadowMapImageInfo{};
			{
				shadowMapImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
				shadowMapImageInfo.imageType = VK_IMAGE_TYPE_2D;
				shadowMapImageInfo.format = VK_FORMAT_R32_SFLOAT;
				shadowMapImageInfo.extent = { _width, _height, 1 };
				shadowMapImageInfo.mipLevels = 1;
				shadowMapImageInfo.arrayLayers = 6;
				shadowMapImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
				shadowMapImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
				shadowMapImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
				shadowMapImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
				shadowMapImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				shadowMapImageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
			}

			VkImageViewCreateInfo shadowMapViewCreateInfo = {};
			{
				shadowMapViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				shadowMapViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
				shadowMapViewCreateInfo.format = VK_FORMAT_R32_SFLOAT;
				shadowMapViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R };
				shadowMapViewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
				shadowMapViewCreateInfo.subresourceRange.layerCount = 6;
				shadowMapViewCreateInfo.flags = 0;
			}

			for (uint32_t i = 0; i < _numFrames; ++i)
			{
				_shadowMaps.push_back({});
				_shadowMaps.back().Create(shadowMapImageInfo, shadowMapViewCreateInfo);
			}

			VkImageCreateInfo colorImageInfo{};
			{
				colorImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
				colorImageInfo.imageType = VK_IMAGE_TYPE_2D;
				colorImageInfo.extent.width = static_cast<uint32_t>(_width);
				colorImageInfo.extent.height = static_cast<uint32_t>(_height);
				colorImageInfo.extent.depth = 1;
				colorImageInfo.mipLevels = 1;
				colorImageInfo.arrayLayers = 1;
				colorImageInfo.format = VK_FORMAT_R32_SFLOAT;
				colorImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
				colorImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				colorImageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
				colorImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
				colorImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
				colorImageInfo.flags = 0; // Optional
			}

			VkImageViewCreateInfo viewCreateInfo = {};
			{
				viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
				viewCreateInfo.format = VK_FORMAT_R32_SFLOAT;
				viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				viewCreateInfo.subresourceRange.baseMipLevel = 0;
				viewCreateInfo.subresourceRange.levelCount = 1;
				viewCreateInfo.subresourceRange.baseArrayLayer = 0;
				viewCreateInfo.subresourceRange.layerCount = 1;
			}

			VkImageCreateInfo depthImageInfo{};
			{
				depthImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
				depthImageInfo.imageType = VK_IMAGE_TYPE_2D;
				depthImageInfo.extent.width = _width;
				depthImageInfo.extent.height = _height;
				depthImageInfo.extent.depth = 1;
				depthImageInfo.mipLevels = 1;
				depthImageInfo.arrayLayers = 1;
				depthImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
				depthImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
				depthImageInfo.format = FindDepthFormat();
				depthImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
				depthImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				depthImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
				depthImageInfo.flags = 0;
			}

			VkImageViewCreateInfo depthViewCreateInfo = {};
			{
				depthViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				depthViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
				depthViewCreateInfo.format = FindDepthFormat();
				depthViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY };
				depthViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				depthViewCreateInfo.subresourceRange.baseMipLevel = 0;
				depthViewCreateInfo.subresourceRange.levelCount = 1;
				depthViewCreateInfo.subresourceRange.baseArrayLayer = 0;
				depthViewCreateInfo.subresourceRange.layerCount = 1;
				depthViewCreateInfo.flags = 0;
			}

			for (uint32_t i = 0; i < _numFrames; ++i)
			{
				_colorTextures.push_back({});
				_depthTextures.push_back({});

				_colorTextures.back().Create(colorImageInfo, viewCreateInfo);
				_depthTextures.back().Create(depthImageInfo, depthViewCreateInfo);

				_framebuffer.Create(_pipeline.get()->_renderPass, { _colorTextures.back().ImageView(), _depthTextures.back().ImageView() }, { _width, _height });
			}
		}

		void OmniDirectionShadowRenderPass::Destroy()
		{
			for (uint32_t i = 0; i < _colorTextures.size(); ++i) {
				_colorTextures[i].Destroy();
				_depthTextures[i].Destroy();
				_shadowMaps[i].Destroy();
			}
			_colorTextures.clear();
			_depthTextures.clear();
			_shadowMaps.clear();

			RenderPass::Destroy();
		}

		void OmniDirectionShadowRenderPass::Begin(VkCommandBuffer cmdBuffer)
		{
			vkResetCommandBuffer(cmdBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

			VkCommandBufferBeginInfo bufferBeginInfo = {};
			bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

			VkResult result = vkBeginCommandBuffer(cmdBuffer, &bufferBeginInfo);
			GE_ASSERT(result == VK_SUCCESS, "Failed vkBeginCommandBuffer: {}", result);
			GE_UNUSED(result);

			std::array<VkClearValue, 2> clearValues{};
			clearValues[0].color = { { 0.f, 0.f, 0.f, 1.f } };
			clearValues[1].depthStencil = { 1.0f, 0 };

			VkRenderPassBeginInfo renderPassBeginInfo = {};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.renderPass = _pipeline->_renderPass;
			renderPassBeginInfo.renderArea.offset = { 0,0 };
			renderPassBeginInfo.renderArea.extent = { _width, _height };
			renderPassBeginInfo.pClearValues = clearValues.data();
			renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
			renderPassBeginInfo.framebuffer = _framebuffer.Get(0);

			vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline->Pipeline());

			VkViewport viewport{};
			viewport.x = 0.f;
			viewport.y = static_cast<float>(_height);
			viewport.width = static_cast<float>(_width);
			viewport.height = -static_cast<float>(_height);
			viewport.minDepth = 0.f;
			viewport.maxDepth = 1.f;
			VkRect2D scissor{ {0,0}, {_width, _height} };

			vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
			vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
			vkCmdSetDepthBias(cmdBuffer, 1.25f, 0.0f, 1.75f);
		}

		VulkanTexture& OmniDirectionShadowRenderPass::GetShadowTexture(uint32_t index)
		{
			GE_ASSERT(index < _shadowMaps.size(), "Invalid index");
			return _shadowMaps[index];
		}

		VulkanTexture& OmniDirectionShadowRenderPass::GetColorTexture(uint32_t index)
		{
			GE_ASSERT(index < _colorTextures.size(), "Invalid index");
			return _colorTextures[index];
		}

		VulkanTexture& OmniDirectionShadowRenderPass::GetDepthTexture(uint32_t index)
		{
			GE_ASSERT(index < _depthTextures.size(), "Invalid index");
			return _depthTextures[index];
		}

		void GBufferRenderPass::Create()
		{
			_pipeline = std::make_unique<VulkanGBufferPipeline>();
			_pipeline->Create(_pipelineCreationData);

			VkImageCreateInfo colorImageInfo{};
			{
				colorImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
				colorImageInfo.imageType = VK_IMAGE_TYPE_2D;
				colorImageInfo.extent = { static_cast<uint32_t>(_width), static_cast<uint32_t>(_height), 1 };
				colorImageInfo.mipLevels = 1;
				colorImageInfo.arrayLayers = 1;
				colorImageInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
				colorImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
				colorImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				colorImageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
				colorImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
				colorImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
				colorImageInfo.flags = 0; // Optional
			}

			VkImageViewCreateInfo viewCreateInfo = {};
			{
				viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
				viewCreateInfo.format = VK_FORMAT_R16G16B16A16_SFLOAT;
				viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				viewCreateInfo.subresourceRange.baseMipLevel = 0;
				viewCreateInfo.subresourceRange.levelCount = 1;
				viewCreateInfo.subresourceRange.baseArrayLayer = 0;
				viewCreateInfo.subresourceRange.layerCount = 1;
			}

			VkImageCreateInfo depthImageInfo{};
			{
				depthImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
				depthImageInfo.imageType = VK_IMAGE_TYPE_2D;
				depthImageInfo.extent.width = _width;
				depthImageInfo.extent.height = _height;
				depthImageInfo.extent.depth = 1;
				depthImageInfo.mipLevels = 1;
				depthImageInfo.arrayLayers = 1;
				depthImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
				depthImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
				depthImageInfo.format = FindDepthFormat();
				depthImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
				depthImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				depthImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
				depthImageInfo.flags = 0;
			}

			VkImageViewCreateInfo depthViewCreateInfo = {};
			{
				depthViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				depthViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
				depthViewCreateInfo.format = FindDepthFormat();
				depthViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY };
				depthViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				depthViewCreateInfo.subresourceRange.baseMipLevel = 0;
				depthViewCreateInfo.subresourceRange.levelCount = 1;
				depthViewCreateInfo.subresourceRange.baseArrayLayer = 0;
				depthViewCreateInfo.subresourceRange.layerCount = 1;
				depthViewCreateInfo.flags = 0;
			}

			for (uint32_t i = 0; i < _numFrames; ++i)
			{
				_gbuffers.push_back({});
				_gbuffers.back().positon.Create(colorImageInfo, viewCreateInfo);
				_gbuffers.back().normal.Create(colorImageInfo, viewCreateInfo);
				_gbuffers.back().albedo.Create(colorImageInfo, viewCreateInfo);
				_gbuffers.back().depth.Create(depthImageInfo, depthViewCreateInfo);
				_framebuffer.Create(_pipeline.get()->_renderPass, { _gbuffers[i].positon.ImageView(), _gbuffers[i].normal.ImageView(), _gbuffers[i].albedo.ImageView(), _gbuffers[i].depth.ImageView() }, { _width, _height });
			}
		}

		void GBufferRenderPass::Destroy()
		{
			for (uint32_t i = 0; i < _gbuffers.size(); ++i)
			{
				_gbuffers[i].positon.Destroy();
				_gbuffers[i].normal.Destroy();
				_gbuffers[i].albedo.Destroy();
				_gbuffers[i].depth.Destroy();
			}
			_gbuffers.clear();

			RenderPass::Destroy();
		}

		GBufferRenderPass::GBuffer* GBufferRenderPass::GetGBuffer(uint32_t index)
		{
			GE_ASSERT(index < _gbuffers.size(), "Invalid texture index");
			return &_gbuffers[index];
		}

		void GBufferRenderPass::Begin(VkCommandBuffer cmdBuffer)
		{
			vkResetCommandBuffer(cmdBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

			VkCommandBufferBeginInfo bufferBeginInfo = {};
			bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

			VkResult result = vkBeginCommandBuffer(cmdBuffer, &bufferBeginInfo);
			GE_ASSERT(result == VK_SUCCESS, "Failed vkBeginCommandBuffer: {}", result);
			GE_UNUSED(result);

			std::array<VkClearValue, 4> clearValues{};
			clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
			clearValues[1].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
			clearValues[2].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
			clearValues[3].depthStencil = { 1.0f, 0 };

			VkRenderPassBeginInfo renderPassBeginInfo = {};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.renderPass = _pipeline->_renderPass;
			renderPassBeginInfo.renderArea.offset = { 0,0 };
			renderPassBeginInfo.renderArea.extent = { _width, _height };
			renderPassBeginInfo.pClearValues = clearValues.data();
			renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
			renderPassBeginInfo.framebuffer = _framebuffer.Get(GetCurrentFrame());

			vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline->Pipeline());

			VkViewport viewport{};
			viewport.x = 0.f;
			viewport.y = static_cast<float>(_height);
			viewport.width = static_cast<float>(_width);
			viewport.height = -static_cast<float>(_height);
			viewport.minDepth = 0.f;
			viewport.maxDepth = 1.f;
			VkRect2D scissor{ {0,0}, {_width, _height} };

			vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
			vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
		}
	}
}