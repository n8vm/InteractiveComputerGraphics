#pragma once
#include "Components/Materials/Material.hpp"
#include "Components/Textures/Texture.hpp"
#include "vkdk.hpp"
#include <memory>
#include <glm/glm.hpp>
#include "System/System.hpp"
#include <array>
#include "Entities/Lights/LightBufferObject.hpp"
#include "Entities/Cameras/CameraBufferObject.hpp"

namespace Components::Materials::SurfaceMaterials {
	class TexturedBlinnSurface : public Material {
	public:
		static void Initialize(int maxDescriptorSets, std::vector<PipelineParameters> pipelineParameters) {
			TexturedBlinnSurface::maxDescriptorSets = maxDescriptorSets;
			createDescriptorSetLayout();
			createDescriptorPool();
			setupGraphicsPipeline(pipelineParameters);
		}

		static void Destroy() {
			for (TexturedBlinnSurface tbs : TexturedBlinnSurfaces) tbs.cleanup();

			/* Clean up descriptor pool, descriptor layout, pipeline layout, and pipeline */
			vkDestroyDescriptorPool(VKDK::device, descriptorPool, nullptr);
			vkDestroyDescriptorSetLayout(VKDK::device, descriptorSetLayout, nullptr);
			for (auto layout : pipelineLayouts)
				vkDestroyPipelineLayout(VKDK::device, layout.second, nullptr);
			for (auto pipeline : pipelines)
				vkDestroyPipeline(VKDK::device, pipeline.second, nullptr);
		}

		/* Primarily used to account for render pass changes */
		static void RefreshPipeline(std::vector<PipelineParameters> pipelineParameters) {
			for (auto layout : pipelineLayouts)
				vkDestroyPipelineLayout(VKDK::device, layout.second, nullptr);
			for (auto pipeline : pipelines)
				vkDestroyPipeline(VKDK::device, pipeline.second, nullptr);
			setupGraphicsPipeline(pipelineParameters);
		}

		/* Note: one material instance per entity! Cleanup before destroying VKDK stuff */
		TexturedBlinnSurface(std::vector<std::shared_ptr<Scene>> scenes, std::shared_ptr<Textures::Texture> diffuseTexture) : Material() {
			this->diffuseTexture = diffuseTexture;
			this->setColor(glm::vec4(1.0, 1.0, 1.0, 1.0), glm::vec4(1.0, 1.0, 1.0, 1.0), glm::vec4(1.0, 1.0, 1.0, 1.0));
			useSpecularTexture = false;
			createUniformBuffers(scenes, sizeof(UniformBufferObject));
			createDescriptorSets(scenes);

			TexturedBlinnSurfaces.push_back(*this);
		}

		TexturedBlinnSurface(std::vector<std::shared_ptr<Scene>> scenes, std::shared_ptr<Textures::Texture> diffuseTexture, std::shared_ptr<Textures::Texture> specularTexture) : Material() {
			this->diffuseTexture = diffuseTexture;
			this->specularTexture = specularTexture;
			this->setColor(glm::vec4(1.0, 1.0, 1.0, 1.0), glm::vec4(1.0, 1.0, 1.0, 1.0), glm::vec4(1.0, 1.0, 1.0, 1.0));
			useSpecularTexture = true;
			createUniformBuffers(scenes, sizeof(UniformBufferObject));
			createDescriptorSets(scenes);
			TexturedBlinnSurfaces.push_back(*this);
		}
		TexturedBlinnSurface(std::shared_ptr<Scene> scene, std::shared_ptr<Textures::Texture> diffuseTexture) : Material() {
			std::vector<std::shared_ptr<Scene>> scenes = { scene };
			this->diffuseTexture = diffuseTexture;
			this->setColor(glm::vec4(1.0, 1.0, 1.0, 1.0), glm::vec4(1.0, 1.0, 1.0, 1.0), glm::vec4(1.0, 1.0, 1.0, 1.0));
			useSpecularTexture = false;
			createUniformBuffers(scenes, sizeof(UniformBufferObject));
			createDescriptorSets(scenes);

			TexturedBlinnSurfaces.push_back(*this);
		}

