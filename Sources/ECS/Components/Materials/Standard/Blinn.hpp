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

namespace Components::Materials::Standard {
  using namespace Components::Math;
  using namespace Components::Meshes;
  using namespace Components::Textures;

  class Blinn : public MaterialInterface {
  public:
    static std::shared_ptr<Blinn> Create(std::string name, PipelineKey pipelineKey)
    {
      std::cout << "ComponentManager: Adding Blinn \"" << name << "\"" << std::endl;

      auto material = std::make_shared<Material>();
      auto blinnMat = std::make_shared<Blinn>(pipelineKey);
      material->material = blinnMat;
      Systems::ComponentManager::Materials[name] = material;
      return blinnMat;
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
      VkImageView reflectionImageView, VkSampler reflectionSampler,
      VkImageView shadowMapImageView, VkSampler shadowMapSampler,
      VkSampler voxelSampler, VkImageView voxelImageView)
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

      /* Reflection Image sampler */
      VkDescriptorImageInfo reflectionImageInfo = {};
      reflectionImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      reflectionImageInfo.imageView = reflectionImageView;
      reflectionImageInfo.sampler = reflectionSampler;

      /* Voxel cone tracing volume */
      VkDescriptorImageInfo voxelImageInfo = {};
      voxelImageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
      voxelImageInfo.imageView = voxelImageView;
      voxelImageInfo.sampler = voxelSampler;

      /* Shadowmap Image sampler */
      VkDescriptorImageInfo shadowMapImageInfo = {};
      shadowMapImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      shadowMapImageInfo.imageView = shadowMapImageView;
      shadowMapImageInfo.sampler = shadowMapSampler;

      std::array<VkWriteDescriptorSet, 9> descriptorWrites = {};

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
      descriptorWrites[6].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      descriptorWrites[6].descriptorCount = 1;
      descriptorWrites[6].pImageInfo = &reflectionImageInfo;

      descriptorWrites[7].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[7].dstSet = descriptorSet;
      descriptorWrites[7].dstBinding = 7;
      descriptorWrites[7].dstArrayElement = 0;
      descriptorWrites[7].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      descriptorWrites[7].descriptorCount = 1;
      descriptorWrites[7].pImageInfo = &voxelImageInfo;

      descriptorWrites[8].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptorWrites[8].dstSet = descriptorSet;
      descriptorWrites[8].dstBinding = 8;
      descriptorWrites[8].dstArrayElement = 0;
      descriptorWrites[8].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      descriptorWrites[8].descriptorCount = 1;
      descriptorWrites[8].pImageInfo = &shadowMapImageInfo;

      vkUpdateDescriptorSets(VKDK::device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
      return descriptorSet;
    }

