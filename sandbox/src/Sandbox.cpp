#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <entt/entt.hpp>

#include <ge/core/Global.hpp>
#include <ge/core/Common.hpp>
#include <ge/components/Common.hpp>
#include <ge/EntryPoint.hpp>
#include <ge/events/Common.hpp>
#include <ge/gfx/Common.hpp>
#include <ge/gfx/Descriptor.hpp>
#include <ge/gfx/Model.hpp>
#include <ge/gfx/RenderPass.hpp>
#include <ge/gfx/Texture.hpp>
#include <ge/systems/Camera3D.hpp>
#include <ge/systems/InputSystem.hpp>
#include <ge/systems/SkyboxSystem.hpp>
#include <ge/systems/FrustumCullingSystem.hpp>
#include <ge/utils/Types.hpp>
#include <ge/utils/BlobParser.hpp>
#include "ManifestHeader.hpp"

template <typename genType>
GLM_FUNC_QUALIFIER GLM_CONSTEXPR genType pi() { return genType(3.14159265358979323846264338327950288); }

#define SHADOWMAP_RESOLUTION 1024

struct GBufferUniforms
{
	alignas(16) glm::mat4 gWVP;
	alignas(16) glm::mat4 gWorld;
};

struct Light
{
	alignas(16) glm::vec4 Position;
	alignas(16) glm::vec4 Color;

	float Linear;
	float Quadratic;
};

struct Vertex 
{
	glm::vec3 pos;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}
	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions()
	{
		return {
			{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)},
			{1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, texCoord)}
		};
	}
};

struct ShadowMapUniforms
{
	alignas(16)  glm::mat4 projection;
	alignas(16)  glm::mat4 model;
	alignas(16)  glm::vec4 lightPos;
};

class Mesh : public Drawable
{
public:
	Mesh() {}
	Mesh(GE::Gfx::ModelObject* object) : object(object) 
	{
		auto& registry = GE::GlobalRegistry();
		entity = registry.create();
		registry.emplace<Visibility>(entity, false);
		registry.emplace<Object>(entity, this);
		registry.emplace<CenterOfMass>(entity, object->centerOfMass);
	}
	virtual ~Mesh()
	{
		if (loaded) {
			verticesBuffer.Destroy();
			texCoordsBuffer.Destroy();
			normalsBuffer.Destroy();
			texIdsBuffer.Destroy();
		}
	}

	virtual bool Buffer(VkCommandBuffer cmdBuffer, VkDescriptorSet* descriptor, uint32_t frameIndex) override
	{
		if (loaded)
			return false;

		numVerts = static_cast<uint32_t>(object->vertices.size());

		verticesBuffer.Create(object->vertices.size() * sizeof(glm::vec3));
		verticesBuffer.Buffer(cmdBuffer, object->vertices.data(), object->vertices.size() * sizeof(glm::vec3));

		glm::vec3 min = object->vertices.front();
		glm::vec3 max = object->vertices.front();
		for (auto& v : object->vertices) {
			min = glm::vec3(std::min(min.x, v.x), std::min(min.y, v.y), std::min(min.z, v.z));
			max = glm::vec3(std::max(max.x, v.x), std::max(max.y, v.y), std::max(max.z, v.z));
		}
		GE::GlobalRegistry().emplace<AABB>(entity, min, max);

		texCoordsBuffer.Create(object->texCoords.size() * sizeof(glm::vec2));
		texCoordsBuffer.Buffer(cmdBuffer, object->texCoords.data(), object->texCoords.size() * sizeof(glm::vec2));

		normalsBuffer.Create(object->normals.size() * sizeof(glm::vec3));
		normalsBuffer.Buffer(cmdBuffer, object->normals.data(), object->normals.size() * sizeof(glm::vec3));

		texIdsBuffer.Create(object->texture_id.size() * sizeof(float));
		texIdsBuffer.Buffer(cmdBuffer, object->texture_id.data(), object->texture_id.size() * sizeof(float));

		loaded = true;
		return true;
	}