		TexturedBlinnSurface(std::shared_ptr<Scene> scene, std::shared_ptr<Textures::Texture> diffuseTexture, std::shared_ptr<Textures::Texture> specularTexture) : Material() {
			std::vector<std::shared_ptr<Scene>> scenes = { scene };
			this->diffuseTexture = diffuseTexture;
			this->specularTexture = specularTexture;
			this->setColor(glm::vec4(1.0, 1.0, 1.0, 1.0), glm::vec4(1.0, 1.0, 1.0, 1.0), glm::vec4(1.0, 1.0, 1.0, 1.0));
			useSpecularTexture = true;
			createUniformBuffers(scenes, sizeof(UniformBufferObject));
			createDescriptorSets(scenes);
			TexturedBlinnSurfaces.push_back(*this);
		}
		void cleanup() {
			Material::cleanup(descriptorPool);
		}

		/* TODO: use seperate descriptor set for model view projection, update only once per update tick */
		void update(Scene *scene, glm::mat4 model, glm::mat4 view, glm::mat4 projection) {
			/* Update uniform buffer */
			UniformBufferObject ubo = {};
			ubo.modelMatrix = model;
			ubo.normalMatrix = transpose(inverse(view * model));
			ubo.ka = ka;
			ubo.kd = kd;
			ubo.ks = ks;
			ubo.useSpecular = useSpecularTexture;

			/* Map uniform buffer, copy data directly, then unmap */
			void* data;
			vkMapMemory(VKDK::device, uniformBufferMemories[scene->getRenderPass()], 0, sizeof(ubo), 0, &data);
			memcpy(data, &ubo, sizeof(ubo));
			vkUnmapMemory(VKDK::device, uniformBufferMemories[scene->getRenderPass()]);
		}