    /* Note: one material instance per entity! Cleanup before destroying VKDK stuff */
    Blinn(PipelineKey pipelineKey) {
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
      mbo.kr = kr;
      mbo.useRoughnessLod = useRoughnessLod;
      mbo.roughness = roughness;
      mbo.useDiffuseTexture = useDiffuseTextureComponent;
      mbo.useSpecularTexture = useSpecularTextureComponent;
      mbo.useCubemapTexture = useReflectionTextureComponent;
      mbo.useShadowMap = useShadowMapTextureComponent;
      mbo.useGI = useGI;

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
        VkSampler diffuseSampler, specularSampler, reflectionSampler, shadowMapSampler, voxelSampler;
        VkImageView diffuseImageView, specularImageView, reflectionImageView, shadowMapImageView, voxelImageView;
        auto missingTexture = Systems::ComponentManager::Textures["DefaultTexture"];
        diffuseSampler = specularSampler = missingTexture->texture->getColorSampler();
        diffuseImageView = specularImageView = missingTexture->texture->getColorImageView();

        auto missingCubemap = Systems::ComponentManager::Textures["DefaultTextureCube"];
        reflectionSampler = shadowMapSampler = missingCubemap->texture->getColorSampler();
        reflectionImageView = shadowMapImageView = missingCubemap->texture->getColorImageView();

        auto missingVolume = Systems::ComponentManager::Textures["DefaultTexture3D"];
        voxelSampler = missingVolume->texture->getColorSampler();
        voxelImageView = missingVolume->texture->getColorImageView();

        if (diffuseTextureComponent && useDiffuseTextureComponent) {
          diffuseSampler = diffuseTextureComponent->texture->getColorSampler();
          diffuseImageView = diffuseTextureComponent->texture->getColorImageView();
        }

        if (specularTextureComponent && useSpecularTextureComponent) {
          specularSampler = specularTextureComponent->texture->getColorSampler();
          specularImageView = specularTextureComponent->texture->getColorImageView();
        }

        if (reflectionTextureComponent && useReflectionTextureComponent) {
          reflectionSampler = reflectionTextureComponent->texture->getColorSampler();
          reflectionImageView = reflectionTextureComponent->texture->getColorImageView();
        }

        if (shadowMapTextureComponent && useShadowMapTextureComponent) {
          shadowMapSampler = shadowMapTextureComponent->texture->getDepthSampler();
          shadowMapImageView = shadowMapTextureComponent->texture->getDepthImageView();
        }

        if (voxelTextureComponent && useGI) {
          voxelSampler = voxelTextureComponent->texture->getColorSampler();
          voxelImageView = voxelTextureComponent->texture->getColorImageView();
        }

        getStaticProperties().descriptorSets[key] = CreateDescriptorSet(materialUBO, uboSet.perspectiveUBO,
          uboSet.transformUBO, uboSet.pointLightUBO, diffuseImageView, diffuseSampler,
          specularImageView, specularSampler, reflectionImageView, reflectionSampler,
          shadowMapImageView, shadowMapSampler, voxelSampler, voxelImageView);
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
      vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
        getStaticProperties().pipelineLayout, 0, 1,
        &descriptorSet, 0, nullptr);

      /* Draw elements indexed */
      vkCmdDrawIndexed(commandBuffer, totalIndices, 1, 0, 0, 0);
    }

    void setColor(
      glm::vec4 kd,
      glm::vec4 ks = glm::vec4(1.0, 1.0, 1.0, 1.0),
      glm::vec4 ka = glm::vec4(0.0, 0.0, 0.0, 1.0),
      glm::vec4 kr = glm::vec4(0.0, 0.0, 0.0, 1.0))
    {
      this->ka = ka;
      this->kd = kd;
      this->ks = ks;
      this->kr = kr;
    }

    void useDiffuseTexture(bool useTexture) {
      this->useDiffuseTextureComponent = useTexture;
    }
    void useSpecularTexture(bool useTexture) {
      this->useSpecularTextureComponent = useTexture;
    }
    void useCubemapTexture(bool useTexture) {
      this->useReflectionTextureComponent = useTexture;
    }
    void useShadowMapTexture(bool useTexture) {
      this->useShadowMapTextureComponent = useTexture;
    }
    void useGlobalIllumination(bool useGI) {
      this->useGI = useGI;
    }
    void useRoughness(bool useRoughnessLod) {
      this->useRoughnessLod = useRoughnessLod;
    }

    void setDiffuseTexture(std::shared_ptr<Components::Textures::Texture> texture) {
      this->diffuseTextureComponent = texture;
    }
    void setSpecularTexture(std::shared_ptr<Components::Textures::Texture> texture) {
      this->specularTextureComponent = texture;
    }
    void setCubemapTexture(std::shared_ptr<Components::Textures::Texture> texture) {
      this->reflectionTextureComponent = texture;
    }
    void setShadowMapTexture(std::shared_ptr<Components::Textures::Texture> texture) {
      this->shadowMapTextureComponent = texture;
    }
    void setRoughness(float roughness) {
      this->roughness = roughness;
    }
    void setGlobalIlluminationVolume(std::shared_ptr<Components::Textures::Texture> texture) {
      this->voxelTextureComponent = texture;
    }