	virtual void Draw(VkCommandBuffer cmdBuffer, VkDescriptorSet* descriptor, uint32_t frameIndex) override
	{
		if (!loaded)
			return;

		VkDeviceSize offsets[] = { 0, 0, 0, 0 };
		VkBuffer buffers[] = { verticesBuffer, texCoordsBuffer, normalsBuffer, texIdsBuffer };
		vkCmdBindVertexBuffers(cmdBuffer, 0, 4, buffers, offsets);

		vkCmdDraw(cmdBuffer, numVerts, 1, 0, 0);
	}

public:
	GE::Gfx::ModelObject* object{ nullptr };
	entt::entity entity;
};

class TestLayer : virtual public GE::Sys::System, public GE::Camera3DSink, public GE::ResourceSink
{
public:
	TestLayer()
		: GE::Sys::System("TestLayer")
	{
		REGISTER_SYSTEM();
		GE::Utils::EngineResourceParser::Get();
	}

	void Attach()
	{
		GE::Gfx::NullTexture::Get();
		CreateUserData();
		GE::GlobalDispatcher().sink<GE::Gfx::ResizeEvent>().connect<&TestLayer::Resize>(this);
	}

	void Detach()
	{
		GE::Gfx::NullTexture::Get()->Destroy();
		GE::GlobalDispatcher().sink<GE::Gfx::ResizeEvent>().disconnect<&TestLayer::Resize>(this);
		DestroyUserData();
	}

