#pragma once
#include "vkdk.hpp"
#include "Components/Meshes/Mesh.hpp"

class Scene;

namespace Components {
	namespace Materials {
		struct PipelineParameters {
			VkRenderPass renderPass;

			VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
			VkPipelineRasterizationStateCreateInfo rasterizer = {};
			
			PipelineParameters() {
				/* Default input assembly */
				inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
				inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
				inputAssembly.primitiveRestartEnable = VK_FALSE;

				/* Default rasterizer */
				rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
				rasterizer.depthClampEnable = VK_FALSE; // Helpful for shadow maps.
				rasterizer.rasterizerDiscardEnable = VK_FALSE; // if true, geometry never goes through rasterization, or to frame buffer
				rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // could be line or point too. those require GPU features to be enabled.
				rasterizer.lineWidth = 1.0f; // anything thicker than 1.0 requires wideLines GPU feature
				rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
				rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
				rasterizer.depthBiasEnable = VK_FALSE;
				rasterizer.depthBiasConstantFactor = 0.0f;
				rasterizer.depthBiasClamp = 0.0f;
				rasterizer.depthBiasSlopeFactor = 0.0f;
			}
		};

		/* A material defines a render interface, and cannot be instantiated directly. */
		class Material {
		public:
			virtual void render(Scene *scene, std::shared_ptr<Components::Meshes::Mesh> mesh) = 0;
			virtual void update(glm::mat4 model, glm::mat4 view, glm::mat4 projection) = 0;
			virtual void cleanup() = 0;

		protected:
			/* Helper function for reading in SPIR-V files */
			static std::vector<char> readFile(const std::string& filename) {
				std::ifstream file(filename, std::ios::ate | std::ios::binary);

				if (!file.is_open()) {
					throw std::runtime_error("failed to open file!");
				}

				size_t fileSize = (size_t)file.tellg();
				std::vector<char> buffer(fileSize);

				file.seekg(0);
				file.read(buffer.data(), fileSize);
				file.close();

				return buffer;
			}
			
