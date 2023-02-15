#include <ge/gfx/Pipeline.hpp>

#include <array>
#include <bitset>
#include <iterator>

#include <ge/gfx/Device.hpp>
#include <ge/utils/Common.hpp>

namespace
{
	typedef std::pair<std::vector<VkShaderModule>, std::vector<VkPipelineShaderStageCreateInfo>> LoadShaderResult;

	VkShaderModule CreateShaderModule(VkDevice device, const char* code, std::size_t size)
	{
		VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
		shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderModuleCreateInfo.codeSize = size;
		shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(code);

		VkShaderModule shaderModule;
		VkResult result = vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &shaderModule);
		GE_ASSERT(result == VK_SUCCESS, "Failed vkCreateShaderModule: {}", result);
		GE_UNUSED(result);

		return shaderModule;
	}

	LoadShaderResult LoadShaders(VkDevice device, std::vector<GE::Gfx::ShaderData>& shaderData)
	{
		std::vector<VkShaderModule> shaderModules;
		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
		for (auto& shader : shaderData)
		{
			std::vector<char> shaderContents;
			if (shader.data.empty()) {
				shaderContents = GE::Utils::LoadFile(shader.path.c_str());
			}
			else
			{
				std::string str = "";
				for (const char& ch : shader.data)
				{
					if (ch != ' ') {
						str += ch;
					}
					else {
						if (str != "") shaderContents.push_back(stoi(str));
						str = "";
					}
				}
				shaderContents.push_back(0x0);
			}
			VkShaderModule shaderModule = CreateShaderModule(device, shaderContents.data(), shaderContents.size());

			VkPipelineShaderStageCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			createInfo.stage = shader.type;
			createInfo.flags = 0;
			createInfo.module = shaderModule;
			createInfo.pName = "main";

			shaderStages.push_back(createInfo);
			shaderModules.push_back(shaderModule);
		}

		return { shaderModules, shaderStages };
	}
}

namespace GE
{
	namespace Gfx
	{
		VkShaderStageFlagBits GetShaderType(const std::string& path)
		{
			if (Utils::Contains(path, ".vert."))
			{
				return VK_SHADER_STAGE_VERTEX_BIT;
			}
			else if (Utils::Contains(path, ".frag."))
			{
				return VK_SHADER_STAGE_FRAGMENT_BIT;
			}
			else
			{
				GE_ASSERT(0, "Shader Type Unhandled: {}", path.c_str());
				return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
			}
		}

		void VulkanPipeline::Destroy()
		{
			vkDestroyDescriptorSetLayout(*_core.device, _descriptorSetLayout, nullptr);
			vkDestroyPipelineLayout(*_core.device, _layout, nullptr);
			vkDestroyPipeline(*_core.device, _pipeline, nullptr);
			vkDestroyRenderPass(*_core.device, _renderPass, nullptr);

			_layout = VK_NULL_HANDLE;
			_pipeline = VK_NULL_HANDLE;
			_renderPass = VK_NULL_HANDLE;
		}

