#pragma once
#include "vkdk.hpp"

#include "Components/Component.hpp"
#include "Components/Meshes/Mesh.hpp"
#include "Components/Math/Perspective.hpp"

#include "PipelineParameters.hpp"
#include "MaterialProperties.hpp"
#include "Tools/HashCombiner.hpp"
#include "Tools/FileReader.hpp"
#include "Systems/ComponentManager.hpp"

#include <unordered_set>
namespace Components::Materials {
	struct UBOSet {
		VkBuffer transformUBO;
		VkBuffer perspectiveUBO;
		VkBuffer pointLightUBO;
	};

	class MaterialInterface {
	public:
		/* A material can be rendered for a particular command buffer/renderpass/descriptorset/mesh */
		virtual void render(PipelineKey pipelineKey, VkCommandBuffer commandBuffer,
			VkDescriptorSet descriptorSet, std::shared_ptr<Components::Meshes::Mesh> meshComponent) {};

		/* Returns either a preexisting descriptor set, or a new one if one doesn't exist */
		virtual VkDescriptorSet getDescriptorSet(UBOSet uboSet) { return VK_NULL_HANDLE; };

		/* Leave it up to inheriting materials to upload UBO data */
		virtual void uploadUBO() {};

		/* Returns a handle to a material buffer object */
		VkBuffer getUBO() {
			return materialUBO;
		}

		PipelineKey getPipelineKey() {
			return pipelineKey;
		}

		/* Destroys UBO resources */
		void cleanup() {
			vkDestroyBuffer(VKDK::device, materialUBO, nullptr);
			vkFreeMemory(VKDK::device, materialUBOMemory, nullptr);
		}

	protected:

		/* Wrapper for shader module creation */
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

		/* Under the hood, all material types have a set of Vulkan pipeline objects. */
		static void createPipelines(
			std::vector<VkPipelineShaderStageCreateInfo> shaderStages,
			std::vector<VkVertexInputBindingDescription> bindingDescriptions,
			std::vector<VkVertexInputAttributeDescription> attributeDescriptions,
			std::vector<VkDescriptorSetLayout> descriptorSetLayouts,
			std::unordered_map<PipelineKey, VkPipeline> &pipelines,
			VkPipelineLayout &layout
		) {
			std::vector<VkPipeline> pipelineVector;
			pipelineVector.resize(Systems::ComponentManager::PipelineSettings.size());
			std::vector<VkGraphicsPipelineCreateInfo> pipelineInfos(Systems::ComponentManager::PipelineSettings.size());

			/* Vertex Input */
			VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
			vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputInfo.vertexBindingDescriptionCount = (uint32_t)bindingDescriptions.size();
			vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
			vertexInputInfo.vertexAttributeDescriptionCount = (uint32_t)attributeDescriptions.size();
			vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

			/* Connect things together with pipeline layout */
			VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
			pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			pipelineLayoutInfo.setLayoutCount = (uint32_t)descriptorSetLayouts.size();
			pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
			pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
			pipelineLayoutInfo.pPushConstantRanges = 0; // Optional

			if (vkCreatePipelineLayout(VKDK::device, &pipelineLayoutInfo, nullptr, &layout) != VK_SUCCESS) {
				throw std::runtime_error("failed to create pipeline layout!");
			}

			/* Any of the following parameters may change depending on pipeline parameters. */
			int i = 0;
			for (auto pair : Systems::ComponentManager::PipelineSettings) {
				pipelineInfos[i].sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
				pipelineInfos[i].stageCount = (uint32_t)shaderStages.size();
				pipelineInfos[i].pStages = shaderStages.data();
				pipelineInfos[i].pVertexInputState = &vertexInputInfo;
				pipelineInfos[i].pInputAssemblyState = &pair.second->inputAssembly;
				pipelineInfos[i].pViewportState = &pair.second->viewportState;
				pipelineInfos[i].pRasterizationState = &pair.second->rasterizer;
				pipelineInfos[i].pMultisampleState = &pair.second->multisampling;
				pipelineInfos[i].pDepthStencilState = &pair.second->depthStencil;
				pipelineInfos[i].pColorBlendState = &pair.second->colorBlending;
				pipelineInfos[i].pDynamicState = &pair.second->dynamicState; // Optional
				pipelineInfos[i].layout = layout;
				pipelineInfos[i].renderPass = pair.first.renderpass;
				pipelineInfos[i].subpass = pair.first.subpass;
				pipelineInfos[i].basePipelineHandle = VK_NULL_HANDLE; // Optional
				pipelineInfos[i].basePipelineIndex = -1; // Optional
				i++;
			}

			if (vkCreateGraphicsPipelines(VKDK::device, VK_NULL_HANDLE, (uint32_t)pipelineInfos.size(), 
				pipelineInfos.data(), nullptr, pipelineVector.data()) != VK_SUCCESS) {
				throw std::runtime_error("failed to create graphics pipeline!");
			}

			/* Create unordered map of pipeline keys to pipelines */
			i = 0;
			for (auto pair : Systems::ComponentManager::PipelineSettings) {
				pipelines[pair.first] = pipelineVector[i];
				i++;
			}
		}

