#pragma once
#include "Components/Materials/Material.hpp"
#include "Components/Textures/TextureCube.hpp"
#include "vkdk.hpp"
#include <memory>
#include <glm/glm.hpp>
#include "System/System.hpp"
#include <array>

#include "Entities/Lights/LightBufferObject.hpp"
#include "Entities/Cameras/CameraBufferObject.hpp"

namespace Components::Materials::SurfaceMaterials {
	class CubemapReflectionSurface : public Material {
	public:
		static void Initialize(int maxDescriptorSets, std::vector<PipelineParameters> pipelineParameters) {
			CubemapReflectionSurface::maxDescriptorSets = maxDescriptorSets;
			createDescriptorSetLayout();
			createDescriptorPool();
			setupGraphicsPipeline(pipelineParameters);
		}

		static void Destroy() {
			for (CubemapReflectionSurface ns : CubemapReflectionSurfaces) ns.cleanup();

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
		CubemapReflectionSurface(std::shared_ptr<Scene> scene, std::shared_ptr<Components::Textures::Texture> cubemap) : Material() {
			std::vector<std::shared_ptr<Scene>> scenes = { scene };
			this->cubemap = cubemap;
			createUniformBuffers(scenes, sizeof(UniformBufferObject));
			createDescriptorSets(scenes);

			CubemapReflectionSurfaces.push_back(*this);
		}
		CubemapReflectionSurface(std::vector<std::shared_ptr<Scene>> scenes, std::shared_ptr<Components::Textures::Texture> cubemap) : Material() {
			this->cubemap = cubemap;
			createUniformBuffers(scenes, sizeof(UniformBufferObject));
			createDescriptorSets(scenes);

			CubemapReflectionSurfaces.push_back(*this);
		}

		void cleanup() {
			Material::cleanup(descriptorPool);
		}

		/* TODO: use seperate descriptor set for model view projection, update only once per update tick */
		void update(Scene *scene, glm::mat4 model, glm::mat4 view, glm::mat4 projection) {
			/* Update uniform buffer */
			UniformBufferObject ubo = {};
			ubo.modelMatrix = model;
			ubo.inverseModelMatrix = glm::inverse(model);
			ubo.normalMatrix = transpose(inverse(view * model));


			/* Map uniform buffer, copy data directly, then unmap */
			void* data;
			vkMapMemory(VKDK::device, uniformBufferMemories[scene->getRenderPass()], 0, sizeof(ubo), 0, &data);
			memcpy(data, &ubo, sizeof(ubo));
			vkUnmapMemory(VKDK::device, uniformBufferMemories[scene->getRenderPass()]);
		}

		void render(Scene *scene, std::shared_ptr<Components::Meshes::Mesh> mesh) {
			if (!mesh) {
				std::cout << "CubemapReflectionSurface: mesh is empty, not rendering!" << std::endl;
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
			uint32_t totalIndices = mesh->getTotalIndices();

			VkBuffer vertexBuffers[] = { vertexBuffer, normalBuffer };
			VkDeviceSize offsets[] = { 0, 0 };

			vkCmdBindVertexBuffers(scene->getCommandBuffer(), 0, 2, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(scene->getCommandBuffer(), indexBuffer, 0, VK_INDEX_TYPE_UINT16);
			vkCmdBindDescriptorSets(scene->getCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, 
				&descriptorSets[scene->getRenderPass()], 0, nullptr);

			/* Draw elements indexed */
			vkCmdDrawIndexed(scene->getCommandBuffer(), totalIndices, 1, 0, 0, 0);
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

		static std::vector<CubemapReflectionSurface> CubemapReflectionSurface::CubemapReflectionSurfaces;
		
		struct UniformBufferObject {
			glm::mat4 modelMatrix;
			glm::mat4 inverseModelMatrix;
			glm::mat4 normalMatrix;
		};

		/* A descriptor set layout describes how uniforms are used in the pipeline */
		static void createDescriptorSetLayout() {
			std::array<VkDescriptorSetLayoutBinding, 3> bindings = {};
			
			/* Camera buffer object */
			bindings[0].binding = 0;
			bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			bindings[0].descriptorCount = 1;
			bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT; 
			bindings[0].pImmutableSamplers = nullptr; // Optional

			/* Uniform buffer object */
			bindings[1].binding = 1;
			bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			bindings[1].descriptorCount = 1;
			bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			bindings[1].pImmutableSamplers = nullptr; // Optional

			/* Cubemap sampler */
			bindings[2].binding = 2;
			bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; //  this might not be right...
			bindings[2].descriptorCount = 1;
			bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			bindings[2].pImmutableSamplers = nullptr; // Optional

			VkDescriptorSetLayoutCreateInfo layoutInfo = {};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
			layoutInfo.pBindings = bindings.data();

			if (vkCreateDescriptorSetLayout(VKDK::device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
				throw std::runtime_error("failed to create CubemapReflectionSurface descriptor set layout!");
			}
		}

		static void createDescriptorPool() {
			std::array<VkDescriptorPoolSize, 2> poolSizes = {};
			poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			poolSizes[0].descriptorCount = maxDescriptorSets;
			poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			poolSizes[1].descriptorCount = maxDescriptorSets;

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
			auto vertShaderCode = readFile(ResourcePath "MaterialShaders/SurfaceMaterials/CubemapReflectionSurface/vert.spv");
			auto fragShaderCode = readFile(ResourcePath "MaterialShaders/SurfaceMaterials/CubemapReflectionSurface/frag.spv");

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
			std::vector<VkVertexInputBindingDescription> bindingDescriptions(2);

			/* The first structure is VKVertexInputBindingDescription */
			VkVertexInputBindingDescription vertexBindingDescription = {};
			vertexBindingDescription.binding = 0;
			vertexBindingDescription.stride = 3 * sizeof(float);
			vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			VkVertexInputBindingDescription normalBindingDescription = {};
			normalBindingDescription.binding = 1;
			normalBindingDescription.stride = 3 * sizeof(float);
			normalBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			bindingDescriptions[0] = vertexBindingDescription;
			bindingDescriptions[1] = normalBindingDescription;

			/* Move to next data entry after each vertex */
			/* Could be move to next data entry after each instance! */

			return bindingDescriptions;
		}

		static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
			std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);
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

			return attributeDescriptions;
		}

		/* Instanced material properties */
		std::shared_ptr<Components::Textures::Texture> cubemap;

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

				/* Cubemap Image sampler */
				VkDescriptorImageInfo skyboxImageInfo = {};
				skyboxImageInfo.imageLayout = cubemap->getLayout();
				skyboxImageInfo.imageView = cubemap->getColorImageView();
				skyboxImageInfo.sampler = cubemap->getSampler();

				std::array<VkWriteDescriptorSet, 3> descriptorWrites = {};

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
				descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				descriptorWrites[2].descriptorCount = 1;
				descriptorWrites[2].pImageInfo = &skyboxImageInfo;

				vkUpdateDescriptorSets(VKDK::device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
			}
		}
	};
}