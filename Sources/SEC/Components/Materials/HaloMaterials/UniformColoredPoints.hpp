#pragma once
#include "Components/Materials/Material.hpp"
#include "vkdk.hpp"
#include <memory>
#include <glm/glm.hpp>
#include "System/System.hpp"
#include <array>


namespace Components::Materials::HaloMaterials {
	class UniformColoredPoints : public Material {
	public:
		static void Initialize(int maxDescriptorSets) {
			UniformColoredPoints::maxDescriptorSets = maxDescriptorSets;
			createDescriptorSetLayout();
			createDescriptorPool();
			setupGraphicsPipeline();
		}

		static void Destroy() {
			/* Clean up descriptor pool, descriptor layout, pipeline layout, and pipeline */
			vkDestroyDescriptorPool(VKDK::device, descriptorPool, nullptr);
			vkDestroyDescriptorSetLayout(VKDK::device, descriptorSetLayout, nullptr);
			vkDestroyPipelineLayout(VKDK::device, pipelineLayout, nullptr);
			vkDestroyPipeline(VKDK::device, graphicsPipeline, nullptr);
		}

		/* Primarily used to account for render pass changes */
		static void RefreshPipeline() {
			vkDestroyPipelineLayout(VKDK::device, pipelineLayout, nullptr);
			vkDestroyPipeline(VKDK::device, graphicsPipeline, nullptr);
			setupGraphicsPipeline();
		}
		
		/* Note: one material instance per entity! Cleanup before destroying VKDK stuff */
		UniformColoredPoints() : Material() {
			/* Should I create a descriptor set here? If so, how many do I create?
			
				If only one object will be using this material, then I could create one descriptor set. 
				However, if multiple objects intend to use the same material, then I might need multiple descriptor sets...

				Thinking, maybe materials should be static, since the "state" of the material depends heavily on descriptor set...
				
				So, render would become a static method, which uses an initialized graphics pipeline and provided VBO, IBO, Descriptor data

				How does that effect the concept of "adding" a material to an entity? 

				Would be nice to wrap descriptor set info for different materials

				Maybe instances of this material could coorespond to descriptor sets, and static stuff creates descriptor pools, graphics pipeline, etc
			*/

			createUniformBuffer();
			createDescriptorSet();
		}

		void cleanup() {
			/* Frees descriptor set and uniform buffer memory, destroys uniform buffer handle */
			vkFreeDescriptorSets(VKDK::device, VKDK::descriptorPool, 1, &descriptorSet);
			vkDestroyBuffer(VKDK::device, uniformBuffer, nullptr);
			vkFreeMemory(VKDK::device, uniformBufferMemory, nullptr);
		}
		
		/* TODO: use seperate descriptor set for model view projection, update only once per update tick */
		void update(glm::mat4 model, glm::mat4 view, glm::mat4 projection) {
			/* Update uniform buffer */
			UniformBufferObject ubo = {};
			ubo.model = model;
			ubo.view = view;
			ubo.proj = projection;
			ubo.proj[1][1] *= -1; // required so that image doesn't flip upside down.
			ubo.pointSize = pointSize;
			ubo.color = color;

			/* Map uniform buffer, copy data directly, then unmap */
			void* data;
			vkMapMemory(VKDK::device, uniformBufferMemory, 0, sizeof(ubo), 0, &data);
			memcpy(data, &ubo, sizeof(ubo));
			vkUnmapMemory(VKDK::device, uniformBufferMemory);
		}

