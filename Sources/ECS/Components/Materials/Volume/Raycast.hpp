#pragma once
#include "vkdk.hpp"
#include "Components/Materials/Material.hpp"
#include "Components/Math/Perspective.hpp"
#include "Components/Math/Transform.hpp"
#include "Components/Meshes/Mesh.hpp"
#include "Components/Lights/PointLight/PointLight.hpp"
#include "Components/Textures/Texture2D.hpp"
#include <glm/glm.hpp>
#include <memory>
#include <array>

namespace Components::Materials::Volume {
	using namespace Components::Math;
	using namespace Components::Meshes;
	using namespace Components::Textures;

	class Raycast : public MaterialInterface {
	public:
		static std::shared_ptr<Raycast> Create(std::string name,
			PipelineKey pipelineKey,
			std::shared_ptr<Texture> texture)
		{
			auto material = std::make_shared<Material>();
			auto RaycastMat = std::make_shared<Raycast>(pipelineKey);
			RaycastMat->texture3DComponent = texture;
			material->material = RaycastMat;
			Systems::ComponentManager::Materials[name] = material;
			return RaycastMat;
		}

		static void Initialize(int maxDescriptorSets) {
			getStaticProperties().maxDescriptorSets = maxDescriptorSets;
			createDescriptorSetLayout();
			createDescriptorPool();
			setupGraphicsPipeline();
		}

		static MaterialProperties& getStaticProperties() {
			static MaterialProperties properties;
			return properties;
		}

		static void Destroy() {
			MaterialInterface::Destroy(getStaticProperties());
		}

		/* Primarily used to account for render pass changes */
		static void RefreshPipeline() {
			MaterialInterface::DestroyPipeline(getStaticProperties());
			setupGraphicsPipeline();
		}

