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

	class Voxelize : public MaterialInterface {
	public:
		static std::shared_ptr<Voxelize> Create(std::string name,
			PipelineKey pipelineKey,
			std::shared_ptr<Texture> output3DTexture,
			glm::vec4 diffuse = glm::vec4(1.0, 0.0, 1.0, 1.0),
			glm::vec4 specular = glm::vec4(1.0, 1.0, 1.0, 1.0),
			glm::vec4 ambient = glm::vec4(0.0, 0.0, 0.0, 1.0))
		{
			std::cout << "ComponentManager: Adding Voxelize \"" << name << "\"" << std::endl;

			auto material = std::make_shared<Material>();
			auto voxelizeMat = std::make_shared<Voxelize>(pipelineKey);
			voxelizeMat->output3DTexture = output3DTexture;
			voxelizeMat->setColor(diffuse, specular, ambient);
			material->material = voxelizeMat;
			Systems::ComponentManager::Materials[name] = material;
			return voxelizeMat;
		}

		static std::shared_ptr<Voxelize> Create(std::string name,
			PipelineKey pipelineKey,
			std::shared_ptr<Texture> output3DTexture,
			std::shared_ptr<Texture> diffuseTexture,
			glm::vec4 specular = glm::vec4(1.0, 1.0, 1.0, 1.0),
			glm::vec4 ambient = glm::vec4(0.0, 0.0, 0.0, 1.0))
		{
			std::cout << "ComponentManager: Adding Voxelize \"" << name << "\"" << std::endl;
			auto material = std::make_shared<Material>();
			auto voxelizeMat = std::make_shared<Voxelize>(pipelineKey);
			voxelizeMat->output3DTexture = output3DTexture;
			voxelizeMat->useDiffuseTexture = true;
			voxelizeMat->diffuseTextureComponent = diffuseTexture;
			voxelizeMat->setColor(glm::vec4(1.0, 0.0, 1.0, 1.0), specular, ambient);
			material->material = voxelizeMat;
			Systems::ComponentManager::Materials[name] = material;
			return voxelizeMat;
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

		static VkDescriptorSet CreateDescriptorSet(
      VkBuffer materialUBO, VkBuffer perspectiveUBO, VkBuffer transformUBO, VkBuffer pointLightUBO,
			VkImageView diffuseImageView, VkSampler diffuseSampler, 
      VkImageView specularImageView, VkSampler specularSampler, 
      VkImageView tex3DImageView, VkSampler tex3DSampler,
      VkImageView shadowMapImageView, VkSampler shadowMapSampler) {
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

			VkDescriptorBufferInfo pointLightBufferInfo = {};
			pointLightBufferInfo.buffer = pointLightUBO;
			pointLightBufferInfo.offset = 0;
			pointLightBufferInfo.range = sizeof(Components::Lights::PointLightBufferObject);

			VkDescriptorBufferInfo materialBufferInfo = {};
			materialBufferInfo.buffer = materialUBO;
			materialBufferInfo.offset = 0;
			materialBufferInfo.range = sizeof(MaterialBufferObject);

			/* Diffuse Image sampler */
			VkDescriptorImageInfo diffuseImageInfo = {};
			diffuseImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			diffuseImageInfo.imageView = diffuseImageView;
			diffuseImageInfo.sampler = diffuseSampler;

			/* Specular Image sampler */
			VkDescriptorImageInfo specularImageInfo = {};
			specularImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			specularImageInfo.imageView = specularImageView;
			specularImageInfo.sampler = specularSampler;

      /* Output voxelization */
			VkDescriptorImageInfo tex3DImageInfo = {};
			tex3DImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL; /* This might not be right */
			tex3DImageInfo.imageView = tex3DImageView;
			tex3DImageInfo.sampler = tex3DSampler;

      /* Shadow map sampler */
      VkDescriptorImageInfo shadowMapImageInfo = {};
      shadowMapImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      shadowMapImageInfo.imageView = shadowMapImageView;
      shadowMapImageInfo.sampler = shadowMapSampler;
			
			std::array<VkWriteDescriptorSet, 8> descriptorWrites = {};

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
			descriptorWrites[2].pBufferInfo = &pointLightBufferInfo;

			descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[3].dstSet = descriptorSet;
			descriptorWrites[3].dstBinding = 3;
			descriptorWrites[3].dstArrayElement = 0;
			descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[3].descriptorCount = 1;
			descriptorWrites[3].pBufferInfo = &materialBufferInfo;

			descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[4].dstSet = descriptorSet;
			descriptorWrites[4].dstBinding = 4;
			descriptorWrites[4].dstArrayElement = 0;
			descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[4].descriptorCount = 1;
			descriptorWrites[4].pImageInfo = &diffuseImageInfo;

			descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[5].dstSet = descriptorSet;
			descriptorWrites[5].dstBinding = 5;
			descriptorWrites[5].dstArrayElement = 0;
			descriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[5].descriptorCount = 1;
			descriptorWrites[5].pImageInfo = &specularImageInfo;

			descriptorWrites[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[6].dstSet = descriptorSet;
			descriptorWrites[6].dstBinding = 6;
			descriptorWrites[6].dstArrayElement = 0;
			descriptorWrites[6].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			descriptorWrites[6].descriptorCount = 1;
			descriptorWrites[6].pImageInfo = &tex3DImageInfo;

      descriptorWrites[7].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[7].dstSet = descriptorSet;
      descriptorWrites[7].dstBinding = 7;
      descriptorWrites[7].dstArrayElement = 0;
      descriptorWrites[7].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      descriptorWrites[7].descriptorCount = 1;
      descriptorWrites[7].pImageInfo = &shadowMapImageInfo;

			vkUpdateDescriptorSets(VKDK::device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
			return descriptorSet;
		}
		
		/* Note: one material instance per entity! Cleanup before destroying VKDK stuff */
		Voxelize(PipelineKey pipelineKey) {
			this->pipelineKey = pipelineKey;
			createUniformBuffer(sizeof(MaterialBufferObject));
		}

		/* TODO: use seperate descriptor set for model view projection, update only once per update tick */
		void uploadUBO() {
			/* Update uniform buffer */
			MaterialBufferObject mbo = {};
			mbo.ka = ka;
			mbo.kd = kd;
			mbo.ks = ks;
			mbo.diffuseReflectivity = diffuseReflectivity;
			mbo.specularReflectivity = specularReflectivity;
			mbo.transparency = transparency;
			mbo.emissivity = emissivity;
			mbo.useDiffuseTexture = useDiffuseTexture;
			mbo.useSpecularTexture = useSpecularTexture;
      mbo.useShadowMap = useShadowMapTextureComponent;

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
				VkSampler diffuseSampler, specularSampler, tex3DSampler, shadowMapSampler;
				VkImageView diffuseImageView, specularImageView, tex3DImageView, shadowMapImageView;
				auto missingTexture = Systems::ComponentManager::Textures["DefaultTexture"];
        auto missingTexture3D = Systems::ComponentManager::Textures["DefaultTexture3D"];
        auto missingTextureCube = Systems::ComponentManager::Textures["DefaultTextureCube"];
				diffuseSampler = specularSampler = missingTexture->texture->getColorSampler();
				diffuseImageView = specularImageView = missingTexture->texture->getColorImageView();
				tex3DSampler = missingTexture3D->texture->getColorSampler();
				tex3DImageView = missingTexture3D->texture->getColorImageView();
        shadowMapSampler = missingTextureCube->texture->getColorSampler();
        shadowMapImageView = missingTextureCube->texture->getColorImageView();

				if (useDiffuseTexture) {
					diffuseSampler = diffuseTextureComponent->texture->getColorSampler();
					diffuseImageView = diffuseTextureComponent->texture->getColorImageView();
				}

				if (output3DTexture) {
					tex3DSampler = output3DTexture->texture->getColorSampler();
					tex3DImageView = output3DTexture->texture->getColorImageView();
				}

        if (useShadowMapTextureComponent && shadowMapTextureComponent) {
          shadowMapSampler = shadowMapTextureComponent->texture->getDepthSampler();
          shadowMapImageView = shadowMapTextureComponent->texture->getDepthImageView();
        }

				getStaticProperties().descriptorSets[key] = CreateDescriptorSet(materialUBO, uboSet.perspectiveUBO,
					uboSet.transformUBO, uboSet.pointLightUBO, diffuseImageView, diffuseSampler,
					specularImageView, specularSampler, tex3DImageView, tex3DSampler, shadowMapImageView, shadowMapSampler);
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

		void setColor(glm::vec4 kd, glm::vec4 ks, glm::vec4 ka) {
			this->ka = ka;
			this->kd = kd;
			this->ks = ks;
		}

    void useShadowMapTexture(bool useTexture) {
      this->useShadowMapTextureComponent = useTexture;
    }

    void setShadowMapTexture(std::shared_ptr<Components::Textures::Texture> texture) {
      this->shadowMapTextureComponent = texture;
    }

	private:
		struct MaterialBufferObject {

			glm::vec4 ka;
			glm::vec4 kd;
			glm::vec4 ks;
			float diffuseReflectivity;
			float specularReflectivity;
			float emissivity;
			float transparency;
			uint32_t useDiffuseTexture;
			uint32_t useSpecularTexture;
      uint32_t useShadowMap;
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

			VkDescriptorSetLayoutBinding pointLightLayoutBinding = {};
			pointLightLayoutBinding.binding = 2;
			pointLightLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			pointLightLayoutBinding.descriptorCount = 1;
			pointLightLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			pointLightLayoutBinding.pImmutableSamplers = nullptr; // Optional

			VkDescriptorSetLayoutBinding materialLayoutBinding = {};
			materialLayoutBinding.binding = 3;
			materialLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			materialLayoutBinding.descriptorCount = 1;
			materialLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			materialLayoutBinding.pImmutableSamplers = nullptr; // Optional

			/* 2D Diffuse Texture Sampler object */
			VkDescriptorSetLayoutBinding diffuseTextureLayoutBinding = {};
			diffuseTextureLayoutBinding.binding = 4;
			diffuseTextureLayoutBinding.descriptorCount = 1;
			diffuseTextureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			diffuseTextureLayoutBinding.pImmutableSamplers = nullptr;
			diffuseTextureLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

			/* 2D Specular Texture Sampler object */
			VkDescriptorSetLayoutBinding specularTextureLayoutBinding = {};
			specularTextureLayoutBinding.binding = 5;
			specularTextureLayoutBinding.descriptorCount = 1;
			specularTextureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			specularTextureLayoutBinding.pImmutableSamplers = nullptr;
			specularTextureLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

			/* 3D Image Storage */
			VkDescriptorSetLayoutBinding image3DLayoutBinding = {};
			image3DLayoutBinding.binding = 6;
			image3DLayoutBinding.descriptorCount = 1;
			image3DLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			image3DLayoutBinding.pImmutableSamplers = nullptr;
			image3DLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

      /* Shadow map */
      VkDescriptorSetLayoutBinding shadowMapLayoutBinding = {};
      shadowMapLayoutBinding.binding = 7;
      shadowMapLayoutBinding.descriptorCount = 1;
      shadowMapLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      shadowMapLayoutBinding.pImmutableSamplers = nullptr;
      shadowMapLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

			std::array<VkDescriptorSetLayoutBinding, 8> bindings = {
				perspectiveLayoutBinding, transformLayoutBinding, pointLightLayoutBinding, materialLayoutBinding,
				diffuseTextureLayoutBinding, specularTextureLayoutBinding, image3DLayoutBinding, 
        shadowMapLayoutBinding };

			VkDescriptorSetLayoutCreateInfo layoutInfo = {};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
			layoutInfo.pBindings = bindings.data();

			if (vkCreateDescriptorSetLayout(VKDK::device, &layoutInfo, nullptr, &getStaticProperties().descriptorSetLayout) != VK_SUCCESS) {
				throw std::runtime_error("failed to create Voxelize descriptor set layout!");
			}
		}

		static void createDescriptorPool() {
			std::array<VkDescriptorPoolSize, 8> poolSizes = {};
			poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			poolSizes[0].descriptorCount = getStaticProperties().maxDescriptorSets;
			poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			poolSizes[1].descriptorCount = getStaticProperties().maxDescriptorSets;
			poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			poolSizes[2].descriptorCount = getStaticProperties().maxDescriptorSets;
			poolSizes[3].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			poolSizes[3].descriptorCount = getStaticProperties().maxDescriptorSets;
			poolSizes[4].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			poolSizes[4].descriptorCount = getStaticProperties().maxDescriptorSets;
			poolSizes[5].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			poolSizes[5].descriptorCount = getStaticProperties().maxDescriptorSets;
			poolSizes[6].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			poolSizes[6].descriptorCount = getStaticProperties().maxDescriptorSets;
      poolSizes[7].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      poolSizes[7].descriptorCount = getStaticProperties().maxDescriptorSets;

			VkDescriptorPoolCreateInfo poolInfo = {};
			poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
			poolInfo.pPoolSizes = poolSizes.data();
			poolInfo.maxSets = getStaticProperties().maxDescriptorSets;
			poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

			if (vkCreateDescriptorPool(VKDK::device, &poolInfo, nullptr, &getStaticProperties().descriptorPool) != VK_SUCCESS) {
				throw std::runtime_error("failed to create Voxelize descriptor pool!");
			}
		}

		static void setupGraphicsPipeline() {
			/* Read in shader modules */
			auto vertShaderCode = readFile(ResourcePath "MaterialShaders/Volume/Voxelize/vert.spv");
			auto geomShaderCode = readFile(ResourcePath "MaterialShaders/Volume/Voxelize/geom.spv");
			auto fragShaderCode = readFile(ResourcePath "MaterialShaders/Volume/Voxelize/frag.spv");

			/* Create shader modules */
			auto vertShaderModule = createShaderModule(vertShaderCode);
			auto geomShaderModule = createShaderModule(geomShaderCode);
			auto fragShaderModule = createShaderModule(fragShaderCode);

			/* Info for shader stages */
			VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
			vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			vertShaderStageInfo.module = vertShaderModule;
			vertShaderStageInfo.pName = "main";

			VkPipelineShaderStageCreateInfo geomShaderStageInfo = {};
			geomShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			geomShaderStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
			geomShaderStageInfo.module = geomShaderModule;
			geomShaderStageInfo.pName = "main";

			VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
			fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			fragShaderStageInfo.module = fragShaderModule;
			fragShaderStageInfo.pName = "main";

			std::vector<VkPipelineShaderStageCreateInfo> shaderStages = { vertShaderStageInfo, geomShaderStageInfo, fragShaderStageInfo };
			std::vector<VkPipeline> newPipelines;
			std::vector<VkPipelineLayout> newLayouts;

			createPipelines(shaderStages, getBindingDescriptions(),
				getAttributeDescriptions(), { getStaticProperties().descriptorSetLayout },
				getStaticProperties().pipelines, getStaticProperties().pipelineLayout);

			vkDestroyShaderModule(VKDK::device, fragShaderModule, nullptr);
			vkDestroyShaderModule(VKDK::device, geomShaderModule, nullptr);
			vkDestroyShaderModule(VKDK::device, vertShaderModule, nullptr);
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

			VkVertexInputBindingDescription texcoordBindingDescription = {};
			texcoordBindingDescription.binding = 2;
			texcoordBindingDescription.stride = 2 * sizeof(float);
			texcoordBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			bindingDescriptions[0] = vertexBindingDescription;
			bindingDescriptions[1] = normalBindingDescription;
			bindingDescriptions[2] = texcoordBindingDescription;

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

		public:
		/* Instanced material properties */
		glm::vec4 ka = glm::vec4(0.5, 0.5, 0.5, 1.0);
		glm::vec4 kd = glm::vec4(0.5, 0.5, 0.5, 1.0);
		glm::vec4 ks = glm::vec4(0.5, 0.5, 0.5, 1.0);
		float diffuseReflectivity = 0.0;
		float specularReflectivity = 0.0;
		float emissivity = 0.0;
		float transparency = 0.0;

		std::shared_ptr<Texture> output3DTexture;

		bool useDiffuseTexture = false;
		bool useSpecularTexture = false;
    bool useShadowMapTextureComponent = false;
    std::shared_ptr<Texture> diffuseTextureComponent;
    std::shared_ptr<Texture> shadowMapTextureComponent;
	};
}