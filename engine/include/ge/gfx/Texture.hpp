#pragma once

#include <string>

#include <ge/core/Global.hpp>
#include <ge/gfx/Buffer.hpp>
#include <ge/gfx/Common.hpp>
#include <ge/gfx/CommandBuffers.hpp>
#include <ge/gfx/Device.hpp>
#include <ge/systems/ResourceSystem.hpp>

namespace GE
{
	namespace Gfx
	{
		void SetImageLayout(VkCommandBuffer cmdbuffer, VkImage image, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkImageSubresourceRange subresourceRange, VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
		void SetImageLayout(VkCommandBuffer cmdbuffer, VkImage image, VkImageAspectFlags aspectMask, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

		class VulkanTexture : public VulkanGfxElement
		{
		public:
			virtual void Create(VkImageCreateInfo imageCreateInfo, VkImageViewCreateInfo imageViewCreateInfo);
			virtual void Create(VkCommandBuffer& cmdBuffer);
			virtual void Destroy();

			void LoadFromStorage(const std::string& path);
			void LoadFromEngineResources(const std::string& path);
			bool IsLoaded() { return _loaded.Get(); }

			VkImage Image() { return _image; }
			VkImageView ImageView() { return _imageView; }
			VkSampler Sampler() { return _sampler; }

			//operator VkImage() { return _image; }
			//operator VkImageView() { return _imageView; }
			//operator VkSampler() { return _sampler; }

		public:
			void CopyBufferToImage(VkCommandBuffer& cmdBuffer);
			void TransitionLayout(VkCommandBuffer& cmdBuffer, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

			void BeginVkCmd(VkCommandBuffer& cmdBuffer);
			void EndVkCmd(VkCommandBuffer& cmdBuffer);

			Utils::Guarded<bool> _loaded{ false };

			int _width{ 0 };
			int _height{ 0 };
			int _channels{ 0 };

			VulkanBuffer _buffer;
			VkImage _image;
			VkImageView _imageView;
			VkSampler _sampler;
			VkDeviceMemory _memory;

			unsigned char* _pixels{ nullptr };
		};

		class NullTexture : public VulkanTexture
		{
		public:
			static NullTexture* Get() {
				static NullTexture texture;

				if (!texture.IsLoaded())
				{
					texture.LoadFromEngineResources("TEXTURE_NULL_PNG");

					VulkanCommandBuffers cmdBuffer;
					cmdBuffer.Create(1);
					texture.VulkanTexture::Create(cmdBuffer.GetBuffer());
					cmdBuffer.Destroy();
				}

				return &texture;
			}
		};

		class Texture : public Sys::Resource
		{
		public:
			Texture()
				: Resource({})
			{}

			Texture(Sys::ResourceData* data)
				: Resource(data)
				, _path(data->path)
			{
			}

			virtual void Load() override {
				if (!_isLoaded)
				{
					_cmdBuffer.Create(1);
					auto buffer = _cmdBuffer.GetBuffer();
					_texture.Create(buffer);
					_cmdBuffer.Destroy();
					_isLoaded = true;
				}
			}

			virtual bool LimitToMainThread() override { return true; }

			virtual void LoadFromStorage() override {
				if (!_isLoaded)
				{
					_texture.LoadFromStorage(_path);
				}
			}

			virtual void Unload() override {
				if (_isLoaded)
				{
					_texture.Destroy();
					_isLoaded = false;
				}
			}

			operator VulkanTexture& () {
				return _texture;
			}

			std::string _path;
			VulkanTexture _texture;
			VulkanCommandBuffers _cmdBuffer;
		};
	}
}