		static VkDescriptorSet CreateDescriptorSet(VkBuffer materialUBO, VkBuffer perspectiveUBO, VkBuffer transformUBO,
			VkImageView imageView, VkSampler sampler) {
			VkDescriptorSet descriptorSet;

			VkDescriptorSetLayout layouts[] = { getStaticProperties().descriptorSetLayout };
			VkDescriptorSetAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = getStaticProperties().descriptorPool;
			allocInfo.descriptorSetCount = 1;
			allocInfo.pSetLayouts = layouts;

			auto error = vkAllocateDescriptorSets(VKDK::device, &allocInfo, &descriptorSet);
			if (error != VK_SUCCESS) {
				throw std::runtime_error("failed to allocate descriptor set! " + error);
			}

			VkDescriptorBufferInfo perspectiveBufferInfo = {};
			perspectiveBufferInfo.buffer = perspectiveUBO;
			perspectiveBufferInfo.offset = 0;
			perspectiveBufferInfo.range = sizeof(Components::Math::PerspectiveBufferObject);

			VkDescriptorBufferInfo transformBufferInfo = {};
			transformBufferInfo.buffer = transformUBO;
			transformBufferInfo.offset = 0;
			transformBufferInfo.range = sizeof(Components::Math::TransformBufferObject);

			VkDescriptorBufferInfo materialBufferInfo = {};
			materialBufferInfo.buffer = materialUBO;
			materialBufferInfo.offset = 0;
			materialBufferInfo.range = sizeof(MaterialBufferObject);

			/* Tex3D sampler */
			VkDescriptorImageInfo imageInfo = {};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			imageInfo.imageView = imageView;
			imageInfo.sampler = sampler;

			std::array<VkWriteDescriptorSet, 4> descriptorWrites = {};

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = descriptorSet;
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &perspectiveBufferInfo;

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = descriptorSet;
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pBufferInfo = &transformBufferInfo;

			descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[2].dstSet = descriptorSet;
			descriptorWrites[2].dstBinding = 2;
			descriptorWrites[2].dstArrayElement = 0;
			descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[2].descriptorCount = 1;
			descriptorWrites[2].pBufferInfo = &materialBufferInfo;

			descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[3].dstSet = descriptorSet;
			descriptorWrites[3].dstBinding = 3;
			descriptorWrites[3].dstArrayElement = 0;
			descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[3].descriptorCount = 1;
			descriptorWrites[3].pImageInfo = &imageInfo;

			vkUpdateDescriptorSets(VKDK::device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
			return descriptorSet;
		}
		
		/* Note: one material instance per entity! Cleanup before destroying VKDK stuff */
		Raycast(PipelineKey pipelineKey) {
			this->pipelineKey = pipelineKey;
			createUniformBuffer(sizeof(MaterialBufferObject));
		}

		/* TODO: use seperate descriptor set for model view projection, update only once per update tick */
		void uploadUBO() {
			/* Update uniform buffer */
			MaterialBufferObject mbo = {};
			mbo.numSamples = numSamples;
			mbo.lod = lod;

			/* Map uniform buffer, copy data directly, then unmap */
			void* data;
			vkMapMemory(VKDK::device, materialUBOMemory, 0, sizeof(mbo), 0, &data);
			memcpy(data, &mbo, sizeof(mbo));
			vkUnmapMemory(VKDK::device, materialUBOMemory);
		}

		/* Returns a preexisting descriptor set, or creates a new one */
		VkDescriptorSet getDescriptorSet(UBOSet uboSet) {
			size_t key = 0;
			hash_combine(key, uboSet.transformUBO);
			hash_combine(key, uboSet.perspectiveUBO);
			hash_combine(key, uboSet.pointLightUBO);

			if (getStaticProperties().descriptorSets.find(key) == getStaticProperties().descriptorSets.end()) {
				VkSampler sampler;;
				VkImageView imageView;
				auto missingTexture = Systems::ComponentManager::Textures["DefaultTexture3D"];
				sampler = missingTexture->texture->getColorSampler();
				imageView = missingTexture->texture->getColorImageView();
				
				if (texture3DComponent) {
					sampler = texture3DComponent->texture->getColorSampler();
					imageView = texture3DComponent->texture->getColorImageView();
				}

				getStaticProperties().descriptorSets[key] = CreateDescriptorSet(materialUBO, uboSet.perspectiveUBO,
					uboSet.transformUBO, imageView, sampler);
				return getStaticProperties().descriptorSets[key];
			}
			else return getStaticProperties().descriptorSets[key];
		}

		void render(PipelineKey pipelineKey, VkCommandBuffer commandBuffer, VkDescriptorSet descriptorSet, std::shared_ptr<Mesh> meshComponent) {
			/* Look up the pipeline cooresponding to this render pass */
			VkPipeline pipeline = getStaticProperties().pipelines[pipelineKey];

			/* Bind the pipeline for this material */
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

			/* Get mesh data */
			VkBuffer vertexBuffer = meshComponent->mesh->getVertexBuffer();
			VkBuffer normalBuffer = meshComponent->mesh->getNormalBuffer();
			VkBuffer texcoordBuffer = meshComponent->mesh->getTexCoordBuffer();
			VkBuffer indexBuffer = meshComponent->mesh->getIndexBuffer();
			uint32_t totalIndices = meshComponent->mesh->getTotalIndices();

			VkBuffer vertexBuffers[] = { vertexBuffer, normalBuffer, texcoordBuffer };
			VkDeviceSize offsets[] = { 0 , 0, 0 };

			VkIndexType indexType = meshComponent->mesh->getIndexBytes() == sizeof(uint32_t) ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;

			vkCmdBindVertexBuffers(commandBuffer, 0, 3, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, indexType);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, getStaticProperties().pipelineLayout,
				0, 1, &descriptorSet, 0, nullptr);

			/* Draw elements indexed */
			vkCmdDrawIndexed(commandBuffer, totalIndices, 1, 0, 0, 0);
		}

		void setNumSamples(int newNumSamples) {
			numSamples = newNumSamples;
		}

	private:
		struct MaterialBufferObject {
			int numSamples;
			float lod;
		};