		void VulkanScreenSpacePipeline::Create(PipelineCreationData& data)
		{
			LoadShaderResult loadShaderResults = LoadShaders(*_core.device, data.shaders);

			VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
			vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputCreateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(data.bufferDescs.size());
			vertexInputCreateInfo.pVertexBindingDescriptions = data.bufferDescs.data(); // List of Vertex Binding Description (stride, spacing)
			vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(data.attribs.size());
			vertexInputCreateInfo.pVertexAttributeDescriptions = data.attribs.data(); // List of Vertex Attribute Descriptions (format, location)

			VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
			inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			inputAssembly.primitiveRestartEnable = VK_FALSE;

			VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
			viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportStateCreateInfo.viewportCount = 1;
			viewportStateCreateInfo.pViewports = nullptr;
			viewportStateCreateInfo.scissorCount = 1;
			viewportStateCreateInfo.pScissors = nullptr;

			VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
			rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizerCreateInfo.depthClampEnable = VK_FALSE;
			rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;
			rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
			rasterizerCreateInfo.lineWidth = 1.f;
			rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
			rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
			rasterizerCreateInfo.depthBiasEnable = VK_FALSE;

			VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = {};
			multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
			multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

			VkPipelineColorBlendAttachmentState colorState = {};
			colorState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			colorState.blendEnable = VK_TRUE;
			colorState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			colorState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			colorState.colorBlendOp = VK_BLEND_OP_ADD;
			colorState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			colorState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			colorState.alphaBlendOp = VK_BLEND_OP_ADD;

			VkPipelineColorBlendStateCreateInfo colorBlendingCreateInfo = {};
			colorBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colorBlendingCreateInfo.logicOpEnable = VK_FALSE;
			colorBlendingCreateInfo.attachmentCount = 1;
			colorBlendingCreateInfo.pAttachments = &colorState;

			VkPipelineDepthStencilStateCreateInfo depthStencil = {};
			depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthStencil.depthTestEnable = VK_TRUE;
			depthStencil.depthWriteEnable = VK_TRUE;
			depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
			depthStencil.depthBoundsTestEnable = VK_FALSE;
			depthStencil.stencilTestEnable = VK_FALSE;

			VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
			pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(data.descriptorSetLayouts.size());
			pipelineLayoutCreateInfo.pSetLayouts = data.descriptorSetLayouts.data();
			pipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32_t>(data.pushConstants.size());
			pipelineLayoutCreateInfo.pPushConstantRanges = data.pushConstants.data();

			CreateRenderPass(data);

			std::vector<VkDynamicState> dynamicStateEnables{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
			VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
			dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicStateCreateInfo.pDynamicStates = dynamicStateEnables.data();
			dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
			dynamicStateCreateInfo.flags = 0;

			VkResult result = vkCreatePipelineLayout(*_core.device, &pipelineLayoutCreateInfo, nullptr, &_layout);
			GE_ASSERT(result == VK_SUCCESS, "Failed vkCreatePipelineLayout: {}", result);

			VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
			graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			graphicsPipelineCreateInfo.stageCount = static_cast<uint32_t>(loadShaderResults.second.size());
			graphicsPipelineCreateInfo.pStages = loadShaderResults.second.data();
			graphicsPipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
			graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssembly;
			graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
			graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
			graphicsPipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
			graphicsPipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;
			graphicsPipelineCreateInfo.pColorBlendState = &colorBlendingCreateInfo;
			graphicsPipelineCreateInfo.pDepthStencilState = &depthStencil;
			graphicsPipelineCreateInfo.layout = _layout;
			graphicsPipelineCreateInfo.renderPass = _renderPass;
			graphicsPipelineCreateInfo.subpass = 0;
			graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
			graphicsPipelineCreateInfo.basePipelineIndex = -1;

			result = vkCreateGraphicsPipelines(*_core.device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &_pipeline);
			GE_ASSERT(result == VK_SUCCESS, "Failed vkCreateGraphicsPipelines: {}", result);

			for (auto& shaderModule : loadShaderResults.first)
			{
				vkDestroyShaderModule(*_core.device, shaderModule, nullptr);
			}
		}

		void VulkanScreenSpacePipeline::CreateRenderPass(PipelineCreationData& data)
		{
			VkAttachmentDescription colorAttachment{};
			colorAttachment.format = _core.swapchain->GetFormat(); // VK_FORMAT_B8G8R8A8_UNORM;
			colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachment.loadOp = data.clearFlag ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			VkAttachmentReference colorAttachmentReference{};
			colorAttachmentReference.attachment = 0;
			colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkAttachmentDescription depthAttachment{};
			depthAttachment.format = FindDepthFormat();
			depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkAttachmentReference depthAttachmentRef{ 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &colorAttachmentReference;
			subpass.pDepthStencilAttachment = &depthAttachmentRef;

			std::array<VkSubpassDependency, 2> subpassDependecies;
			subpassDependecies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			subpassDependecies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			subpassDependecies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			subpassDependecies[0].dstSubpass = 0;
			subpassDependecies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			subpassDependecies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			subpassDependecies[0].dependencyFlags = 0;

			subpassDependecies[1].srcSubpass = 0;
			subpassDependecies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			subpassDependecies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			subpassDependecies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
			subpassDependecies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			subpassDependecies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			subpassDependecies[1].dependencyFlags = 0;

			std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

			VkRenderPassCreateInfo renderPassCreateInfo{};
			renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			renderPassCreateInfo.pAttachments = attachments.data();
			renderPassCreateInfo.subpassCount = 1;
			renderPassCreateInfo.pSubpasses = &subpass;
			renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(subpassDependecies.size());
			renderPassCreateInfo.pDependencies = subpassDependecies.data();

			VkResult result = vkCreateRenderPass(*_core.device, &renderPassCreateInfo, nullptr, &_renderPass);
			GE_ASSERT(result == VK_SUCCESS, "Failed vkCreateRenderPass: {}", result);
			GE_UNUSED(result);
		}

		void VulkanOffscreenPipeline::Create(PipelineCreationData& data)
		{
			LoadShaderResult loadShaderResults = LoadShaders(*_core.device, data.shaders);

			VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
			vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputCreateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(data.bufferDescs.size());
			vertexInputCreateInfo.pVertexBindingDescriptions = data.bufferDescs.data(); // List of Vertex Binding Description (stride, spacing)
			vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(data.attribs.size());
			vertexInputCreateInfo.pVertexAttributeDescriptions = data.attribs.data(); // List of Vertex Attribute Descriptions (format, location)

			VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
			inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			inputAssembly.primitiveRestartEnable = VK_FALSE;

			VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
			viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportStateCreateInfo.viewportCount = 1;
			viewportStateCreateInfo.pViewports = nullptr;
			viewportStateCreateInfo.scissorCount = 1;
			viewportStateCreateInfo.pScissors = nullptr;

			VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
			rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizerCreateInfo.depthClampEnable = VK_FALSE;
			rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;
			rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
			rasterizerCreateInfo.lineWidth = 1.f;
			rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
			rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
			rasterizerCreateInfo.depthBiasEnable = VK_FALSE;

			VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = {};
			multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
			multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

			VkPipelineColorBlendAttachmentState colorState = {};
			colorState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			colorState.blendEnable = VK_TRUE;
			colorState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			colorState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			colorState.colorBlendOp = VK_BLEND_OP_ADD;
			colorState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			colorState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			colorState.alphaBlendOp = VK_BLEND_OP_ADD;

			VkPipelineColorBlendStateCreateInfo colorBlendingCreateInfo = {};
			colorBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colorBlendingCreateInfo.logicOpEnable = VK_FALSE;
			colorBlendingCreateInfo.attachmentCount = 1;
			colorBlendingCreateInfo.pAttachments = &colorState;

			VkPipelineDepthStencilStateCreateInfo depthStencil = {};
			depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthStencil.depthTestEnable = VK_TRUE;
			depthStencil.depthWriteEnable = VK_TRUE;
			depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
			depthStencil.depthBoundsTestEnable = VK_FALSE;
			depthStencil.stencilTestEnable = VK_FALSE;

			VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
			pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(data.descriptorSetLayouts.size());
			pipelineLayoutCreateInfo.pSetLayouts = data.descriptorSetLayouts.data();
			pipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32_t>(data.pushConstants.size());
			pipelineLayoutCreateInfo.pPushConstantRanges = data.pushConstants.data();

			CreateRenderPass(data);

			std::vector<VkDynamicState> dynamicStateEnables{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
			VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
			dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicStateCreateInfo.pDynamicStates = dynamicStateEnables.data();
			dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
			dynamicStateCreateInfo.flags = 0;

			VkResult result = vkCreatePipelineLayout(*_core.device, &pipelineLayoutCreateInfo, nullptr, &_layout);
			GE_ASSERT(result == VK_SUCCESS, "Failed vkCreatePipelineLayout: {}", result);

			VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
			graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			graphicsPipelineCreateInfo.stageCount = static_cast<uint32_t>(loadShaderResults.second.size());
			graphicsPipelineCreateInfo.pStages = loadShaderResults.second.data();
			graphicsPipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
			graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssembly;
			graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
			graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
			graphicsPipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
			graphicsPipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;
			graphicsPipelineCreateInfo.pColorBlendState = &colorBlendingCreateInfo;
			graphicsPipelineCreateInfo.pDepthStencilState = &depthStencil;
			graphicsPipelineCreateInfo.layout = _layout;
			graphicsPipelineCreateInfo.renderPass = _renderPass;
			graphicsPipelineCreateInfo.subpass = 0;
			graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
			graphicsPipelineCreateInfo.basePipelineIndex = -1;

			result = vkCreateGraphicsPipelines(*_core.device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &_pipeline);
			GE_ASSERT(result == VK_SUCCESS, "Failed vkCreateGraphicsPipelines: {}", result);

			for (auto& shaderModule : loadShaderResults.first)
			{
				vkDestroyShaderModule(*_core.device, shaderModule, nullptr);
			}
		}

		void VulkanOffscreenPipeline::CreateRenderPass(PipelineCreationData& data)
		{
			VkAttachmentDescription colorAttachment{};
			colorAttachment.format = VK_FORMAT_B8G8R8A8_UNORM;
			colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachment.loadOp = data.clearFlag ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkAttachmentReference colorAttachmentReference{};
			colorAttachmentReference.attachment = 0;
			colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkAttachmentDescription depthAttachment{};
			depthAttachment.format = FindDepthFormat();
			depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkAttachmentReference depthAttachmentRef{ 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &colorAttachmentReference;
			subpass.pDepthStencilAttachment = &depthAttachmentRef;

			std::array<VkSubpassDependency, 2> subpassDependecies;
			subpassDependecies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			subpassDependecies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			subpassDependecies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			subpassDependecies[0].dstSubpass = 0;
			subpassDependecies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			subpassDependecies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			subpassDependecies[0].dependencyFlags = 0;

			subpassDependecies[1].srcSubpass = 0;
			subpassDependecies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			subpassDependecies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			subpassDependecies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
			subpassDependecies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			subpassDependecies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			subpassDependecies[1].dependencyFlags = 0;

			std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

			VkRenderPassCreateInfo renderPassCreateInfo{};
			renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			renderPassCreateInfo.pAttachments = attachments.data();
			renderPassCreateInfo.subpassCount = 1;
			renderPassCreateInfo.pSubpasses = &subpass;
			renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(subpassDependecies.size());
			renderPassCreateInfo.pDependencies = subpassDependecies.data();

			VkResult result = vkCreateRenderPass(*_core.device, &renderPassCreateInfo, nullptr, &_renderPass);
			GE_ASSERT(result == VK_SUCCESS, "Failed vkCreateRenderPass: {}", result);
			GE_UNUSED(result);
		}

		void VulkanMSAAOffscreenPipeline::Create(PipelineCreationData& data)
		{
			LoadShaderResult loadShaderResults = LoadShaders(*_core.device, data.shaders);

			VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
			vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputCreateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(data.bufferDescs.size());
			vertexInputCreateInfo.pVertexBindingDescriptions = data.bufferDescs.data(); // List of Vertex Binding Description (stride, spacing)
			vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(data.attribs.size());
			vertexInputCreateInfo.pVertexAttributeDescriptions = data.attribs.data(); // List of Vertex Attribute Descriptions (format, location)

			VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
			inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			inputAssembly.primitiveRestartEnable = VK_FALSE;

			VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
			viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportStateCreateInfo.viewportCount = 1;
			viewportStateCreateInfo.pViewports = nullptr;
			viewportStateCreateInfo.scissorCount = 1;
			viewportStateCreateInfo.pScissors = nullptr;

			VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
			rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizerCreateInfo.depthClampEnable = VK_FALSE;
			rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;
			rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
			rasterizerCreateInfo.lineWidth = 1.f;
			rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
			rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
			rasterizerCreateInfo.depthBiasEnable = VK_FALSE;

			VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = {};
			multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
			multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_8_BIT;

			VkPipelineColorBlendAttachmentState colorState = {};
			colorState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			colorState.blendEnable = VK_TRUE;
			colorState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			colorState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			colorState.colorBlendOp = VK_BLEND_OP_ADD;
			colorState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			colorState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			colorState.alphaBlendOp = VK_BLEND_OP_ADD;

			VkPipelineColorBlendStateCreateInfo colorBlendingCreateInfo = {};
			colorBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colorBlendingCreateInfo.logicOpEnable = VK_FALSE;
			colorBlendingCreateInfo.attachmentCount = 1;
			colorBlendingCreateInfo.pAttachments = &colorState;

			VkPipelineDepthStencilStateCreateInfo depthStencil = {};
			depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthStencil.depthTestEnable = VK_TRUE;
			depthStencil.depthWriteEnable = VK_TRUE;
			depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
			depthStencil.depthBoundsTestEnable = VK_FALSE;
			depthStencil.stencilTestEnable = VK_FALSE;

			VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
			pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(data.descriptorSetLayouts.size());
			pipelineLayoutCreateInfo.pSetLayouts = data.descriptorSetLayouts.data();
			pipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32_t>(data.pushConstants.size());
			pipelineLayoutCreateInfo.pPushConstantRanges = data.pushConstants.data();

			CreateRenderPass(data);

			std::vector<VkDynamicState> dynamicStateEnables{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
			VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
			dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicStateCreateInfo.pDynamicStates = dynamicStateEnables.data();
			dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
			dynamicStateCreateInfo.flags = 0;

			VkResult result = vkCreatePipelineLayout(*_core.device, &pipelineLayoutCreateInfo, nullptr, &_layout);
			GE_ASSERT(result == VK_SUCCESS, "Failed vkCreatePipelineLayout: {}", result);

			VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
			graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			graphicsPipelineCreateInfo.stageCount = static_cast<uint32_t>(loadShaderResults.second.size());
			graphicsPipelineCreateInfo.pStages = loadShaderResults.second.data();
			graphicsPipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
			graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssembly;
			graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
			graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
			graphicsPipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
			graphicsPipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;
			graphicsPipelineCreateInfo.pColorBlendState = &colorBlendingCreateInfo;
			graphicsPipelineCreateInfo.pDepthStencilState = &depthStencil;
			graphicsPipelineCreateInfo.layout = _layout;
			graphicsPipelineCreateInfo.renderPass = _renderPass;
			graphicsPipelineCreateInfo.subpass = 0;
			graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
			graphicsPipelineCreateInfo.basePipelineIndex = -1;

			result = vkCreateGraphicsPipelines(*_core.device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &_pipeline);
			GE_ASSERT(result == VK_SUCCESS, "Failed vkCreateGraphicsPipelines: {}", result);

			for (auto& shaderModule : loadShaderResults.first)
			{
				vkDestroyShaderModule(*_core.device, shaderModule, nullptr);
			}
		}

		void VulkanMSAAOffscreenPipeline::CreateRenderPass(PipelineCreationData& data)
		{
			VkAttachmentDescription colorAttachment{};
			colorAttachment.format = _core.swapchain->GetFormat();
			colorAttachment.samples = VK_SAMPLE_COUNT_8_BIT;
			colorAttachment.loadOp = data.clearFlag ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			VkAttachmentReference colorAttachmentReference{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

			VkAttachmentDescription colorResolveAttachment{};
			colorResolveAttachment.format = _core.swapchain->GetFormat();
			colorResolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			colorResolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorResolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorResolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorResolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorResolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorResolveAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			VkAttachmentReference colorResolveAttachmentReference{ 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

			VkAttachmentDescription depthAttachment{};
			depthAttachment.format = FindDepthFormat();
			depthAttachment.samples = VK_SAMPLE_COUNT_8_BIT;
			depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			VkAttachmentReference depthAttachmentRef{ 2, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &colorAttachmentReference;
			subpass.pDepthStencilAttachment = &depthAttachmentRef;
			subpass.pResolveAttachments = &colorResolveAttachmentReference;

			std::array<VkSubpassDependency, 2> subpassDependecies;
			subpassDependecies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			subpassDependecies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			subpassDependecies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			subpassDependecies[0].dstSubpass = 0;
			subpassDependecies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			subpassDependecies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			subpassDependecies[0].dependencyFlags = 0;

			subpassDependecies[1].srcSubpass = 0;
			subpassDependecies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			subpassDependecies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			subpassDependecies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
			subpassDependecies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			subpassDependecies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			subpassDependecies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, colorResolveAttachment, depthAttachment };

			VkRenderPassCreateInfo renderPassCreateInfo{};
			renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			renderPassCreateInfo.pAttachments = attachments.data();
			renderPassCreateInfo.subpassCount = 1;
			renderPassCreateInfo.pSubpasses = &subpass;
			renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(subpassDependecies.size());
			renderPassCreateInfo.pDependencies = subpassDependecies.data();

			VkResult result = vkCreateRenderPass(*_core.device, &renderPassCreateInfo, nullptr, &_renderPass);
			GE_ASSERT(result == VK_SUCCESS, "Failed vkCreateRenderPass: {}", result);
			GE_UNUSED(result);
		}

		void VulkanShadowPipeline::Create(PipelineCreationData& data)
		{
			LoadShaderResult loadShaderResults = LoadShaders(*_core.device, data.shaders);

			VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
			vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputCreateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(data.bufferDescs.size());
			vertexInputCreateInfo.pVertexBindingDescriptions = data.bufferDescs.data(); // List of Vertex Binding Description (stride, spacing)
			vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(data.attribs.size());
			vertexInputCreateInfo.pVertexAttributeDescriptions = data.attribs.data(); // List of Vertex Attribute Descriptions (format, location)

			VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
			inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			inputAssembly.primitiveRestartEnable = VK_FALSE;

			VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
			viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportStateCreateInfo.viewportCount = 1;
			viewportStateCreateInfo.pViewports = nullptr;
			viewportStateCreateInfo.scissorCount = 1;
			viewportStateCreateInfo.pScissors = nullptr;

			VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
			rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizerCreateInfo.depthClampEnable = VK_FALSE;
			rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;
			rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
			rasterizerCreateInfo.lineWidth = 1.f;
			rasterizerCreateInfo.cullMode = VK_CULL_MODE_NONE;
			rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
			rasterizerCreateInfo.depthBiasEnable = VK_TRUE;

			VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = {};
			multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
			multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

			VkPipelineColorBlendAttachmentState colorState = {};
			colorState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			colorState.blendEnable = VK_TRUE;
			colorState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			colorState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			colorState.colorBlendOp = VK_BLEND_OP_ADD;
			colorState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			colorState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			colorState.alphaBlendOp = VK_BLEND_OP_ADD;

			VkPipelineColorBlendStateCreateInfo colorBlendingCreateInfo = {};
			colorBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colorBlendingCreateInfo.logicOpEnable = VK_FALSE;
			colorBlendingCreateInfo.attachmentCount = 0;
			colorBlendingCreateInfo.pAttachments = &colorState;

			VkPipelineDepthStencilStateCreateInfo depthStencil = {};
			depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthStencil.depthTestEnable = VK_TRUE;
			depthStencil.depthWriteEnable = VK_TRUE;
			depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
			depthStencil.depthBoundsTestEnable = VK_FALSE;
			depthStencil.stencilTestEnable = VK_FALSE;

			VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
			pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(data.descriptorSetLayouts.size());
			pipelineLayoutCreateInfo.pSetLayouts = data.descriptorSetLayouts.data();
			pipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32_t>(data.pushConstants.size());
			pipelineLayoutCreateInfo.pPushConstantRanges = data.pushConstants.data();

			CreateRenderPass(data);

			std::vector<VkDynamicState> dynamicStateEnables{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_DEPTH_BIAS };
			VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
			dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicStateCreateInfo.pDynamicStates = dynamicStateEnables.data();
			dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
			dynamicStateCreateInfo.flags = 0;

			VkResult result = vkCreatePipelineLayout(*_core.device, &pipelineLayoutCreateInfo, nullptr, &_layout);
			GE_ASSERT(result == VK_SUCCESS, "Failed vkCreatePipelineLayout: {}", result);

			VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
			graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			graphicsPipelineCreateInfo.stageCount = static_cast<uint32_t>(loadShaderResults.second.size());
			graphicsPipelineCreateInfo.pStages = loadShaderResults.second.data();
			graphicsPipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
			graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssembly;
			graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
			graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
			graphicsPipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
			graphicsPipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;
			graphicsPipelineCreateInfo.pColorBlendState = &colorBlendingCreateInfo;
			graphicsPipelineCreateInfo.pDepthStencilState = &depthStencil;
			graphicsPipelineCreateInfo.layout = _layout;
			graphicsPipelineCreateInfo.renderPass = _renderPass;
			graphicsPipelineCreateInfo.subpass = 0;
			graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
			graphicsPipelineCreateInfo.basePipelineIndex = -1;

			result = vkCreateGraphicsPipelines(*_core.device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &_pipeline);
			GE_ASSERT(result == VK_SUCCESS, "Failed vkCreateGraphicsPipelines: {}", result);

			for (auto& shaderModule : loadShaderResults.first)
			{
				vkDestroyShaderModule(*_core.device, shaderModule, nullptr);
			}
		}

		void VulkanShadowPipeline::CreateRenderPass(PipelineCreationData& data)
		{
			VkAttachmentDescription attachmentDescription{};
			attachmentDescription.format = FindDepthFormat();
			attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
			attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;							// Clear depth at beginning of the render pass
			attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;						// We will read from depth, so it's important to store the depth attachment results
			attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;					// We don't care about initial layout of the attachment
			attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;// Attachment will be transitioned to shader read at render pass end

			VkAttachmentReference depthReference = {};
			depthReference.attachment = 0;
			depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;			// Attachment will be used as depth/stencil during render pass

			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 0;													// No color attachments
			subpass.pDepthStencilAttachment = &depthReference;									// Reference to our depth attachment

			// Use subpass dependencies for layout transitions
			std::array<VkSubpassDependency, 2> dependencies;

			dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[0].dstSubpass = 0;
			dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
			dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			dependencies[1].srcSubpass = 0;
			dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
			dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

			VkRenderPassCreateInfo renderPassCreateInfo{};
			renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassCreateInfo.attachmentCount = 1;
			renderPassCreateInfo.pAttachments = &attachmentDescription;
			renderPassCreateInfo.subpassCount = 1;
			renderPassCreateInfo.pSubpasses = &subpass;
			renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
			renderPassCreateInfo.pDependencies = dependencies.data();

			vkCreateRenderPass(*_core.device, &renderPassCreateInfo, nullptr, &_renderPass);
		}

		void VulkanMultipassShadowPipeline::Create(PipelineCreationData& data)
		{
			LoadShaderResult loadShaderResults = LoadShaders(*_core.device, data.shaders);

			VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
			vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputCreateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(data.bufferDescs.size());
			vertexInputCreateInfo.pVertexBindingDescriptions = data.bufferDescs.data(); // List of Vertex Binding Description (stride, spacing)
			vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(data.attribs.size());
			vertexInputCreateInfo.pVertexAttributeDescriptions = data.attribs.data(); // List of Vertex Attribute Descriptions (format, location)

			VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
			inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			inputAssembly.primitiveRestartEnable = VK_FALSE;

			VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
			viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportStateCreateInfo.viewportCount = 1;
			viewportStateCreateInfo.pViewports = nullptr;
			viewportStateCreateInfo.scissorCount = 1;
			viewportStateCreateInfo.pScissors = nullptr;

			VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
			rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizerCreateInfo.depthClampEnable = VK_FALSE;
			rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;
			rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
			rasterizerCreateInfo.lineWidth = 1.f;
			rasterizerCreateInfo.cullMode = VK_CULL_MODE_NONE;
			rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
			rasterizerCreateInfo.depthBiasEnable = VK_FALSE;

			VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = {};
			multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
			multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

			VkPipelineColorBlendAttachmentState colorState = {};
			colorState.colorWriteMask = 0xf;
			colorState.blendEnable = VK_FALSE;
			colorState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			colorState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			colorState.colorBlendOp = VK_BLEND_OP_ADD;
			colorState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			colorState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			colorState.alphaBlendOp = VK_BLEND_OP_ADD;

			std::vector<VkPipelineColorBlendAttachmentState> colorStates{ colorState };

			VkPipelineColorBlendStateCreateInfo colorBlendingCreateInfo = {};
			colorBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colorBlendingCreateInfo.logicOpEnable = VK_FALSE;
			colorBlendingCreateInfo.attachmentCount = static_cast<uint32_t>(colorStates.size());
			colorBlendingCreateInfo.pAttachments = colorStates.data();

			VkPipelineDepthStencilStateCreateInfo depthStencil = {};
			depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthStencil.depthTestEnable = VK_TRUE;
			depthStencil.depthWriteEnable = VK_TRUE;
			depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
			depthStencil.depthBoundsTestEnable = VK_FALSE;
			depthStencil.stencilTestEnable = VK_FALSE;

			VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
			pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(data.descriptorSetLayouts.size());
			pipelineLayoutCreateInfo.pSetLayouts = data.descriptorSetLayouts.data();
			pipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32_t>(data.pushConstants.size());
			pipelineLayoutCreateInfo.pPushConstantRanges = data.pushConstants.data();

			CreateRenderPass(data);

			std::vector<VkDynamicState> dynamicStateEnables{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_DEPTH_BIAS };
			VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
			dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicStateCreateInfo.pDynamicStates = dynamicStateEnables.data();
			dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
			dynamicStateCreateInfo.flags = 0;

			VkResult result = vkCreatePipelineLayout(*_core.device, &pipelineLayoutCreateInfo, nullptr, &_layout);
			GE_ASSERT(result == VK_SUCCESS, "Failed vkCreatePipelineLayout: {}", result);

			VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
			graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			graphicsPipelineCreateInfo.stageCount = static_cast<uint32_t>(loadShaderResults.second.size());
			graphicsPipelineCreateInfo.pStages = loadShaderResults.second.data();
			graphicsPipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
			graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssembly;
			graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
			graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
			graphicsPipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
			graphicsPipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;
			graphicsPipelineCreateInfo.pColorBlendState = &colorBlendingCreateInfo;
			graphicsPipelineCreateInfo.pDepthStencilState = &depthStencil;
			graphicsPipelineCreateInfo.layout = _layout;
			graphicsPipelineCreateInfo.renderPass = _renderPass;
			graphicsPipelineCreateInfo.subpass = 0;
			graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
			graphicsPipelineCreateInfo.basePipelineIndex = -1;

			result = vkCreateGraphicsPipelines(*_core.device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &_pipeline);
			GE_ASSERT(result == VK_SUCCESS, "Failed vkCreateGraphicsPipelines: {}", result);

			for (auto& shaderModule : loadShaderResults.first)
			{
				vkDestroyShaderModule(*_core.device, shaderModule, nullptr);
			}
		}

		void VulkanMultipassShadowPipeline::CreateRenderPass(PipelineCreationData& data)
		{
			VkAttachmentDescription colorAttachment{};
			colorAttachment.format = VK_FORMAT_R32_SFLOAT;
			colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachment.loadOp = data.clearFlag ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkAttachmentDescription depthAttachment{};
			depthAttachment.format = FindDepthFormat();
			depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			VkAttachmentReference colorAttachmentReference{ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
			VkAttachmentReference depthAttachmentRef{ 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &colorAttachmentReference;
			subpass.pDepthStencilAttachment = &depthAttachmentRef;

			std::array<VkSubpassDependency, 2> subpassDependecies;
			subpassDependecies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			subpassDependecies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			subpassDependecies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			subpassDependecies[0].dstSubpass = 0;
			subpassDependecies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			subpassDependecies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			subpassDependecies[0].dependencyFlags = 0;

			subpassDependecies[1].srcSubpass = 0;
			subpassDependecies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			subpassDependecies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			subpassDependecies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
			subpassDependecies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			subpassDependecies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			subpassDependecies[1].dependencyFlags = 0;

			std::array<VkAttachmentDescription, 2> attachments{ colorAttachment, depthAttachment };

			VkRenderPassCreateInfo renderPassCreateInfo{};
			renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassCreateInfo.pAttachments = attachments.data();
			renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			renderPassCreateInfo.subpassCount = 1;
			renderPassCreateInfo.pSubpasses = &subpass;
			renderPassCreateInfo.dependencyCount = 2;
			renderPassCreateInfo.pDependencies = subpassDependecies.data();

			VkResult result = vkCreateRenderPass(*_core.device, &renderPassCreateInfo, nullptr, &_renderPass);
			GE_ASSERT(result == VK_SUCCESS, "Failed vkCreateRenderPass: {}", result);
			GE_UNUSED(result);
		}

		void VulkanGBufferPipeline::Create(PipelineCreationData& data)
		{
			LoadShaderResult loadShaderResults = LoadShaders(*_core.device, data.shaders);

			VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
			vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputCreateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(data.bufferDescs.size());
			vertexInputCreateInfo.pVertexBindingDescriptions = data.bufferDescs.data(); // List of Vertex Binding Description (stride, spacing)
			vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(data.attribs.size());
			vertexInputCreateInfo.pVertexAttributeDescriptions = data.attribs.data(); // List of Vertex Attribute Descriptions (format, location)

			VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
			inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			inputAssembly.primitiveRestartEnable = VK_FALSE;

			VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
			viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportStateCreateInfo.viewportCount = 1;
			viewportStateCreateInfo.pViewports = nullptr;
			viewportStateCreateInfo.scissorCount = 1;
			viewportStateCreateInfo.pScissors = nullptr;

			VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
			rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizerCreateInfo.depthClampEnable = VK_FALSE;
			rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;
			rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
			rasterizerCreateInfo.lineWidth = 1.f;
			rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
			rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
			rasterizerCreateInfo.depthBiasEnable = VK_FALSE;

			VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = {};
			multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
			multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

			VkPipelineColorBlendAttachmentState colorState = {};
			colorState.colorWriteMask = 0xf;
			colorState.blendEnable = VK_FALSE;
			colorState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			colorState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			colorState.colorBlendOp = VK_BLEND_OP_ADD;
			colorState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			colorState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			colorState.alphaBlendOp = VK_BLEND_OP_ADD;

			std::vector<VkPipelineColorBlendAttachmentState> colorStates{ colorState, colorState, colorState };

			VkPipelineColorBlendStateCreateInfo colorBlendingCreateInfo = {};
			colorBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			colorBlendingCreateInfo.logicOpEnable = VK_FALSE;
			colorBlendingCreateInfo.attachmentCount = static_cast<uint32_t>(colorStates.size());
			colorBlendingCreateInfo.pAttachments = colorStates.data();

			VkPipelineDepthStencilStateCreateInfo depthStencil = {};
			depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthStencil.depthTestEnable = VK_TRUE;
			depthStencil.depthWriteEnable = VK_TRUE;
			depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
			depthStencil.depthBoundsTestEnable = VK_FALSE;
			depthStencil.stencilTestEnable = VK_FALSE;

			VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
			pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(data.descriptorSetLayouts.size());
			pipelineLayoutCreateInfo.pSetLayouts = data.descriptorSetLayouts.data();
			pipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32_t>(data.pushConstants.size());
			pipelineLayoutCreateInfo.pPushConstantRanges = data.pushConstants.data();

			CreateRenderPass(data);

			std::vector<VkDynamicState> dynamicStateEnables{ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
			VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
			dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicStateCreateInfo.pDynamicStates = dynamicStateEnables.data();
			dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
			dynamicStateCreateInfo.flags = 0;

			VkResult result = vkCreatePipelineLayout(*_core.device, &pipelineLayoutCreateInfo, nullptr, &_layout);
			GE_ASSERT(result == VK_SUCCESS, "Failed vkCreatePipelineLayout: {}", result);

			VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
			graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			graphicsPipelineCreateInfo.stageCount = static_cast<uint32_t>(loadShaderResults.second.size());
			graphicsPipelineCreateInfo.pStages = loadShaderResults.second.data();
			graphicsPipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
			graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssembly;
			graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
			graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
			graphicsPipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
			graphicsPipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;
			graphicsPipelineCreateInfo.pColorBlendState = &colorBlendingCreateInfo;
			graphicsPipelineCreateInfo.pDepthStencilState = &depthStencil;
			graphicsPipelineCreateInfo.layout = _layout;
			graphicsPipelineCreateInfo.renderPass = _renderPass;
			graphicsPipelineCreateInfo.subpass = 0;
			graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
			graphicsPipelineCreateInfo.basePipelineIndex = -1;

			result = vkCreateGraphicsPipelines(*_core.device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &_pipeline);
			GE_ASSERT(result == VK_SUCCESS, "Failed vkCreateGraphicsPipelines: {}", result);

			for (auto& shaderModule : loadShaderResults.first)
			{
				vkDestroyShaderModule(*_core.device, shaderModule, nullptr);
			}
		}

		void VulkanGBufferPipeline::CreateRenderPass(PipelineCreationData& data)
		{
			VkAttachmentDescription colorAttachment{};
			colorAttachment.format = _core.swapchain->GetFormat();
			colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachment.loadOp = data.clearFlag ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkAttachmentDescription depthAttachment{};
			depthAttachment.format = FindDepthFormat();
			depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;	
			depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			depthAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			std::vector<VkAttachmentReference> colorAttachmentReference;
			colorAttachmentReference.push_back({ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
			colorAttachmentReference.push_back({ 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
			colorAttachmentReference.push_back({ 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });

			VkAttachmentReference depthAttachmentRef{ 3, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentReference.size());
			subpass.pColorAttachments = colorAttachmentReference.data();
			subpass.pDepthStencilAttachment = &depthAttachmentRef;

			std::array<VkSubpassDependency, 2> subpassDependecies;
			subpassDependecies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			subpassDependecies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			subpassDependecies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			subpassDependecies[0].dstSubpass = 0;
			subpassDependecies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			subpassDependecies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			subpassDependecies[0].dependencyFlags = 0;

			subpassDependecies[1].srcSubpass = 0;
			subpassDependecies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			subpassDependecies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			subpassDependecies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
			subpassDependecies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			subpassDependecies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			subpassDependecies[1].dependencyFlags = 0;

			std::array<VkAttachmentDescription, 4> attachments{ colorAttachment, colorAttachment, colorAttachment, depthAttachment };

			attachments[0].format = VK_FORMAT_R16G16B16A16_SFLOAT;
			attachments[1].format = VK_FORMAT_R16G16B16A16_SFLOAT;
			attachments[2].format = VK_FORMAT_R16G16B16A16_SFLOAT;
			attachments[3].format = FindDepthFormat();

			VkRenderPassCreateInfo renderPassCreateInfo{};
			renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassCreateInfo.pAttachments = attachments.data();
			renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			renderPassCreateInfo.subpassCount = 1;
			renderPassCreateInfo.pSubpasses = &subpass;
			renderPassCreateInfo.dependencyCount = 2;
			renderPassCreateInfo.pDependencies = subpassDependecies.data();

			VkResult result = vkCreateRenderPass(*_core.device, &renderPassCreateInfo, nullptr, &_renderPass);
			GE_ASSERT(result == VK_SUCCESS, "Failed vkCreateRenderPass: {}", result);
			GE_UNUSED(result);
		}
	}
}