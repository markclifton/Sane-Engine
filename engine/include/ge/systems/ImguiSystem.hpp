#pragma once

#include <backends/imgui_impl_vulkan.h>

#include <ge/gfx/CommandBuffers.hpp>
#include <ge/gfx/Descriptor.hpp>
#include <ge/gfx/RenderPass.hpp>
#include <ge/systems/Systems.hpp>

namespace GE
{
	namespace Sys
	{
		class ImguiSystem : public System, public Gfx::VulkanGfxElement
		{
		public:
			ImguiSystem();

			void Create();
			void Destroy();

			virtual void Configure();

			void CreateImguiData();
			void DestroyImguiData();

			virtual void Begin() override;
			virtual void End() override;

			virtual void Update(int64_t tsMicroseconds) override;
			virtual void Resize(const Gfx::ResizeEvent& event);

		private:
			ImGuiContext* _context = nullptr;

			Gfx::VulkanCommandBuffers _commandBuffers;
			Gfx::VulkanDescriptorsPool _descriptorPool;
			Gfx::ScreenSpaceRenderPass _renderPass;
		};
	}
}