		/* A descriptor set layout describes how uniforms are used in the pipeline */
		static void createDescriptorSetLayout() {
			VkDescriptorSetLayoutBinding perspectiveLayoutBinding = {};
			perspectiveLayoutBinding.binding = 0;
			perspectiveLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			perspectiveLayoutBinding.descriptorCount = 1;
			perspectiveLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			perspectiveLayoutBinding.pImmutableSamplers = nullptr; // Optional

			VkDescriptorSetLayoutBinding transformLayoutBinding = {};
			transformLayoutBinding.binding = 1;
			transformLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			transformLayoutBinding.descriptorCount = 1;
			transformLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			transformLayoutBinding.pImmutableSamplers = nullptr; // Optional

			VkDescriptorSetLayoutBinding materialLayoutBinding = {};
			materialLayoutBinding.binding = 2;
			materialLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			materialLayoutBinding.descriptorCount = 1;
			materialLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			materialLayoutBinding.pImmutableSamplers = nullptr; // Optional

			VkDescriptorSetLayoutBinding textureLayoutBinding = {};
			textureLayoutBinding.binding = 3;
			textureLayoutBinding.descriptorCount = 1;
			textureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			textureLayoutBinding.pImmutableSamplers = nullptr;
			textureLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

			std::array<VkDescriptorSetLayoutBinding,4> bindings = { 
				perspectiveLayoutBinding, transformLayoutBinding, materialLayoutBinding,
				textureLayoutBinding};

			VkDescriptorSetLayoutCreateInfo layoutInfo = {};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
			layoutInfo.pBindings = bindings.data();

			if (vkCreateDescriptorSetLayout(VKDK::device, &layoutInfo, nullptr, &getStaticProperties().descriptorSetLayout) != VK_SUCCESS) {
				throw std::runtime_error("failed to create Raycast descriptor set layout!");
			}
		}

		static void createDescriptorPool() {
			std::array<VkDescriptorPoolSize, 4> poolSizes = {};
			poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			poolSizes[0].descriptorCount = getStaticProperties().maxDescriptorSets;
			poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			poolSizes[1].descriptorCount = getStaticProperties().maxDescriptorSets;
			poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			poolSizes[2].descriptorCount = getStaticProperties().maxDescriptorSets;
			poolSizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			poolSizes[3].descriptorCount = getStaticProperties().maxDescriptorSets;

			VkDescriptorPoolCreateInfo poolInfo = {};
			poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
			poolInfo.pPoolSizes = poolSizes.data();
			poolInfo.maxSets = getStaticProperties().maxDescriptorSets;
			poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

			if (vkCreateDescriptorPool(VKDK::device, &poolInfo, nullptr, &getStaticProperties().descriptorPool) != VK_SUCCESS) {
				throw std::runtime_error("failed to create Raycast descriptor pool!");
			}
		}

		static void setupGraphicsPipeline() {
			/* Read in shader modules */
			auto vertShaderCode = readFile(ResourcePath "MaterialShaders/Volume/Raycast/vert.spv");
			auto fragShaderCode = readFile(ResourcePath "MaterialShaders/Volume/Raycast/frag.spv");

			/* Create shader modules */
			auto vertShaderModule = createShaderModule(vertShaderCode);
			auto fragShaderModule = createShaderModule(fragShaderCode);

			/* Info for shader stages */
			VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
			vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			vertShaderStageInfo.module = vertShaderModule;
			vertShaderStageInfo.pName = "main";

			VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
			fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			fragShaderStageInfo.module = fragShaderModule;
			fragShaderStageInfo.pName = "main";

			std::vector<VkPipelineShaderStageCreateInfo> shaderStages = { vertShaderStageInfo, fragShaderStageInfo };
			std::vector<VkPipeline> newPipelines;
			std::vector<VkPipelineLayout> newLayouts;

			createPipelines(shaderStages, getBindingDescriptions(),
				getAttributeDescriptions(), { getStaticProperties().descriptorSetLayout },
				getStaticProperties().pipelines, getStaticProperties().pipelineLayout);

			vkDestroyShaderModule(VKDK::device, fragShaderModule, nullptr);
			vkDestroyShaderModule(VKDK::device, vertShaderModule, nullptr);
		}

		static std::vector<VkVertexInputBindingDescription> getBindingDescriptions() {
			std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);

			VkVertexInputBindingDescription vertexBindingDescription = {};
			vertexBindingDescription.binding = 0;
			vertexBindingDescription.stride = 3 * sizeof(float);
			vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			bindingDescriptions[0] = vertexBindingDescription;
			
			return bindingDescriptions;
		}

		static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
			std::vector<VkVertexInputAttributeDescription> attributeDescriptions(1);
			/* Vertex input */
			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = 0;

			return attributeDescriptions;
		}

		public:
		/* Instanced material properties */
		int numSamples = 256;
		float lod;
		std::shared_ptr<Texture> texture3DComponent;
	};
}