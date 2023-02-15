#pragma once

#include <glm/glm.hpp>

#include <ge/gfx/Common.hpp>
#include <ge/gfx/CommandBuffers.hpp>
#include <ge/gfx/Descriptor.hpp>
#include <ge/gfx/Model.hpp>
#include <ge/gfx/RenderPass.hpp>
#include <ge/gfx/Texture.hpp>
#include <ge/systems/Camera3D.hpp>
#include <ge/systems/Systems.hpp>

namespace GE
{
	namespace Sys
	{
		namespace
		{
			static const char* SKYBOX_CUBE = R""(
				v 0 0 0
				v 1 0 0
				v 1 1 0
				v 0 1 0
				v 1 1 1
				v 0 1 1
				v 1 0 1
				v 0 0 1
				vt 0.25 0.33334
				vt 0.50 0.33334
				vt 0.50 0.66666
				vt 0.25 0.66666
				vt 0.50 1.00000
				vt 0.25 1.00000
				vt 0.75 0.66666
				vt 0.75 0.33334
				vt 0.25 0.00000
				vt 0.50 0.00000
				vt 0.00 0.33334
				vt 0.00 0.66666
				vt 1.00 0.33334
				vt 1.00 0.66666
				f 1/1 2/2 3/3
				f 1/1 3/3 4/4
				f 4/4 3/3 5/5
				f 4/4 5/5 6/6
				f 2/2 5/7 3/3
				f 2/2 7/8 5/7
				f 1/1 8/9 2/2
				f 2/2 8/9 7/10
				f 4/4 8/11 1/1
				f 4/4 6/12 8/11
				f 6/14 7/8 8/13
				f 6/14 5/7 7/8
			)"";
		}

		class SkyboxSystem : public System
		{
		public:
			SkyboxSystem();
			void SetImage(const std::string& texturePath);

			void Attach();
			void Detach();

			virtual void RenderScene() override;

			void Resize(Gfx::ResizeEvent evt);
			void UpdateCamera(const Camera3DData& cameraData);

			virtual void Update(int64_t tsMicroseconds) override {
				totalTimeAlive += tsMicroseconds;
			}

		private:
			bool _active{ false };
			std::string _texturePath{ "" };
			Camera3DData _cameraData;

			Gfx::VulkanCommandBuffers commandBuffers;
			Gfx::Model _skyboxModel;
			Gfx::ModelObject _skyboxObject;

			Gfx::ScreenSpaceRenderPass defaultRenderPass;
			Gfx::VulkanTexture skyboxTexture;

			Gfx::VulkanVertexBuffer buffer;
			Gfx::VulkanVertexBuffer buffer2;
			Gfx::VulkanIndexBuffer ibo;

			Gfx::VulkanDescriptorsPool descriptorPool;
		
			int64_t totalTimeAlive = 0;
		};
	}
}