	void CreateUserData()
	{
		commandBuffers.Create(64);
		descriptorPool.Create(
			{
				{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
				{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000}
			},
			VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
			10
		);

		descriptorPool.CreateNewDescriptorSet({
			{{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT , nullptr }, 0},
			{{ 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 256, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr }, VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT},
		});

		descriptorPool.CreateNewDescriptorSet({
			{{ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT , nullptr }, 0},
			{{ 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT , nullptr }, 0},
			{{ 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT , nullptr }, 0},
			{{ 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT , nullptr }, 0},
			{{ 4, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT , nullptr }, 0},
			{{ 5, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT , nullptr }, 0}
		});

		descriptorPool.CreateNewDescriptorSet({
			{{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT , nullptr }, 0},
		});

		descriptorPool.CreateNewDescriptorSet({
			{{ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr }, 0},
		});

		{
			_gbufferPass.AddShader("shaders/gbuffer.vert.spv");
			_gbufferPass.AddShader("shaders/gbuffer.frag.spv");
			_gbufferPass.SetClearFlag(true);

			_gbufferPass.AddVertexBufferDescs({
				{ 0, 3_FLOAT, VK_VERTEX_INPUT_RATE_VERTEX },
				{ 1, 2_FLOAT, VK_VERTEX_INPUT_RATE_VERTEX },
				{ 2, 3_FLOAT, VK_VERTEX_INPUT_RATE_VERTEX },
				{ 3, 1_FLOAT, VK_VERTEX_INPUT_RATE_VERTEX },
				});

			_gbufferPass.AddVertexAttribDescs({
				{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
				{ 1, 1, VK_FORMAT_R32G32_SFLOAT, 0 },
				{ 2, 2, VK_FORMAT_R32G32B32_SFLOAT, 0 },
				{ 3, 3, VK_FORMAT_R32_SFLOAT, 0 },
				});

			_gbufferPass.AddDescriptorsSetLayouts({ descriptorPool.GetDescriptorSetLayout(0) });
			_gbufferPass.Configure(GE::Gfx::GetNumFrames(), 1280, 720);
			_gbufferPass.Create();
		}
		{
			_shadowPass.SetClearFlag(true);
			_shadowPass.AddShader("shaders/shadowcube.vert.spv");
			_shadowPass.AddShader("shaders/shadowcube.frag.spv");

			_shadowPass.AddVertexBufferDescs({
				{ 0, 3_FLOAT, VK_VERTEX_INPUT_RATE_VERTEX },
				});

			_shadowPass.AddVertexAttribDescs({
				{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
				});

			_shadowPass.AddPushConstants({ {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4)} });

			_shadowPass.AddDescriptorsSetLayouts({ descriptorPool.GetDescriptorSetLayout(2) });
			_shadowPass.Configure(1, SHADOWMAP_RESOLUTION, SHADOWMAP_RESOLUTION);
			_shadowPass.Create();
		}
		{
			_compositionPass.AddShader("shaders/composition.vert.spv");
			_compositionPass.AddShader("shaders/composition.frag.spv");
			_compositionPass.AddVertexBufferDescs({ Vertex::getBindingDescription() });
			_compositionPass.AddVertexAttribDescs(Vertex::getAttributeDescriptions());

			_compositionPass.AddDescriptorsSetLayouts({ descriptorPool.GetDescriptorSetLayout(1) });
			_compositionPass.Configure(GE::Gfx::GetNumFrames(), 1280, 720);
			_compositionPass.Create();
		}
		{
			_finalPass.AddShader("shaders/screen.vert.spv");
			_finalPass.AddShader("shaders/screen.frag.spv");
			_finalPass.AddVertexBufferDescs({ Vertex::getBindingDescription() });
			_finalPass.AddVertexAttribDescs(Vertex::getAttributeDescriptions());
			_finalPass.AddDescriptorsSetLayouts({ descriptorPool.GetDescriptorSetLayout(3) });
			_finalPass.Configure();
			_finalPass.Create();
		}

		_sun = Light {
			{500, 1500, 0, 0 },
			{1,1,1,0},
			.0000007f,
			.00000018f
			};

		_compositionPassVertexBuffer.Create(vertices.size() * sizeof(Vertex));
		_sunDataBuffer.Create(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(Light));
		_shadowPassUniformBuffer.Create(sizeof(ShadowMapUniforms));
		_finalPassVertices.Create(sizeof(Vertex) * vertices.size());

		_compositionPassVertexBuffer.Buffer(commandBuffers.GetBuffer(), (void*)vertices.data(), vertices.size() * sizeof(Vertex));
		_finalPassVertices.Buffer(commandBuffers.GetBuffer(), (void*)vertices.data(), sizeof(Vertex) * vertices.size());

		for (uint32_t i = 0; i < GE::Gfx::GetNumFrames(); i++)
		{
			_gbufferUniformBuffers[i].Create(sizeof(GBufferUniforms));

			//Set Shadow Images to correct layout
			{
				auto currentBuffer = commandBuffers.GetBuffer();
				vkResetCommandBuffer(currentBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
				VkCommandBufferBeginInfo bufferBeginInfo = {};
				bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				VkResult result = vkBeginCommandBuffer(currentBuffer, &bufferBeginInfo);
				GE_UNUSED(result);

				// Image barrier for optimal image (target)
				VkImageSubresourceRange subresourceRange = {};
				subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				subresourceRange.baseMipLevel = 0;
				subresourceRange.levelCount = 1;
				subresourceRange.layerCount = 6;

				GE::Gfx::SetImageLayout(currentBuffer, _shadowPass.GetShadowTexture(0).Image(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange);
				GE::Gfx::SetImageLayout(currentBuffer, _shadowPass._colorTextures[0].Image(), VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
				GE::Gfx::SetImageLayout(currentBuffer, _shadowPass._depthTextures[0].Image(), VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

				vkEndCommandBuffer(currentBuffer);
				commandBuffers.SubmitBuffer();
			}
			// Shadow/GBuffer Composition Render Pass
			{
				VkDescriptorImageInfo gPosition{ _gbufferPass.GetGBuffer(i)->positon.Sampler(), _gbufferPass.GetGBuffer(i)->positon.ImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
				descriptorPool.UpdateDescriptorImage(1, i, 0u, &gPosition, 1);

				VkDescriptorImageInfo gColor{ _gbufferPass.GetGBuffer(i)->normal.Sampler(), _gbufferPass.GetGBuffer(i)->normal.ImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
				descriptorPool.UpdateDescriptorImage(1, i, 1u, &gColor, 1);

				VkDescriptorImageInfo gNormal{ _gbufferPass.GetGBuffer(i)->albedo.Sampler(), _gbufferPass.GetGBuffer(i)->albedo.ImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
				descriptorPool.UpdateDescriptorImage(1, i, 2u, &gNormal, 1);

				VkDescriptorImageInfo gShadowMap{ _shadowPass.GetShadowTexture(0).Sampler(), _shadowPass.GetShadowTexture(0).ImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
				descriptorPool.UpdateDescriptorImage(1, i, 3u, &gShadowMap, 1);

				_compositionPassCameraDataBuffers[i].Create(sizeof(glm::vec3));
				VkDescriptorBufferInfo posBufferInfo{ _compositionPassCameraDataBuffers[i], 0, sizeof(glm::vec3) };
				descriptorPool.UpdateDescriptorBuffer(1, i, 4u, &posBufferInfo);

				_sunDataBuffer.Buffer(commandBuffers.GetBuffer(), &_sun, sizeof(Light));
				VkDescriptorBufferInfo lightBufferInfo{ _sunDataBuffer, 0, sizeof(Light) };
				descriptorPool.UpdateDescriptorBuffer(1, i, 5u, &lightBufferInfo);

				descriptorPool.WriteDescriptorSet(1, i);
			}
			// Shadow Map Render Pass Descriptors
			{
				VkDescriptorBufferInfo bufferInfo{ _shadowPassUniformBuffer, 0, sizeof(ShadowMapUniforms) };
				descriptorPool.UpdateDescriptorBuffer(2, i, 0, &bufferInfo);
				descriptorPool.WriteDescriptorSet(2, i);
			}
			// Final Render Pass Descriptors
			{
				VkDescriptorImageInfo gColor{ _compositionPass.GetColorTexture(i).Sampler(), _compositionPass.GetColorTexture(i).ImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
				descriptorPool.UpdateDescriptorImage(3, i, 0u, &gColor, 1);
				descriptorPool.WriteDescriptorSet(3, i);
			}
		}
	}

	void DestroyUserData()
	{
		_modelObjects.clear();
		_finalPassVertices.Destroy();
		_finalPass.Destroy();
		for (uint32_t i = 0; i < GE::Gfx::GetNumFrames(); i++) {
			_gbufferUniformBuffers[i].Destroy();
			_compositionPassCameraDataBuffers[i].Destroy();
		}
		_shadowPassUniformBuffer.Destroy();
		_sunDataBuffer.Destroy();
		_compositionPassVertexBuffer.Destroy();
		_compositionPass.Destroy();
		_shadowPass.Destroy();
		_gbufferPass.Destroy();
		descriptorPool.Destroy();
		commandBuffers.Destroy();
	}

	virtual void RenderScene() override
	{
		if (!_loadedResources)
			return;

		auto currentFrame = GE::Gfx::GetCurrentFrame();
		auto view = GE::GlobalRegistry().view<const Object, const Visibility, const CenterOfMass>();

		// Generate G-Buffer Textures
		{
			VkCommandBuffer& currentBuffer = commandBuffers.GetBuffer();

			int64_t numBuffered = 0;
			view.each([&](const Object& obj, const Visibility visibility, const CenterOfMass) {
				if (numBuffered > 0)
					return;

				if (obj.drawable->Buffer(currentBuffer, &descriptorPool.GetDescriptorSet(0, currentFrame), currentFrame))
				{
					numBuffered++;
					_renderShadowCube = true;
				}
			});

			GBufferUniforms gbufferUniform;
			gbufferUniform.gWVP = _cameraData.projection * _cameraData.view;
			gbufferUniform.gWorld = glm::mat4(1.f);
			_gbufferUniformBuffers[currentFrame].Buffer(currentBuffer, &gbufferUniform, sizeof(gbufferUniform));

			_gbufferPass.Begin(currentBuffer);

			vkCmdBindDescriptorSets(currentBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _gbufferPass.Layout(), 0, 1, &descriptorPool.GetDescriptorSet(0, currentFrame), 0, nullptr);
			
			std::vector<std::pair<Drawable*, float>> objects;
			view.each([&](const Object obj, const Visibility visibility, const CenterOfMass centerOfMass) {
				if (visibility) {
					objects.push_back({ obj.drawable, glm::length(_cameraData.position - centerOfMass)});
				}
			});

			// Sort objects by distance (front to back)
			std::sort(objects.begin(), objects.end(), [](const std::pair<Drawable*, float>& a, const std::pair<Drawable*, float>& b) -> bool { return a.second < b.second; });
			for (auto& obj : objects) {
				obj.first->Draw(currentBuffer, &descriptorPool.GetDescriptorSet(0, currentFrame), currentFrame);
			}

			_gbufferPass.End(currentBuffer);
			commandBuffers.SubmitBuffer();
		}

		// Generate Shadow Map
		if(_renderShadowCube)
		{
			float zNear = 0.1f;
			float zFar = 2048.0f;

			ShadowMapUniforms shadowMapUniforms;
			shadowMapUniforms.projection = glm::perspective((float)(pi<float>() / 2.0), 1.0f, zNear, zFar) * glm::scale(glm::mat4(1.f), { 1,-1,1 });
			shadowMapUniforms.lightPos = _sun.Position;
			shadowMapUniforms.model = glm::translate(glm::mat4(1.0f), glm::vec3(-_sun.Position.x, -_sun.Position.y, -_sun.Position.z));

			_shadowPassUniformBuffer.Buffer(commandBuffers.GetBuffer(), &shadowMapUniforms, sizeof(shadowMapUniforms));

			for (uint32_t faceIndex = 0; faceIndex < 6; faceIndex++)
			{
				VkCommandBuffer& currentBuffer = commandBuffers.GetBuffer();
				_shadowPass.Begin(currentBuffer);

				glm::mat4 viewMatrix = glm::mat4(1.0f);
				switch (faceIndex)
				{
				case 0: // POSITIVE_X
					viewMatrix = glm::rotate(viewMatrix, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
					viewMatrix = glm::rotate(viewMatrix, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
					break;
				case 1:	// NEGATIVE_X
					viewMatrix = glm::rotate(viewMatrix, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
					viewMatrix = glm::rotate(viewMatrix, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
					break;
				case 2:	// POSITIVE_Y
					viewMatrix = glm::rotate(viewMatrix, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
					break;
				case 3:	// NEGATIVE_Y
					viewMatrix = glm::rotate(viewMatrix, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
					break;
				case 4:	// POSITIVE_Z
					viewMatrix = glm::rotate(viewMatrix, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
					break;
				case 5:	// NEGATIVE_Z
					viewMatrix = glm::rotate(viewMatrix, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
					break;
				}
				vkCmdPushConstants(currentBuffer, _shadowPass._pipeline->PipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &viewMatrix);

				vkCmdBindDescriptorSets(currentBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _shadowPass.Layout(), 0, 1, &descriptorPool.GetDescriptorSet(2, currentFrame), 0, nullptr);

				view.each([&](const Object obj, const Visibility visibility, const CenterOfMass) {
					if (obj.drawable->loaded) {
						VkDeviceSize offsets[] = { 0 };
						VkBuffer buffers[] = { obj.drawable->verticesBuffer };
						vkCmdBindVertexBuffers(currentBuffer, 0, 1, buffers, offsets);

						vkCmdDraw(currentBuffer, obj.drawable->numVerts, 1, 0, 0);
					}
					});

				vkCmdEndRenderPass(currentBuffer);

				GE::Gfx::SetImageLayout(currentBuffer, _shadowPass._colorTextures[0].Image(), VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

				VkImageSubresourceRange cubeFaceSubresourceRange = {};
				cubeFaceSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				cubeFaceSubresourceRange.baseMipLevel = 0;
				cubeFaceSubresourceRange.levelCount = 1;
				cubeFaceSubresourceRange.baseArrayLayer = faceIndex;
				cubeFaceSubresourceRange.layerCount = 1;

				// Change image layout of one cubemap face to transfer destination
				GE::Gfx::SetImageLayout(currentBuffer, _shadowPass.GetShadowTexture(0).Image(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, cubeFaceSubresourceRange);

				// Copy region for transfer from framebuffer to cube face
				VkImageCopy copyRegion = {};
				copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				copyRegion.srcSubresource.baseArrayLayer = 0;
				copyRegion.srcSubresource.mipLevel = 0;
				copyRegion.srcSubresource.layerCount = 1;
				copyRegion.srcOffset = { 0, 0, 0 };
				copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				copyRegion.dstSubresource.baseArrayLayer = faceIndex;
				copyRegion.dstSubresource.mipLevel = 0;
				copyRegion.dstSubresource.layerCount = 1;
				copyRegion.dstOffset = { 0, 0, 0 };
				copyRegion.extent = { SHADOWMAP_RESOLUTION, SHADOWMAP_RESOLUTION, 1 };

				// Put image copy into command buffer
				vkCmdCopyImage(currentBuffer, _shadowPass._colorTextures[0].Image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, _shadowPass.GetShadowTexture(0).Image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

				// Transform framebuffer color attachment back
				GE::Gfx::SetImageLayout(currentBuffer, _shadowPass._colorTextures[0].Image(), VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

				// Change image layout of copied face to shader read
				GE::Gfx::SetImageLayout(currentBuffer, _shadowPass.GetShadowTexture(0).Image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, cubeFaceSubresourceRange);

				vkEndCommandBuffer(currentBuffer);
				commandBuffers.SubmitBuffer();
			}

			_renderShadowCube = false;
		}

		// Render Shadow Map and G-Buffer resulting Image
		{
			VkCommandBuffer& currentBuffer = commandBuffers.GetBuffer();

			auto position = _cameraData.position;
			position.x *= -1;
			position.y *= -1;
			_compositionPassCameraDataBuffers[currentFrame].Buffer(currentBuffer, &position, sizeof(glm::vec3));

			_compositionPass.Begin(currentBuffer);
			_compositionPassVertexBuffer.Bind(currentBuffer);
			vkCmdBindDescriptorSets(currentBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _compositionPass.Layout(), 0, 1, &descriptorPool.GetDescriptorSet(1, currentFrame), 0, nullptr);
			vkCmdDraw(currentBuffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);

			_compositionPass.End(currentBuffer);
			commandBuffers.SubmitBuffer();
		}

		//Render final image to screen
		{
			VkCommandBuffer& currentBuffer = commandBuffers.GetBuffer();
			_finalPass.Begin(currentBuffer);

			_finalPassVertices.Bind(currentBuffer);
			vkCmdBindDescriptorSets(currentBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _finalPass.Layout(), 0, 1, &descriptorPool.GetDescriptorSet(3, currentFrame), 0, nullptr);
			vkCmdDraw(currentBuffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);
			_finalPass.End(currentBuffer);
			commandBuffers.SubmitBuffer();
		}
	}

	void Resize(GE::Gfx::ResizeEvent evt)
	{
		vkDeviceWaitIdle(*GE::Gfx::VulkanCore::Get().device);
		if (evt.recreate)
		{
			_finalPass.Resize(GE::Gfx::VulkanCore::Get().swapchain->GetExtent());

			for (uint32_t i = 0; i < GE::Gfx::GetNumFrames(); i++)
			{
				VkDescriptorImageInfo gColor{ _compositionPass.GetColorTexture(i).Sampler(), _compositionPass.GetColorTexture(i).ImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
				descriptorPool.UpdateDescriptorImage(3, i, 0u, &gColor, 1);
				descriptorPool.WriteDescriptorSet(3, i);
			}
		}
	}

	std::vector<GE::Utils::UUID> waitingToLoad;
	void LoadTexturesForModel()
	{
		bool firstPass = sceneTextures.empty();
		std::vector<GE::Gfx::VulkanTexture> modelTextures;

		std::string path = "";
		for (auto& material : _modelHandle->materials)
		{
			if (!material.ambient_texname.empty())
				path = material.ambient_texname;

			GE::Utils::UUID uuid = GE::Sys::ResourceSystem::Get().LookupResource(path);
			GE::Sys::ResourceHandle<GE::Gfx::Texture> texture(uuid);

			if (firstPass) {
				sceneTextures.push_back(texture);
				RegisterUUID(uuid);
			}

			if (!texture->_isLoaded)
			{
				modelTextures.push_back(*GE::Gfx::NullTexture::Get());
				waitingToLoad.push_back(uuid);
			}
			else
				modelTextures.push_back(texture->_texture);
		}

		uint32_t i = 0;
		uint32_t ii = GE::Gfx::GetNumFrames();
		for (; i < ii; i++)
		{
			VkDescriptorBufferInfo bufferInfo{ _gbufferUniformBuffers[i], 0, sizeof(GBufferUniforms) };
			descriptorPool.UpdateDescriptorBuffer(0, i, 0, &bufferInfo);

			std::vector<VkDescriptorImageInfo> textureDescriptors(modelTextures.size());
			for (uint32_t j = 0; j < static_cast<uint32_t>(modelTextures.size()); j++) {
				textureDescriptors[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				textureDescriptors[j].sampler = modelTextures[j].Sampler();
				textureDescriptors[j].imageView = modelTextures[j].ImageView();
			}
			descriptorPool.UpdateDescriptorImage(0, i, 1U, textureDescriptors.data(), static_cast<uint32_t>(textureDescriptors.size()));
			descriptorPool.WriteDescriptorSet(0, i);
		}

		ResourcesUpdated = false;
	}

	virtual void Update(int64_t tsMicroseconds) override
	{
		if (!_loadedResources && _modelHandle->_isLoaded)
		{
			vkDeviceWaitIdle(*GE::Gfx::VulkanCore::Get().device);

			_modelObjects.reserve(_modelHandle->objects.size());
			for (auto& object : _modelHandle->objects) {
				_modelObjects.emplace_back(&object);
			}

			_loadedResources = true;
			ResourcesUpdated = true;
		}

		if(ResourcesUpdated)
			LoadTexturesForModel();
	}

private:
	GE::Gfx::VulkanDescriptorsPool descriptorPool;
	GE::Gfx::VulkanCommandBuffers commandBuffers;

	// Resources
	bool _loadedResources = false;
	GE::Sys::ResourceHandle<GE::Gfx::Model> _modelHandle{ MODEL_SPONZA_OBJ };
	std::vector<GE::Sys::ResourceHandle<GE::Gfx::Texture>> sceneTextures;
	std::vector<Mesh> _modelObjects;

	// GBuffer Pass
	GE::Gfx::GBufferRenderPass _gbufferPass;
	GE::Gfx::VulkanUniformBuffer _gbufferUniformBuffers[3];

	// Shadow Pass 
	bool _renderShadowCube = true;
	GE::Gfx::OmniDirectionShadowRenderPass _shadowPass;
	GE::Gfx::VulkanUniformBuffer _shadowPassUniformBuffer;
	GE::Gfx::VulkanBuffer _sunDataBuffer;
	Light _sun;

	// GBuffer/Shadow Composition Pass
	GE::Gfx::MSAAFrameBufferRenderPass _compositionPass;
	GE::Gfx::VulkanVertexBuffer _compositionPassVertexBuffer;
	GE::Gfx::VulkanUniformBuffer _compositionPassCameraDataBuffers[3];

	//Final Pass
	GE::Gfx::ScreenSpaceRenderPass _finalPass;
	GE::Gfx::VulkanVertexBuffer _finalPassVertices;

	const std::vector<Vertex> vertices = {
		{{-1.f, -1.f, 0.f}, {0.0f, 1.0f}},
		{{1.f, -1.f, 0.f}, {1.0f, 1.0f}},
		{{1.f, 1.f, 0.f}, {1.0f, 0.0f}},
		{{1.f, 1.f, 0.f}, {1.0f, 0.0f}},
		{{-1.f, 1.f, 0.f}, {0.0f, 0.0f}},
		{{-1.f, -1.f, 0.f}, {0.0f, 1.0f}},
	};
};

class TestGuiLayer : public GE::Sys::System
{
public:
	TestGuiLayer()
		: GE::Sys::System("TestGuiLayer")
	{
	}

	virtual void RenderGui() override
	{
		if (ImGui::Begin("Framerate", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize))
		{
			ImGui::SetWindowSize("Framerate", { 200, 25 });
			ImGui::SetWindowPos("Framerate", { 0, 0 });
			ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		}
		ImGui::End();
	}
};

class Sandbox : public GE::GfxApplication
{
public:
	Sandbox() 
		: GE::GfxApplication("Sandbox"),
		inputSys(this)
	{
#ifdef _DEBUG
		GE_SET_LOG_LEVEL(GE_LOG_LEVEL_TRACE);
#else 
		GE_SET_LOG_LEVEL(GE_LOG_LEVEL_INFO);
#endif
	}

	virtual void Create() override
	{
		TIMED_TRACE();

		inputSys.UpdateKeyBinding(GE::Keyboard::W, GE::Controller::Up);
		inputSys.UpdateKeyBinding(GE::Keyboard::S, GE::Controller::Down);
		inputSys.UpdateKeyBinding(GE::Keyboard::A, GE::Controller::Left);
		inputSys.UpdateKeyBinding(GE::Keyboard::D, GE::Controller::Right);
		inputSys.UpdateKeyBinding(GE::Keyboard::E, GE::Controller::A);
		inputSys.UpdateKeyBinding(GE::Keyboard::Q, GE::Controller::B);
		inputSys.UpdateKeyBinding(GE::Keyboard::Minus, GE::Controller::Minus);
		inputSys.UpdateKeyBinding(GE::Keyboard::Plus, GE::Controller::Plus);
		inputSys.UpdateKeyBinding(GE::Keyboard::LeftArrow, GE::Controller::ZL);
		inputSys.UpdateKeyBinding(GE::Keyboard::RightArrow, GE::Controller::ZR);

		skyboxLayer.SetImage("textures/skybox.png");

		PushSystem(&cullingSys);
		PushSystem(&skyboxLayer);
		PushSystem(&testLayer);
		PushSystem(&testGuiLayer);
		PushSystem(&inputSys);
		PushSystem(&cameraSys);
	}

	virtual void Destroy() override
	{
		TIMED_TRACE();
		WaitForWindowIdle();

		PopSystem(&cullingSys);
		PopSystem(&testLayer);
		PopSystem(&skyboxLayer);
		PopSystem(&testGuiLayer);
		PopSystem(&inputSys);
		PopSystem(&cameraSys);
	}

	virtual bool Update(int64_t tsMicroseconds) override
	{	
		return !ShouldClose();
	}

private:
	TestLayer testLayer;
	TestGuiLayer testGuiLayer;
	GE::Sys::SkyboxSystem skyboxLayer;
	GE::Sys::Camera3D cameraSys;
	GE::Sys::InputSystem inputSys;
	GE::Sys::FrustumCullingSystem cullingSys;
};

std::unique_ptr<GE::Application> GE::CreateApplication()
{
	return std::make_unique<Sandbox>();
}