			static void createPipelines(
				std::vector<PipelineParameters> pipelineParameters, 
				std::vector<VkPipelineShaderStageCreateInfo> shaderStages,
				std::vector<VkVertexInputBindingDescription> bindingDescriptions,
				std::vector<VkVertexInputAttributeDescription> attributeDescriptions,
				std::vector<VkDescriptorSetLayout> descriptorSetLayouts,
				std::vector<VkPipeline> &pipelines,
				std::vector<VkPipelineLayout> &layouts
			){
				pipelines.resize(pipelineParameters.size());
				layouts.resize(pipelineParameters.size());
				std::vector<VkGraphicsPipelineCreateInfo> pipelineInfos(pipelineParameters.size());

				/* Any of the following parameters may change depending on pipeline parameters. */
				for (int i = 0; i < pipelineParameters.size(); ++i) {
					/* Vertex Input */
					VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
					vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
					vertexInputInfo.vertexBindingDescriptionCount = bindingDescriptions.size();
					vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
					vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
					vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

					/* Viewports and Scissors */
					// Note: This is actually overriden by the dynamic states (see below)
					VkPipelineViewportStateCreateInfo viewportState = {};
					viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
					viewportState.viewportCount = 1;
					viewportState.scissorCount = 1;
					viewportState.pViewports = VK_NULL_HANDLE;
					viewportState.pScissors = VK_NULL_HANDLE;
					
					/* Multisampling */
					VkPipelineMultisampleStateCreateInfo multisampling = {};
					multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
					multisampling.sampleShadingEnable = VK_FALSE;
					multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
					multisampling.minSampleShading = 1.0f; // Optional
					multisampling.pSampleMask = nullptr; // Optional
					multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
					multisampling.alphaToOneEnable = VK_FALSE; // Optional

					/* Depth and Stencil testing */
					VkPipelineDepthStencilStateCreateInfo depthStencil = {};
					depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
					depthStencil.depthTestEnable = VK_TRUE;
					depthStencil.depthWriteEnable = VK_TRUE;
					depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
					depthStencil.depthBoundsTestEnable = VK_FALSE;
					depthStencil.minDepthBounds = 0.0f; // Optional
					depthStencil.maxDepthBounds = 1.0f; // Optional
					depthStencil.stencilTestEnable = VK_FALSE;
					depthStencil.front = {}; // Optional
					depthStencil.back = {}; // Optional

					/* Color Blending */
					VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
					colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
					colorBlendAttachment.blendEnable = VK_TRUE;
					colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA; // Optional /* TRANSPARENCY STUFF HERE! */
					colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; // Optional
					colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
					colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
					colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
					colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

					VkPipelineColorBlendStateCreateInfo colorBlending = {};
					colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
					colorBlending.logicOpEnable = VK_FALSE;
					colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
					colorBlending.attachmentCount = 1;
					colorBlending.pAttachments = &colorBlendAttachment;
					colorBlending.blendConstants[0] = 0.0f; // Optional
					colorBlending.blendConstants[1] = 0.0f; // Optional
					colorBlending.blendConstants[2] = 0.0f; // Optional
					colorBlending.blendConstants[3] = 0.0f; // Optional

					/* Dynamic State */
					VkDynamicState dynamicStates[] = {
						VK_DYNAMIC_STATE_VIEWPORT,
						VK_DYNAMIC_STATE_SCISSOR,
						VK_DYNAMIC_STATE_LINE_WIDTH
					};

					VkPipelineDynamicStateCreateInfo dynamicState = {};
					dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
					dynamicState.dynamicStateCount = 3;
					dynamicState.pDynamicStates = dynamicStates;

					/* Connect things together with pipeline layout */
					VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
					pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
					pipelineLayoutInfo.setLayoutCount = descriptorSetLayouts.size();
					pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
					pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
					pipelineLayoutInfo.pPushConstantRanges = 0; // Optional

					if (vkCreatePipelineLayout(VKDK::device, &pipelineLayoutInfo, nullptr, &layouts[i]) != VK_SUCCESS) {
						throw std::runtime_error("failed to create pipeline layout!");
					}

					/* Now we can create a graphics pipeline! */
					pipelineInfos[i].sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
					pipelineInfos[i].stageCount = shaderStages.size();
					pipelineInfos[i].pStages = shaderStages.data();
					pipelineInfos[i].pVertexInputState = &vertexInputInfo;
					pipelineInfos[i].pInputAssemblyState = &pipelineParameters[i].inputAssembly;
					pipelineInfos[i].pViewportState = &viewportState;
					pipelineInfos[i].pRasterizationState = &pipelineParameters[i].rasterizer;
					pipelineInfos[i].pMultisampleState = &multisampling;
					pipelineInfos[i].pDepthStencilState = &depthStencil;
					pipelineInfos[i].pColorBlendState = &colorBlending;
					pipelineInfos[i].pDynamicState = &dynamicState; // Optional
					pipelineInfos[i].layout = layouts[i];
					pipelineInfos[i].renderPass = pipelineParameters[i].renderPass;
					pipelineInfos[i].subpass = 0;
					pipelineInfos[i].basePipelineHandle = VK_NULL_HANDLE; // Optional
					pipelineInfos[i].basePipelineIndex = -1; // Optional
				}

				if (vkCreateGraphicsPipelines(VKDK::device, VK_NULL_HANDLE, pipelineInfos.size(), pipelineInfos.data(), nullptr, pipelines.data()) != VK_SUCCESS) {
					throw std::runtime_error("failed to create graphics pipeline!");
				}
			}
		};
	}
}

/* Halo Materials */
#include "Components/Materials/HaloMaterials/UniformColoredPoints.hpp"

/* Surface Materials */
#include "Components/Materials/SurfaceMaterials/UniformColoredSurface.hpp"
#include "Components/Materials/SurfaceMaterials/NormalSurface.hpp"
#include "Components/Materials/SurfaceMaterials/TexCoordSurface.hpp"
#include "Components/Materials/SurfaceMaterials/BlinnSurface.hpp"
#include "Components/Materials/SurfaceMaterials/TexturedBlinnSurface.hpp"
#include "Components/Materials/SurfaceMaterials/SkyboxSurface.hpp"
#include "Components/Materials/SurfaceMaterials/CubemapReflectionSurface.hpp"
#include "Components/Materials/SurfaceMaterials/EyeSpaceReflectionSurface.hpp"
#include "Components/Materials/SurfaceMaterials/BlinnReflectionSurface.hpp"
