#pragma once

#include "Components/Lights/Light.hpp"
#include "Systems/ComponentManager.hpp"

#include <glm/glm.hpp>
#include "Systems/SceneGraph.hpp"
#include "Entities/Entity.hpp"
#include "Components/Textures/RenderableTextureCube.hpp"

namespace Components::Lights {
#define MAXLIGHTS 10
  struct PointLightBufferItem {
    glm::vec4 color;
    uint32_t falloffType = 0;
    float intensity = 0;
    uint32_t pad2 = 0;
    uint32_t pad3 = 0;

    /* For shadow mapping */
    glm::mat4 localToWorld;
    glm::mat4 worldToLocal;
    glm::mat4 fproj;
    glm::mat4 bproj;
    glm::mat4 lproj;
    glm::mat4 rproj;
    glm::mat4 uproj;
    glm::mat4 dproj;
  };

  struct PointLightBufferObject {
    PointLightBufferItem lights[MAXLIGHTS];
    uint32_t totalLights = 0;
  };

	enum FalloffType {
		NONE = 0, LINEAR = 1, SQUARED = 2
	};

  class PointLights : public LightInterface {
  public:
    static std::shared_ptr<PointLights> Create(std::string name, bool castShadows = false, int shadowResolution = 512) {
      std::cout << "ComponentManager: Adding PointLight \"" << name << "\"" << std::endl;
      auto light = std::make_shared<Light>();
      auto pntLight = std::make_shared<PointLights>(name, castShadows, shadowResolution);
      light->light = pntLight;
      Systems::ComponentManager::Lights[name] = light;
      return pntLight;
    }
    static void Initialize() {
      createUniformBuffer();
    }
    static void UploadUBO() {
      PointLightBufferObject lbo;
      int counter = 0;

      /* Go through all entities, looking for those with light components */
      for (auto pair : Systems::SceneGraph::Entities) {
        auto lightComponent = pair.second->getFirstComponent<Light>();
        if (!lightComponent) continue;

        /* If the light component is a point light... */
        auto pointLight = std::dynamic_pointer_cast<PointLights>(lightComponent->light);
        if (pointLight) {
          PointLightBufferItem lboi = {};
          lboi.color = pointLight->getColor();
          lboi.worldToLocal = pair.second->getWorldToLocalMatrix();
          lboi.localToWorld = glm::inverse(lboi.worldToLocal);
          lboi.intensity = pointLight->getIntensity();
          lboi.falloffType = pointLight->getFalloffType();
          lbo.lights[counter] = lboi;
          counter++;
          lbo.totalLights = counter;
        }

        /* If we've hit the light limit, stop adding lights */
        if (counter >= MAXLIGHTS) {
          break;
        }
      }

      /* Now, upload lights to the GPU */
      void* data;
      vkMapMemory(VKDK::device, pointLightUBOMemory, 0, sizeof(PointLightBufferObject), 0, &data);
      memcpy(data, &lbo, sizeof(PointLightBufferObject));
      vkUnmapMemory(VKDK::device, pointLightUBOMemory);
    }
    static void Destroy() {
      vkDestroyBuffer(VKDK::device, pointLightUBO, nullptr);
      vkFreeMemory(VKDK::device, pointLightUBOMemory, nullptr);
    }
    static VkBuffer GetUBO() {
      return pointLightUBO;
    }

    PointLights(std::string name, bool castShadows = false, int shadowResolution = 512, VkRenderPass renderpass = VK_NULL_HANDLE) {
      if (castShadows) {
        //Textures::RenderableTextureCube::Create(name + "RenderCube", shadowResolution, shadowResolution, renderpass);
      }
    }

    void setColor(glm::vec4 color) {
      this->color = color;
    }
    glm::vec4 getColor() {
      return color;
    }

    void setIntensity(float intensity) {
      this->intensity = intensity;
    }
    float getIntensity() {
      return intensity;
    }
    void setFalloffType(FalloffType falloffType) {
      this->falloffType = falloffType;
    }
    uint32_t getFalloffType() {
      return falloffType;
    }
  private:
    static VkBuffer pointLightUBO;
    static VkDeviceMemory pointLightUBOMemory;
    static void createUniformBuffer() {
      VkDeviceSize bufferSize = sizeof(PointLightBufferObject);
      VKDK::CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        pointLightUBO, pointLightUBOMemory);
    }

    glm::vec4 color = glm::vec4(1.0, 1.0, 1.0, 1.0);
    uint32_t falloffType = 0;
    float intensity = 1.f;
  };
}