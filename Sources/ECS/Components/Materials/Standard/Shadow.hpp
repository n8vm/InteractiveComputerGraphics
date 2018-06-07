#pragma once
#include "vkdk.hpp"
#include "Components/Materials/Material.hpp"
#include "Components/Math/Perspective.hpp"
#include "Components/Math/Transform.hpp"
#include "Components/Meshes/Mesh.hpp"
#include <glm/glm.hpp>
#include <memory>
#include <array>

namespace Components::Materials::Standard {
	using namespace Components::Math;
	using namespace Components::Meshes;

	class Shadow: public MaterialInterface {
	public:
		static std::shared_ptr<Shadow> Create(std::string name, PipelineKey pipelineKey) {
			std::cout << "ComponentManager: Adding Shadow \"" << name << "\"" << std::endl;

			auto material = std::make_shared<Material>();
			auto shadow = std::make_shared<Shadow>(pipelineKey);
			material->material = shadow;
			Systems::ComponentManager::Materials[name] = material;
			return shadow;
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

		static void RefreshPipeline() {
			MaterialInterface::DestroyPipeline(getStaticProperties());
			setupGraphicsPipeline();
		}
		
		static VkDescriptorSet CreateDescriptorSet(VkBuffer materialUBO, VkBuffer perspectiveUBO, VkBuffer transformUBO, 
			VkImageView imageView, VkSampler sampler) 
		{
			VkDescriptorSet descriptorSet;

			VkDescriptorSetLayout layouts[] = { getStaticProperties().descriptorSetLayout };
			VkDescriptorSetAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = getStaticProperties().descriptorPool;
			allocInfo.descriptorSetCount = 1;
			allocInfo.pSetLayouts = layouts;

			auto error = vkAllocateDescriptorSets(VKDK::device, &allocInfo, &descriptorSet);
			if (error != VK_SUCCESS) {
				std::cout << "failed to allocate descriptor set! " + error << std::endl;
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

			/* Image sampler */
			VkDescriptorImageInfo imageInfo = {};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
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
		
		Shadow(PipelineKey pipelineKey) {
			this->pipelineKey = pipelineKey;
			createUniformBuffer(sizeof(MaterialBufferObject));
		}

		void uploadUBO() {
			/* Update uniform buffer */
			MaterialBufferObject mbo = {};
			mbo.pointSize = pointSize;
			mbo.color = color;
			mbo.useTexture = useTextureComponent;

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
			
			if (getStaticProperties().descriptorSets.find(key) == getStaticProperties().descriptorSets.end()) {
				auto missingTexture = Systems::ComponentManager::Textures["DefaultTexture"]; 
				VkSampler sampler = missingTexture->texture->getColorSampler();
				VkImageView imageView = missingTexture->texture->getColorImageView();

				if (textureComponent && useTextureComponent) {
					sampler = textureComponent->texture->getColorSampler();
					imageView = textureComponent->texture->getColorImageView();
				}

				getStaticProperties().descriptorSets[key] = CreateDescriptorSet(materialUBO, uboSet.perspectiveUBO, 
					uboSet.transformUBO, imageView, sampler);
				return getStaticProperties().descriptorSets[key];
			}
			else return getStaticProperties().descriptorSets[key];
		}

		void render(PipelineKey pipelineKey, VkCommandBuffer commandBuffer, VkDescriptorSet descriptorSet, std::shared_ptr<Mesh> meshComponent) 
		{
			/* Look up the pipeline cooresponding to this render pass */
			VkPipeline pipeline = getStaticProperties().pipelines[pipelineKey];

			/* Bind the pipeline for this material */
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

			/* Get mesh data */
			VkBuffer vertexBuffer = meshComponent->mesh->getVertexBuffer();
			VkBuffer texCoordBuffer = meshComponent->mesh->getTexCoordBuffer();
			VkBuffer indexBuffer = meshComponent->mesh->getIndexBuffer();
			uint32_t totalIndices = meshComponent->mesh->getTotalIndices();

			VkBuffer vertexBuffers[] = { vertexBuffer, texCoordBuffer };
			VkDeviceSize offsets[] = { 0, 0 };

			VkIndexType indexType = meshComponent->mesh->getIndexBytes() == sizeof(uint32_t) ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_UINT16;

			vkCmdBindVertexBuffers(commandBuffer, 0, 2, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, indexType);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, getStaticProperties().pipelineLayout,
				0, 1, &descriptorSet, 0, nullptr);

			/* Draw elements indexed */
			vkCmdDrawIndexed(commandBuffer, totalIndices, 1, 0, 0, 0);
		}

		void setColor(glm::vec4 color) {
			this->color = color;
		}

    void useTexture(bool useTexture) {
      this->useTextureComponent = useTexture;
    }

		void setTexture(std::shared_ptr<Components::Textures::Texture> texture) {
			this->textureComponent = texture;
		}

	private:
		struct MaterialBufferObject {
			glm::vec4 color;
			float pointSize;
			uint32_t useTexture;
		};

		/* A descriptor set layout describes how uniforms are used in the pipeline */
		static void createDescriptorSetLayout() {
			VkDescriptorSetLayoutBinding pboLayoutBinding = {};
			pboLayoutBinding.binding = 0;
			pboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			pboLayoutBinding.descriptorCount = 1;
			pboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			pboLayoutBinding.pImmutableSamplers = nullptr; // Optional

			VkDescriptorSetLayoutBinding tboLayoutBinding = {};
			tboLayoutBinding.binding = 1;
			tboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			tboLayoutBinding.descriptorCount = 1;
			tboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			tboLayoutBinding.pImmutableSamplers = nullptr; // Optional

			VkDescriptorSetLayoutBinding mboLayoutBinding = {};
			mboLayoutBinding.binding = 2;
			mboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			mboLayoutBinding.descriptorCount = 1;
			mboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			mboLayoutBinding.pImmutableSamplers = nullptr; // Optional

			/* 2D Texture Sampler object */
			VkDescriptorSetLayoutBinding textureLayoutBinding = {};
			textureLayoutBinding.binding = 3;
			textureLayoutBinding.descriptorCount = 1;
			textureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			textureLayoutBinding.pImmutableSamplers = nullptr;
			textureLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

			std::array<VkDescriptorSetLayoutBinding, 4> bindings = { pboLayoutBinding, tboLayoutBinding, mboLayoutBinding, textureLayoutBinding };
			VkDescriptorSetLayoutCreateInfo layoutInfo = {};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
			layoutInfo.pBindings = bindings.data();

			if (vkCreateDescriptorSetLayout(VKDK::device, &layoutInfo, nullptr, &getStaticProperties().descriptorSetLayout) != VK_SUCCESS) {
				throw std::runtime_error("failed to create ShadowedPoints descriptor set layout!");
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
				throw std::runtime_error("failed to create Uniform Colored Points descriptor pool!");
			}
		}

		static void setupGraphicsPipeline() {
			/* Read in shader modules */
			auto vertShaderCode = readFile(ResourcePath "MaterialShaders/Standard/Shadow/vert.spv");
			auto fragShaderCode = readFile(ResourcePath "MaterialShaders/Standard/Shadow/frag.spv");

			/* Create shader modules */
			auto vertShaderModule = createShaderModule(vertShaderCode);
			auto fragShaderModule = createShaderModule(fragShaderCode);

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

			std::vector<VkPipelineShaderStageCreateInfo> shaderStages = { vertShaderStageInfo, fragShaderStageInfo };

			createPipelines(shaderStages, getBindingDescriptions(), 
				getAttributeDescriptions(), { getStaticProperties().descriptorSetLayout },
				getStaticProperties().pipelines, getStaticProperties().pipelineLayout);

			vkDestroyShaderModule(VKDK::device, fragShaderModule, nullptr);
			vkDestroyShaderModule(VKDK::device, vertShaderModule, nullptr);
		}

		static std::vector<VkVertexInputBindingDescription> getBindingDescriptions() {

			VkVertexInputBindingDescription vertexBindingDescription = {};
			vertexBindingDescription.binding = 0;
			vertexBindingDescription.stride = 3 * sizeof(float);
			vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			VkVertexInputBindingDescription texcoordBindingDescription = {};
			texcoordBindingDescription.binding = 1;
			texcoordBindingDescription.stride = 2 * sizeof(float);
			texcoordBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return { vertexBindingDescription, texcoordBindingDescription };
		}

		static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
			/* The second structure is a VKVertexInputAttributeDescription */
			std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = 0;

			attributeDescriptions[1].binding = 1;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[1].offset = 0;

			return attributeDescriptions;
		}

		/* Instanced material properties */
		glm::vec4 color = glm::vec4(1.0, 0.0, 1.0, 1.0);
		std::shared_ptr<Components::Textures::Texture> textureComponent;
		float pointSize = 4.0;
		bool useTextureComponent = false;
	};
}