		void render(Scene *scene, std::shared_ptr<Components::Meshes::Mesh> mesh) {
			if (!mesh) {
				std::cout << "TexturedBlinnSurface: mesh is empty, not rendering!" << std::endl;
				return;
			}

			/* Look up the pipeline cooresponding to this render pass */
			VkPipeline pipeline = pipelines[scene->getRenderPass()];
			VkPipelineLayout layout = pipelineLayouts[scene->getRenderPass()];

			/* Bind the pipeline for this material */
			vkCmdBindPipeline(scene->getCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

			/* Get mesh data */
			VkBuffer vertexBuffer = mesh->getVertexBuffer();
			VkBuffer normalBuffer = mesh->getNormalBuffer();
			VkBuffer indexBuffer = mesh->getIndexBuffer();
			VkBuffer texCoordBuffer = mesh->getTexCoordBuffer();
			uint32_t totalIndices = mesh->getTotalIndices();

			VkBuffer vertexBuffers[] = { vertexBuffer, normalBuffer, texCoordBuffer };
			VkDeviceSize offsets[] = { 0 , 0, 0 };

			vkCmdBindVertexBuffers(scene->getCommandBuffer(), 0, 3, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(scene->getCommandBuffer(), indexBuffer, 0, VK_INDEX_TYPE_UINT16);
			vkCmdBindDescriptorSets(scene->getCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, 
				&descriptorSets[scene->getRenderPass()], 0, nullptr);

			/* Draw elements indexed */
			vkCmdDrawIndexed(scene->getCommandBuffer(), totalIndices, 1, 0, 0, 0);
		}

		void setColor(glm::vec4 ka, glm::vec4 kd, glm::vec4 ks) {
			this->ka = ka;
			this->kd = kd;
			this->ks = ks;
		}

	private:
		/* Static material properties */
		static VkShaderModule vertShaderModule;
		static VkShaderModule fragShaderModule;

		static std::unordered_map<VkRenderPass, VkPipelineLayout> pipelineLayouts;
		static std::unordered_map<VkRenderPass, VkPipeline> pipelines;

		static VkDescriptorPool descriptorPool;
		static VkDescriptorSetLayout descriptorSetLayout;
		static uint32_t maxDescriptorSets;

		static std::vector<TexturedBlinnSurface> TexturedBlinnSurface::TexturedBlinnSurfaces;

		struct UniformBufferObject {
			glm::mat4 modelMatrix;
			glm::mat4 normalMatrix;
			glm::vec4 ka;
			glm::vec4 kd;
			glm::vec4 ks;
			bool useSpecular;
		};

		/* A descriptor set layout describes how uniforms are used in the pipeline */
		static void createDescriptorSetLayout() {
			std::array<VkDescriptorSetLayoutBinding, 5> bindings = {};
			
			/* Camera buffer object */
			bindings[0].binding = 0;
			bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			bindings[0].descriptorCount = 1;
			bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT; /* Might need to change this */
			bindings[0].pImmutableSamplers = nullptr; // Optional

			/* Uniform buffer object */
			bindings[1].binding = 1;
			bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			bindings[1].descriptorCount = 1;
			bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			bindings[1].pImmutableSamplers = nullptr; // Optional

			/* Light buffer object */
			bindings[2].binding = 2;
			bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			bindings[2].descriptorCount = 1;
			bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; /* Might need to change this */
			bindings[2].pImmutableSamplers = nullptr; // Optional

			/* 2D Diffuse Texture Sampler object */
			bindings[3].binding = 3;
			bindings[3].descriptorCount = 1;
			bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			bindings[3].pImmutableSamplers = nullptr;
			bindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

			/* 2D Specular Texture Sampler object */
			bindings[4].binding = 4;
			bindings[4].descriptorCount = 1;
			bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			bindings[4].pImmutableSamplers = nullptr;
			bindings[4].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

			VkDescriptorSetLayoutCreateInfo layoutInfo = {};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
			layoutInfo.pBindings = bindings.data();

			if (vkCreateDescriptorSetLayout(VKDK::device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
				throw std::runtime_error("failed to create TexturedBlinnSurface descriptor set layout!");
			}
		}

		static void createDescriptorPool() {
			std::array<VkDescriptorPoolSize, 5> poolSizes = {};
			/* Camera Buffer */
			poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			poolSizes[0].descriptorCount = maxDescriptorSets;
			/* Uniform Buffer */
			poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			poolSizes[1].descriptorCount = maxDescriptorSets;
			/* Light Buffer */
			poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			poolSizes[2].descriptorCount = maxDescriptorSets;
			/* Diffuse Image Sampler */
			poolSizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			poolSizes[3].descriptorCount = maxDescriptorSets;
			/* Specular Image Sampler */
			poolSizes[4].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			poolSizes[4].descriptorCount = maxDescriptorSets;

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

		static void setupGraphicsPipeline(std::vector<PipelineParameters> pipelineParameters) {
			/* Read in shader modules */
			auto vertShaderCode = readFile(ResourcePath "MaterialShaders/SurfaceMaterials/TexturedBlinnSurface/vert.spv");
			auto fragShaderCode = readFile(ResourcePath "MaterialShaders/SurfaceMaterials/TexturedBlinnSurface/frag.spv");

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

			std::vector<VkPipelineShaderStageCreateInfo> shaderStages = { vertShaderStageInfo, fragShaderStageInfo };
			std::vector<VkPipeline> newPipelines;
			std::vector<VkPipelineLayout> newLayouts;

			createPipelines(pipelineParameters, shaderStages, getBindingDescriptions(), getAttributeDescriptions(), { descriptorSetLayout },
				newPipelines, newLayouts);

			vkDestroyShaderModule(VKDK::device, fragShaderModule, nullptr);
			vkDestroyShaderModule(VKDK::device, vertShaderModule, nullptr);

			for (int i = 0; i < pipelineParameters.size(); ++i) {
				pipelines[pipelineParameters[i].renderPass] = newPipelines[i];
				pipelineLayouts[pipelineParameters[i].renderPass] = newLayouts[i];
			}
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

		static std::vector<VkVertexInputBindingDescription> getBindingDescriptions() {
			std::vector<VkVertexInputBindingDescription> bindingDescriptions(3);

			/* The first structure is VKVertexInputBindingDescription */
			VkVertexInputBindingDescription vertexBindingDescription = {};
			vertexBindingDescription.binding = 0;
			vertexBindingDescription.stride = 3 * sizeof(float);
			vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			VkVertexInputBindingDescription normalBindingDescription = {};
			normalBindingDescription.binding = 1;
			normalBindingDescription.stride = 3 * sizeof(float);
			normalBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			VkVertexInputBindingDescription texCoordBindingDescription = {};
			texCoordBindingDescription.binding = 2;
			texCoordBindingDescription.stride = 2 * sizeof(float);
			texCoordBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			bindingDescriptions[0] = vertexBindingDescription;
			bindingDescriptions[1] = normalBindingDescription;
			bindingDescriptions[2] = texCoordBindingDescription;

			/* Move to next data entry after each vertex */
			/* Could be move to next data entry after each instance! */

			return bindingDescriptions;
		}

		static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
			std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);
			/* Vertex input */
			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = 0;

			/* Normal input */
			attributeDescriptions[1].binding = 1;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = 0;

			/* Tex Coord input */
			attributeDescriptions[2].binding = 2;
			attributeDescriptions[2].location = 2;
			attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[2].offset = 0;

			return attributeDescriptions;
		}

		/* Instanced material properties */
		glm::vec4 ka = glm::vec4(0.5, 0.5, 0.5, 1.0);
		glm::vec4 kd = glm::vec4(0.5, 0.5, 0.5, 1.0);
		glm::vec4 ks = glm::vec4(0.5, 0.5, 0.5, 1.0);

		std::shared_ptr<Textures::Texture> diffuseTexture;
		std::shared_ptr<Textures::Texture> specularTexture;
		bool useSpecularTexture = false;

		void createDescriptorSets(std::vector<std::shared_ptr<Scene>> scenes) {
			for (int i = 0; i < scenes.size(); ++i) {

				VkDescriptorSetLayout layouts[] = { descriptorSetLayout };
				VkDescriptorSetAllocateInfo allocInfo = {};
				allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				allocInfo.descriptorPool = descriptorPool;
				allocInfo.descriptorSetCount = 1;
				allocInfo.pSetLayouts = layouts;

				if (vkAllocateDescriptorSets(VKDK::device, &allocInfo, &descriptorSets[scenes[i]->getRenderPass()]) != VK_SUCCESS) {
					throw std::runtime_error("failed to allocate descriptor set!");
				}

				/* Camera buffer */
				VkDescriptorBufferInfo cameraDescriptorBufferInfo = {};
				cameraDescriptorBufferInfo.buffer = scenes[i]->getCameraBuffer();
				cameraDescriptorBufferInfo.offset = 0;
				cameraDescriptorBufferInfo.range = sizeof(CameraBufferObject);

				/* Uniform buffer */
				VkDescriptorBufferInfo uniformDescriptorBufferInfo = {};
				uniformDescriptorBufferInfo.buffer = uniformBuffers[scenes[i]->getRenderPass()];
				uniformDescriptorBufferInfo.offset = 0;
				uniformDescriptorBufferInfo.range = sizeof(UniformBufferObject);

				/* Light buffer */
				VkDescriptorBufferInfo lightDescriptorBufferInfo = {};
				lightDescriptorBufferInfo.buffer = scenes[i]->getLightBuffer();
				lightDescriptorBufferInfo.offset = 0;
				lightDescriptorBufferInfo.range = sizeof(LightBufferObject);

				/* Diffuse Image sampler */
				VkDescriptorImageInfo diffuseImageInfo = {};
				diffuseImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				diffuseImageInfo.imageView = diffuseTexture->getColorImageView();
				diffuseImageInfo.sampler = diffuseTexture->getSampler();

				/* Specular Image sampler */
				VkDescriptorImageInfo specularImageInfo = {};
				specularImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				specularImageInfo.imageView = (useSpecularTexture) ? specularTexture->getColorImageView() : diffuseTexture->getColorImageView();
				specularImageInfo.sampler = (useSpecularTexture) ? specularTexture->getSampler() : diffuseTexture->getSampler();

				std::array<VkWriteDescriptorSet, 5> descriptorWrites = {};

				descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[0].dstSet = descriptorSets[scenes[i]->getRenderPass()];
				descriptorWrites[0].dstBinding = 0;
				descriptorWrites[0].dstArrayElement = 0;
				descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descriptorWrites[0].descriptorCount = 1;
				descriptorWrites[0].pBufferInfo = &cameraDescriptorBufferInfo;

				descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[1].dstSet = descriptorSets[scenes[i]->getRenderPass()];
				descriptorWrites[1].dstBinding = 1;
				descriptorWrites[1].dstArrayElement = 0;
				descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descriptorWrites[1].descriptorCount = 1;
				descriptorWrites[1].pBufferInfo = &uniformDescriptorBufferInfo;

				descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[2].dstSet = descriptorSets[scenes[i]->getRenderPass()];
				descriptorWrites[2].dstBinding = 2;
				descriptorWrites[2].dstArrayElement = 0;
				descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descriptorWrites[2].descriptorCount = 1;
				descriptorWrites[2].pBufferInfo = &lightDescriptorBufferInfo;

				descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[3].dstSet = descriptorSets[scenes[i]->getRenderPass()];
				descriptorWrites[3].dstBinding = 3;
				descriptorWrites[3].dstArrayElement = 0;
				descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				descriptorWrites[3].descriptorCount = 1;
				descriptorWrites[3].pImageInfo = &diffuseImageInfo;

				descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[4].dstSet = descriptorSets[scenes[i]->getRenderPass()];
				descriptorWrites[4].dstBinding = 4;
				descriptorWrites[4].dstArrayElement = 0;
				descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				descriptorWrites[4].descriptorCount = 1;
				descriptorWrites[4].pImageInfo = &specularImageInfo;

				vkUpdateDescriptorSets(VKDK::device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
			}
		}
	};
}