		void render(std::shared_ptr<Mesh> mesh) {
			/* Bind the pipeline for this material */
			vkCmdBindPipeline(VKDK::commandBuffers[System::currentImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

			/* Get mesh data */
			VkBuffer vertexBuffer = mesh->getVertexBuffer();
			VkBuffer indexBuffer = mesh->getIndexBuffer();
			uint32_t totalIndices = mesh->getTotalIndices();

			VkBuffer vertexBuffers[] = { vertexBuffer };
			VkDeviceSize offsets[] = { 0 };

			vkCmdBindVertexBuffers(VKDK::commandBuffers[System::currentImageIndex], 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(VKDK::commandBuffers[System::currentImageIndex], indexBuffer, 0, VK_INDEX_TYPE_UINT16);
			vkCmdBindDescriptorSets(VKDK::commandBuffers[System::currentImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

			/* Draw elements indexed */
			vkCmdDrawIndexed(VKDK::commandBuffers[System::currentImageIndex], totalIndices, 1, 0, 0, 0);
		}

		void setColor(float r, float g, float b, float a) {
			color = glm::vec4(r, g, b, a);
		}

		void setPointSize(float size) {
			pointSize = size;
		}

	private:
		/* Static material properties */
		static VkShaderModule vertShaderModule;
		static VkShaderModule fragShaderModule;

		static VkPipelineLayout pipelineLayout;
		static VkPipeline graphicsPipeline;

		static VkDescriptorPool descriptorPool;
		static VkDescriptorSetLayout descriptorSetLayout;
		static uint32_t maxDescriptorSets;

		struct UniformBufferObject {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
			glm::vec4 color;
			float pointSize;
		};

		/* A descriptor set layout describes how uniforms are used in the pipeline */
		static void createDescriptorSetLayout() {
			VkDescriptorSetLayoutBinding uboLayoutBinding = {};
			uboLayoutBinding.binding = 0;
			uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uboLayoutBinding.descriptorCount = 1;
			uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

			std::array<VkDescriptorSetLayoutBinding, 1> bindings = { uboLayoutBinding };
			VkDescriptorSetLayoutCreateInfo layoutInfo = {};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
			layoutInfo.pBindings = bindings.data();

			if (vkCreateDescriptorSetLayout(VKDK::device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
				throw std::runtime_error("failed to create UniformColoredPoints descriptor set layout!");
			}
		}

		static void createDescriptorPool() {
			std::array<VkDescriptorPoolSize, 1> poolSizes = {};
			poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			poolSizes[0].descriptorCount = maxDescriptorSets;

			VkDescriptorPoolCreateInfo poolInfo = {};
			poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
			poolInfo.pPoolSizes = poolSizes.data();
			poolInfo.maxSets = maxDescriptorSets;
			poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

			if (vkCreateDescriptorPool(VKDK::device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
				throw std::runtime_error("failed to create Uniform Colored Points descriptor pool!");
			}
		}

		static void setupGraphicsPipeline() {
			/* Read in shader modules */
			auto vertShaderCode = readFile(ResourcePath "MaterialShaders/UniformColoredPoints/vert.spv");
			auto fragShaderCode = readFile(ResourcePath "MaterialShaders/UniformColoredPoints/frag.spv");

			/* Create shader modules */
			vertShaderModule = createShaderModule(vertShaderCode);
			fragShaderModule = createShaderModule(fragShaderCode);

			/* Info for shader stages */
			VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
			vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			vertShaderStageInfo.module = vertShaderModule;
			vertShaderStageInfo.pName = "main"; // entry point here? would be nice to combine shaders into one file

			VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
			fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			fragShaderStageInfo.module = fragShaderModule;
			fragShaderStageInfo.pName = "main";

			VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

			/* Info for fixed function stuff */

			/* Vertex Input */
			VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
			vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			//vertexInputInfo.vertexBindingDescriptionCount = 0;
			//vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
			//vertexInputInfo.vertexAttributeDescriptionCount = 0;
			//vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

			/* ADDED AFTER VB CHAPTER */
			auto bindingDescription = getBindingDescription();
			auto attributeDescriptions = getAttributeDescriptions();

			vertexInputInfo.vertexBindingDescriptionCount = 1;
			vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
			vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
			vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

			/* Input Assembly */
			VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
			inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
			inputAssembly.primitiveRestartEnable = VK_FALSE;

			/* Viewports and Scissors */
			VkViewport viewport = {};
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.width = (float)VKDK::swapChainExtent.width;
			viewport.height = (float)VKDK::swapChainExtent.height;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;

			VkRect2D scissor = {};
			scissor.offset = { 0, 0 };
			scissor.extent = VKDK::swapChainExtent;

			VkPipelineViewportStateCreateInfo viewportState = {};
			viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportState.viewportCount = 1;
			viewportState.pViewports = &viewport;
			viewportState.scissorCount = 1;
			viewportState.pScissors = &scissor;

			/* Rasterizer */
			VkPipelineRasterizationStateCreateInfo rasterizer = {};
			rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterizer.depthClampEnable = VK_FALSE; // Helpful for shadow maps.
			rasterizer.rasterizerDiscardEnable = VK_FALSE; // if true, geometry never goes through rasterization, or to frame buffer
			rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // could be line or point too. those require GPU features to be enabled.
			rasterizer.lineWidth = 1.0f; // anything thicker than 1.0 requires wideLines GPU feature
			rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
			rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
			rasterizer.depthBiasEnable = VK_FALSE;
			rasterizer.depthBiasConstantFactor = 0.0f; // Optional
			rasterizer.depthBiasClamp = 0.0f; // Optional
			rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

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
			colorBlendAttachment.blendEnable = VK_FALSE;
			colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
			colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
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
				VK_DYNAMIC_STATE_LINE_WIDTH
			};

			VkPipelineDynamicStateCreateInfo dynamicState = {};
			dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			dynamicState.dynamicStateCount = 2;
			dynamicState.pDynamicStates = dynamicStates;

			/* Connect things together with pipeline layout */
			VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
			pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutInfo.setLayoutCount = 1;
			pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
			pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
			pipelineLayoutInfo.pPushConstantRanges = 0; // Optional

			if (vkCreatePipelineLayout(VKDK::device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
				throw std::runtime_error("failed to create pipeline layout!");
			}

			/* Now we can create a graphics pipeline! */
			VkGraphicsPipelineCreateInfo pipelineInfo = {};
			pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineInfo.stageCount = 2;
			pipelineInfo.pStages = shaderStages;
			pipelineInfo.pVertexInputState = &vertexInputInfo;
			pipelineInfo.pInputAssemblyState = &inputAssembly;
			pipelineInfo.pViewportState = &viewportState;
			pipelineInfo.pRasterizationState = &rasterizer;
			pipelineInfo.pMultisampleState = &multisampling;
			pipelineInfo.pDepthStencilState = &depthStencil;
			pipelineInfo.pColorBlendState = &colorBlending;
			pipelineInfo.pDynamicState = nullptr; // Optional

			pipelineInfo.layout = pipelineLayout;

			pipelineInfo.renderPass = VKDK::renderPass;
			pipelineInfo.subpass = 0;

			pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
			pipelineInfo.basePipelineIndex = -1; // Optional
			if (vkCreateGraphicsPipelines(VKDK::device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
				throw std::runtime_error("failed to create graphics pipeline!");
			}

			vkDestroyShaderModule(VKDK::device, fragShaderModule, nullptr);
			vkDestroyShaderModule(VKDK::device, vertShaderModule, nullptr);
		}

		static VkShaderModule createShaderModule(const std::vector<char>& code) {
			VkShaderModuleCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.codeSize = code.size();
			createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

			VkShaderModule shaderModule;
			if (vkCreateShaderModule(VKDK::device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
				throw std::runtime_error("failed to create shader module!");
			}

			return shaderModule;
		}

		static VkVertexInputBindingDescription getBindingDescription() {
			/* The first structure is VKVertexInputBindingDescription */
			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = 0;
			bindingDescription.stride = 3 * sizeof(float);

			/* Move to next data entry after each vertex */
			/* Could be move to next data entry after each instance! */
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 1> getAttributeDescriptions() {
			/* The second structure is a VKVertexInputAttributeDescription */
			std::array<VkVertexInputAttributeDescription, 1> attributeDescriptions = {};

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = 0;

			return attributeDescriptions;
		}

		/* Instanced material properties */
		glm::vec4 color = glm::vec4(1.0, 0.0, 0.0, 1.0);
		float pointSize = 4.0;

		VkBuffer uniformBuffer;
		VkDeviceMemory uniformBufferMemory;
		VkDescriptorSet descriptorSet;

		void createUniformBuffer() {
			VkDeviceSize bufferSize = sizeof(UniformBufferObject);
			VKDK::CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffer, uniformBufferMemory);
		}

		void createDescriptorSet() {
			VkDescriptorSetLayout layouts[] = { descriptorSetLayout };
			VkDescriptorSetAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = VKDK::descriptorPool;
			allocInfo.descriptorSetCount = 1;
			allocInfo.pSetLayouts = layouts;

			if (vkAllocateDescriptorSets(VKDK::device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
				throw std::runtime_error("failed to allocate descriptor set!");
			}

			VkDescriptorBufferInfo bufferInfo = {};
			bufferInfo.buffer = uniformBuffer;
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			/*VkDescriptorImageInfo imageInfo = {};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = textureImageView;
			imageInfo.sampler = textureSampler;*/

			std::array<VkWriteDescriptorSet, 1> descriptorWrites = {};

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = descriptorSet;
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &bufferInfo;

			/*descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = descriptorSet;
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pImageInfo = &imageInfo;*/

			vkUpdateDescriptorSets(VKDK::device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}

	};
}