  private:
    struct MaterialBufferObject {
      glm::vec4 ka;
      glm::vec4 kd;
      glm::vec4 ks;
      glm::vec4 kr;
      uint32_t useRoughnessLod;
      float roughness;
      uint32_t useDiffuseTexture;
      uint32_t useSpecularTexture;
      uint32_t useCubemapTexture;
      uint32_t useGI;
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

      /* Cubemap Texture Sampler object */
      VkDescriptorSetLayoutBinding cubemapTextureLayoutBinding = {};
      cubemapTextureLayoutBinding.binding = 6;
      cubemapTextureLayoutBinding.descriptorCount = 1;
      cubemapTextureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      cubemapTextureLayoutBinding.pImmutableSamplers = nullptr;
      cubemapTextureLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

      /* Voxel Cone Tracing Volume */
      VkDescriptorSetLayoutBinding voxelTextureLayoutBinding = {};
      voxelTextureLayoutBinding.binding = 7;
      voxelTextureLayoutBinding.descriptorCount = 1;
      voxelTextureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      voxelTextureLayoutBinding.pImmutableSamplers = nullptr;
      voxelTextureLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

      /* Shadow Map */
      VkDescriptorSetLayoutBinding shadowMapTextureLayoutBinding = {};
      shadowMapTextureLayoutBinding.binding = 8;
      shadowMapTextureLayoutBinding.descriptorCount = 1;
      shadowMapTextureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      shadowMapTextureLayoutBinding.pImmutableSamplers = nullptr;
      shadowMapTextureLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

      std::array<VkDescriptorSetLayoutBinding, 9> bindings = {
        perspectiveLayoutBinding, transformLayoutBinding, pointLightLayoutBinding, materialLayoutBinding,
        diffuseTextureLayoutBinding, specularTextureLayoutBinding, cubemapTextureLayoutBinding,
        voxelTextureLayoutBinding, shadowMapTextureLayoutBinding };

      VkDescriptorSetLayoutCreateInfo layoutInfo = {};
      layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
      layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
      layoutInfo.pBindings = bindings.data();

      if (vkCreateDescriptorSetLayout(VKDK::device, &layoutInfo, nullptr, &getStaticProperties().descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create BlinnSurface descriptor set layout!");
      }
    }

    static void createDescriptorPool() {
      std::array<VkDescriptorPoolSize, 9> poolSizes = {};
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
      poolSizes[6].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      poolSizes[6].descriptorCount = getStaticProperties().maxDescriptorSets;
      poolSizes[7].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      poolSizes[7].descriptorCount = getStaticProperties().maxDescriptorSets;
      poolSizes[8].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      poolSizes[8].descriptorCount = getStaticProperties().maxDescriptorSets;

      VkDescriptorPoolCreateInfo poolInfo = {};
      poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
      poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
      poolInfo.pPoolSizes = poolSizes.data();
      poolInfo.maxSets = getStaticProperties().maxDescriptorSets;
      poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

      if (vkCreateDescriptorPool(VKDK::device, &poolInfo, nullptr, &getStaticProperties().descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create Blinn descriptor pool!");
      }
    }

    static void setupGraphicsPipeline() {
      /* Read in shader modules */
      auto vertShaderCode = readFile(ResourcePath "MaterialShaders/Standard/Blinn/vert.spv");
      auto fragShaderCode = readFile(ResourcePath "MaterialShaders/Standard/Blinn/frag.spv");

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
    glm::vec4 kr = glm::vec4(0.5, 0.5, 0.5, 1.0);
    float roughness = 0.0;

    bool useDiffuseTextureComponent = false;
    bool useSpecularTextureComponent = false;
    bool useReflectionTextureComponent = false;
    bool useShadowMapTextureComponent = false;
    bool useRoughnessLod = false;
    bool useGI = false;
    std::shared_ptr<Texture> diffuseTextureComponent;
    std::shared_ptr<Texture> specularTextureComponent;
    std::shared_ptr<Texture> reflectionTextureComponent;
    std::shared_ptr<Texture> shadowMapTextureComponent;
    std::shared_ptr<Texture> voxelTextureComponent;
  };
}