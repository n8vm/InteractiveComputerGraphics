#pragma once
#include <unordered_map>
#include "vkdk.hpp"
#include "Components/Math/Transform.hpp"

/* Forward Declarations */
namespace Components::Textures { class RenderableTexture2D; }
namespace Components::Textures { class Texture; }
namespace Entities { class Entity; }
namespace Entities::Lights { class Light; }

class Scene {
public:
	/* If a scene is the main scene, it will render to the swapchain frame buffers. */
	bool useSwapchain = false;
	Components::Perspectives perspective = Components::Perspectives::Forward;

	/* A scene has a render pass */
	VkRenderPass renderpass;
	VkCommandBuffer commandBuffer;

	std::shared_ptr<Components::Textures::RenderableTexture2D> renderTexture;

	/* A scene has a set clear color */
	glm::vec4 clearColor = glm::vec4(0.0, 0.0, 0.0, 0.0);

	/* A scene can recursively contain "subscenes". Used for multiple render passes. */
	std::unordered_map<std::string, std::shared_ptr<Scene>> subScenes;
	
	/* A scene has a camera, which is used for rendering */
	std::shared_ptr<Entities::Entity> camera;
	VkBuffer cameraBuffer;
	VkDeviceMemory cameraBufferMemory;

	/* A scene has a list of lights */
	std::unordered_map<std::string, std::shared_ptr<Entities::Lights::Light>> LightList;
	VkBuffer lightBuffer;
	VkDeviceMemory lightBufferMemory;

	/* A scene has a collection of entities, which act like a traditional scene graph. */
	std::shared_ptr<Entities::Entity> entities;

	Scene(bool useSwapchain = false, std::string renderTextureName = "", int framebufferWidth = -1, int framebufferHeight = -1);
	
	void createRenderPass(std::string renderTextureName, uint32_t framebufferWidth, uint32_t framebufferHeight);

	void createCommandBuffer();

	void createLightUniformBuffer();

	void createCameraUniformBuffer();

	void createGlobalUniformBuffers();

	void cleanup();

	void(*updateCallback) (Scene*) = nullptr;

	void updateLightBuffer();

	/* Useful for altering the location of the scene eg for reflections */
	glm::mat4 SceneTransform = glm::mat4(1.0);

	void updateCameraBuffer();

	virtual void updateCameraUBO();

	virtual void update();

	void setClearColor(glm::vec4 clearColor);

	void recordRenderPass();

	void beginRecordingRenderPass();

	void endRecordingRenderPass();

	VkCommandBuffer getCommandBuffer();

	VkRenderPass getRenderPass();

	VkBuffer getCameraBuffer();

	VkBuffer getLightBuffer();

	void setPerspective(Components::Perspectives newPerspective);

	struct SubmitToGraphicsQueueInfo {
		std::vector<VkSemaphore> waitSemaphores;
		VkPipelineStageFlags submitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		std::vector<VkCommandBuffer> commandBuffers;
		std::vector<VkSemaphore> signalSemaphores;
		VkQueue graphicsQueue;
	};

	void submitToGraphicsQueue(SubmitToGraphicsQueueInfo &submitToGraphicsQueueInfo);
};