		/* Clean up descriptor pool, descriptor layout, pipeline layout, and pipeline */
		static void Destroy(MaterialProperties &properties) {
			vkDestroyDescriptorPool(VKDK::device, properties.descriptorPool, nullptr);
			vkDestroyDescriptorSetLayout(VKDK::device, properties.descriptorSetLayout, nullptr);
			vkDestroyPipelineLayout(VKDK::device, properties.pipelineLayout, nullptr);
			for (auto pipeline : properties.pipelines)
				vkDestroyPipeline(VKDK::device, pipeline.second, nullptr);
		}

		static void DestroyPipeline(MaterialProperties &properties) {
			vkDestroyPipelineLayout(VKDK::device, properties.pipelineLayout, nullptr);
			for (auto pipeline : properties.pipelines)
				vkDestroyPipeline(VKDK::device, pipeline.second, nullptr);
		}

		/* All material instances have these */
		VkBuffer materialUBO;
		VkDeviceMemory materialUBOMemory;
		VkDescriptorSet descriptorSet;

		/* This material will only render on the provided pipeline key */
		PipelineKey pipelineKey;

		void createUniformBuffer(VkDeviceSize bufferSize) {
			VKDK::CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				materialUBO, materialUBOMemory);
		}
	};

	class Material : public Component {
	public:
		/*static std::shared_ptr<Material> Create(std::string name) {
			auto set = std::make_shared<Material>();
			Systems::ComponentManager::Materials[name] = set;
			return set;
		}
		static std::shared_ptr<Material> Create(std::string name, std::vector<std::shared_ptr<Components::Materials::MaterialInterface>> materials) {
			auto set = std::make_shared<Material>();
			set->materials = materials;
			Systems::ComponentManager::Materials[name] = set;
			return set;
		}
		static std::shared_ptr<Material> Create(std::string name, std::shared_ptr<Components::Materials::Material> material) {
			auto set = std::make_shared<Material>();
			set->materials.push_back(material);
			Systems::ComponentManager::Materials[name] = set;
			return set;
		}*/

		/*Material() {};

		Material(std::vector<std::shared_ptr<Components::Materials::Material>> materials) {
			this->materials = materials;
		};

		Material(std::shared_ptr<Components::Materials::MaterialInterface> material) {
			this->materials.push_back(material);
		}
*/
		std::shared_ptr<Components::Materials::MaterialInterface> material;

		//void add(std::shared_ptr<Components::Materials::MaterialInterface> material) {
		//	materials.push_back(material);
		//}

		void cleanup() {
			material->cleanup();
		}
	};
};