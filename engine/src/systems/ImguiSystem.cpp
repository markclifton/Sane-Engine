#include <ge/systems/ImguiSystem.hpp>

#include <ge/core/Global.hpp>
#include <ge/gfx/Common.hpp>
#include <ge/gfx/Device.hpp>
#include <ge/gfx/Surface.hpp>
#include <ge/utils/Types.hpp>
#include <ge/utils/FileLoading.hpp>

#include <resources/EngineShaders.hpp>

namespace GE
{
	namespace Sys
	{
		ImguiSystem::ImguiSystem()
			: GE::Sys::System("ImguiSystem")
		{
		}

		void ImguiSystem::Create()
		{
			_commandBuffers.Create(Gfx::GetNumFrames());
			_renderPass.Create();
			CreateImguiData();

			GE::GlobalDispatcher().sink<Gfx::ResizeEvent>().connect<&ImguiSystem::Resize>(this);
		}

		void ImguiSystem::Destroy()
		{
			GE::GlobalDispatcher().sink<Gfx::ResizeEvent>().disconnect<&ImguiSystem::Resize>(this);
			
			DestroyImguiData();
			_descriptorPool.Destroy();
			_renderPass.Destroy();
			_commandBuffers.Destroy();
		}

		void ImguiSystem::Configure()
		{
			std::vector<VkVertexInputAttributeDescription> vertexAttribsDescs = {
					{ 0, 0, VK_FORMAT_R32G32_SFLOAT, 0 },
					{ 1, 0, VK_FORMAT_R32G32_SFLOAT, 0 },
					{ 2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0 }
			};

			std::vector<VkVertexInputBindingDescription> vertexBufferDescs = {
				{ 0, 2_FLOAT, VK_VERTEX_INPUT_RATE_VERTEX },
				{ 1, 2_FLOAT, VK_VERTEX_INPUT_RATE_VERTEX },
				{ 2, 4_FLOAT, VK_VERTEX_INPUT_RATE_VERTEX }
			};
			_renderPass.AddShaderBinaryData(std::string(SHADER_VERT_IMGUI), VK_SHADER_STAGE_VERTEX_BIT);
			_renderPass.AddShaderBinaryData(std::string(SHADER_FRAG_IMGUI), VK_SHADER_STAGE_FRAGMENT_BIT);
			_renderPass.AddVertexAttribDescs(vertexAttribsDescs);
			_renderPass.AddVertexBufferDescs(vertexBufferDescs);

			_descriptorPool.Create(
				{
					{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
					{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
					{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
					{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
					{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
					{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
					{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
					{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
					{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
					{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
					{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
				},
				VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
				1000
			);

			_descriptorPool.CreateNewDescriptorSet({
				{{ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT , nullptr }, 0 }
			});

			_renderPass.AddDescriptorsSetLayouts({ _descriptorPool.GetDescriptorSetLayout(0) });
			_renderPass.Configure();
		}

		void ImguiSystem::CreateImguiData()
		{
			if (!_context)
			{
				IMGUI_CHECKVERSION();
				_context = ImGui::CreateContext();
			}

			ImGuiIO& io = ImGui::GetIO();
			io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
			io.DisplaySize = ImVec2(static_cast<float>(1280), static_cast<float>(720));
#ifdef _WIN32
			io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
#else
			io.DisplayFramebufferScale = ImVec2(1.5f, 1.5f);
#endif // _WIN32

			io.DeltaTime = 1.0f / 60.0f;
			io.IniFilename = nullptr;

			ImGui_ImplVulkan_InitInfo init_info = {};
			init_info.Instance = *_core.instance;
			init_info.PhysicalDevice = *_core.device;
			init_info.Device = *_core.device;
			init_info.QueueFamily = _core.device->GetQueueFamiles(*_core.device, *_core.surface).graphicsFamily;
			init_info.Queue = _core.device->GetGraphicsQueue();
			init_info.DescriptorPool = _descriptorPool;
			init_info.Subpass = 0;
			init_info.MinImageCount = static_cast<int32_t>(Gfx::GetNumFrames());
			init_info.ImageCount = static_cast<int32_t>(Gfx::GetNumFrames());
			init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
			init_info.Allocator = nullptr;
			init_info.CheckVkResultFn = [](VkResult result) {
				GE_ASSERT(result == VK_SUCCESS, "Failed ImGui_ImplVulkan_Init: {}", result);
			};
			ImGui_ImplVulkan_Init(&init_info, _renderPass.Raw());

			// Upload Font Atlas to the GPU
			io.Fonts->AddFontDefault();
			{
				VkCommandBuffer command_buffer = _commandBuffers.GetBuffer();

				VkCommandBufferBeginInfo begin_info = {};
				begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				
				VkResult result = vkBeginCommandBuffer(command_buffer, &begin_info);
				GE_ASSERT(result == VK_SUCCESS);

				ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

				VkSubmitInfo end_info = {};
				end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
				end_info.commandBufferCount = 1;
				end_info.pCommandBuffers = &command_buffer;
				
				result = vkEndCommandBuffer(command_buffer);
				GE_ASSERT(result == VK_SUCCESS, "Failed vkEndCommandBuffer: {}", result);

				result = vkQueueSubmit(_core.device->GetGraphicsQueue(), 1, &end_info, VK_NULL_HANDLE);
				GE_ASSERT(result == VK_SUCCESS, "Failed vkQueueSubmit: {}", result);

				result = vkDeviceWaitIdle(*_core.device);
				GE_ASSERT(result == VK_SUCCESS, "Failed vkDeviceWaitIdle: {}", result);

				ImGui_ImplVulkan_DestroyFontUploadObjects();
			}
		}

		void ImguiSystem::DestroyImguiData()
		{
			ImGui_ImplVulkan_Shutdown();
		}

		void ImguiSystem::Begin()
		{
			ImGui::NewFrame();
			VkCommandBuffer& currentBuffer = _commandBuffers.GetBuffer();
			_renderPass.Begin(currentBuffer);

			VkViewport viewport{};
			viewport.x = 0.f;
			viewport.y = 0.f;
			viewport.width = static_cast<float>(_core.swapchain->GetExtent().width);
			viewport.height = static_cast<float>(_core.swapchain->GetExtent().height);
			viewport.minDepth = 0.f;
			viewport.maxDepth = 1.f;
			VkRect2D scissor{ {0,0}, _core.swapchain->GetExtent() };

			vkCmdSetViewport(currentBuffer, 0, 1, &viewport);
			vkCmdSetScissor(currentBuffer, 0, 1, &scissor);
		}
		
		void ImguiSystem::End()
		{
			VkCommandBuffer& currentBuffer = _commandBuffers.GetBuffer();

			ImGui::Render();
			ImDrawData* data = ImGui::GetDrawData();
			if (data) 
			{
				ImGui_ImplVulkan_RenderDrawData(data, currentBuffer);

				_renderPass.End(currentBuffer);
				_commandBuffers.SubmitBuffer();
			}
		}

		void ImguiSystem::Update(int64_t tsMicroseconds)
		{
			ImGui::GetIO().DeltaTime = (tsMicroseconds / 1000000.f);
		}

		void ImguiSystem::Resize(const Gfx::ResizeEvent& evt)
		{
			vkDeviceWaitIdle(*_core.device);

			if (evt.recreate)
			{
				ImGui::GetIO().DisplaySize = ImVec2(static_cast<float>(_core.swapchain->GetExtent().width), static_cast<float>(_core.swapchain->GetExtent().height));
				_renderPass.Resize(_core.swapchain->GetExtent());
				_commandBuffers.Reset();
			}
		}
	}
}

// Imgui Src Compilation
#include "imgui.cpp"
#include "imgui_demo.cpp"
#include "imgui_draw.cpp"
#include "backends/imgui_impl_vulkan.cpp"
#include "imgui_tables.cpp"
#include "imgui_widgets.cpp"