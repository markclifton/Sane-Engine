#include <ge/systems/SkyboxSystem.hpp>

#include <glm/gtx/transform.hpp>

#include <ge/core/Common.hpp>
#include <ge/events/CameraEvents.hpp>
#include <ge/utils/Types.hpp>
#include <resources/EngineShaders.hpp>

namespace GE
{
	namespace Sys
	{
		SkyboxSystem::SkyboxSystem()
			: System("SkyboxSystem")
		{
			REGISTER_SYSTEM();
		}

		void SkyboxSystem::SetImage(const std::string& texturePath)
		{
			_texturePath = texturePath;

			if (_active)
			{
				skyboxTexture.Destroy();
				skyboxTexture.LoadFromEngineResources("TEXTURE_SKYBOX_PNG");

				skyboxTexture.Create(commandBuffers.GetBuffer());

				VkDescriptorImageInfo imageInfo{};
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfo.imageView = skyboxTexture.ImageView();
				imageInfo.sampler = skyboxTexture.Sampler();

				descriptorPool.UpdateDescriptorImage(0, 0, 0U, &imageInfo, 1);
				descriptorPool.WriteDescriptorSet(0, 0);
			}
		}

		void SkyboxSystem::Attach()
		{
			GE_ASSERT(!_texturePath.empty(), "No skybox texture set!");
			
			TIMED_TRACE();

			commandBuffers.Create(3U);

			defaultRenderPass.SetClearFlag(true);
			defaultRenderPass.AddShaderBinaryData(std::string(SHADER_VERT_SKYBOX), VK_SHADER_STAGE_VERTEX_BIT);
			defaultRenderPass.AddShaderBinaryData(std::string(SHADER_FRAG_SKYBOX), VK_SHADER_STAGE_FRAGMENT_BIT);

			defaultRenderPass.AddVertexBufferDescs({
				{ 0, 3_FLOAT, VK_VERTEX_INPUT_RATE_VERTEX },
				{ 1, 2_FLOAT, VK_VERTEX_INPUT_RATE_VERTEX },
			});

			defaultRenderPass.AddVertexAttribDescs({
				{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
				{ 1, 1, VK_FORMAT_R32G32_SFLOAT, 0 },
			});

			defaultRenderPass.AddPushConstants({ 
				{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4)}
			});

			descriptorPool.Create({ {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1} }, 0);
			descriptorPool.CreateNewDescriptorSet({
				{{ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr }, 0}
			});

			defaultRenderPass.AddDescriptorsSetLayouts({ descriptorPool.GetDescriptorSetLayout(0) });
			defaultRenderPass.Configure(); 
			defaultRenderPass.Create();

			skyboxTexture.LoadFromStorage(_texturePath);
			skyboxTexture.Create(commandBuffers.GetBuffer());

			char* dataBuffer = (char*)SKYBOX_CUBE;
			std::vector<char> data(dataBuffer, dataBuffer + strlen(dataBuffer));

			_skyboxModel.Load(data);
			_skyboxObject = _skyboxModel.objects.front();

			buffer.Create(_skyboxObject.vertices.size() * sizeof(glm::vec3));
			buffer.Buffer(commandBuffers.GetBuffer(), _skyboxObject.vertices.data(), _skyboxObject.vertices.size() * sizeof(glm::vec3));

			buffer2.Create(_skyboxObject.texCoords.size() * sizeof(glm::vec2));
			buffer2.Buffer(commandBuffers.GetBuffer(), _skyboxObject.texCoords.data(), _skyboxObject.texCoords.size() * sizeof(glm::vec2));

			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = skyboxTexture.ImageView();
			imageInfo.sampler = skyboxTexture.Sampler();

			descriptorPool.UpdateDescriptorImage(0, 0, 0U, &imageInfo, 1);
			descriptorPool.WriteDescriptorSet(0, 0);

			GlobalDispatcher().sink<Gfx::ResizeEvent>().connect<&SkyboxSystem::Resize>(this);
			GlobalDispatcher().sink<Camera3DData>().connect<&SkyboxSystem::UpdateCamera>(this);

			_active = true;
		}

		void SkyboxSystem::Detach()
		{
			_active = false;

			commandBuffers.Destroy();
			descriptorPool.Destroy();
			skyboxTexture.Destroy();

			buffer.Destroy();
			buffer2.Destroy();

			defaultRenderPass.Destroy();

			GlobalDispatcher().sink<Gfx::ResizeEvent>().disconnect<&SkyboxSystem::Resize>(this);
			GlobalDispatcher().sink<Camera3DData>().disconnect<&SkyboxSystem::UpdateCamera>(this);
		}

		void SkyboxSystem::RenderScene()
		{
			auto currentBuffer = commandBuffers.GetBuffer();
			defaultRenderPass.Begin(currentBuffer);

			VkDeviceSize offsets[] = { 0, 0 };
			VkBuffer buffers[] = { buffer, buffer2 };
			vkCmdBindVertexBuffers(currentBuffer, 0, 2, buffers, offsets);

			glm::mat4 projection = _cameraData.projection * _cameraData.view;
			projection[3] = { 0,0,0,1 };

			glm::mat4 push = projection * glm::scale(glm::mat4(1.f), { 100, 100, 100 }) 
				* glm::rotate(glm::mat4(1.f), (float)(totalTimeAlive / 4000000.), glm::vec3(0,1,0))
				* glm::translate(glm::mat4(1.f), { -.5f, -.5f, -.5f });
			vkCmdPushConstants(currentBuffer, defaultRenderPass._pipeline->_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &push);

			vkCmdBindDescriptorSets(currentBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, defaultRenderPass.Layout(), 0, 1, &descriptorPool.GetDescriptorSet(0, 0), 0, nullptr);
			vkCmdDraw(currentBuffer, static_cast<uint32_t>(_skyboxObject.vertices.size()), 1, 0, 0);

			defaultRenderPass.End(currentBuffer);
			commandBuffers.SubmitBuffer();
		}

		void SkyboxSystem::Resize(GE::Gfx::ResizeEvent evt)
		{
			vkDeviceWaitIdle(*GE::Gfx::VulkanCore::Get().device);
			if (evt.recreate) {
				defaultRenderPass.Resize(GE::Gfx::VulkanCore::Get().swapchain->GetExtent());
			}
		}

		void SkyboxSystem::UpdateCamera(const Camera3DData& cameraData)
		{
			_cameraData = cameraData;
		}
	}
}