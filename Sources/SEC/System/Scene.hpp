#pragma once
#include <unordered_map>

#include "Entities/Entity.hpp"
#include "Entities/Cameras/Camera.hpp"
#include "Entities/Lights/Light.hpp"
#include "Components/Textures/RenderableTexture2D.hpp"
#include "vkdk.hpp"
#include "System.hpp"

class Scene {
public:
	/* If a scene is the main scene, it will render to the swapchain frame buffers. */
	bool useSwapchain = false;

	/* A scene has a render pass */
	VkRenderPass renderpass;
	VkCommandBuffer commandBuffer;

	std::shared_ptr<Components::Textures::RenderableTexture2D> renderTexture;

	/* A scene has a set clear color */
	glm::vec4 clearColor = glm::vec4(0.0, 0.0, 0.0, 0.0);

	/* A scene can recursively contain "subscenes". Used for multiple render passes. */
	std::unordered_map<std::string, std::shared_ptr<Scene>> subScenes;
	
	/* A scene has a camera, which is used for rendering */
	std::shared_ptr<Entities::Cameras::Camera> camera;
	VkBuffer cameraBuffer;
	VkDeviceMemory cameraBufferMemory;

	/* A scene has a list of lights */
	std::unordered_map<std::string, std::shared_ptr<Entities::Lights::Light>> LightList;
	VkBuffer lightBuffer;
	VkDeviceMemory lightBufferMemory;

	/* A scene has a collection of entities, which act like a traditional scene graph. */
	std::shared_ptr<Entities::Entity> entities;

	Scene(bool useSwapchain = false, std::string renderTextureName = "", int framebufferWidth = -1, int framebufferHeight = -1) {
		this->useSwapchain = useSwapchain;
		
		/* If we're using the swapchain, use the swapchain's render pass. (this might change... ) */
		if (useSwapchain) {
			renderpass = VKDK::renderPass;
		}
		/* Else, create a render pass, and a command buffer */
		else {
			createRenderPass(renderTextureName, framebufferWidth, framebufferHeight);
			createCommandBuffer();
		}

		createGlobalUniformBuffers();

		entities = std::make_shared<Entities::Entity>();
	}

	void createRenderPass(std::string renderTextureName, uint32_t framebufferWidth, uint32_t framebufferHeight) {
		using namespace VKDK;

		// Create a separate render pass for the offscreen rendering as it may differ from the one used for scene rendering
		std::array<VkAttachmentDescription, 2> attchmentDescriptions = {};

		// Color attachment
		attchmentDescriptions[0].format = Components::Textures::RenderableTexture2D::GetImageColorFormat();
		attchmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
		attchmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attchmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attchmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attchmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attchmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attchmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		// Depth attachment
		attchmentDescriptions[1].format = Components::Textures::RenderableTexture2D::GetImageDepthFormat();
		attchmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
		attchmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attchmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attchmentDescriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attchmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attchmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attchmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
		VkAttachmentReference depthReference = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

		VkSubpassDescription subpassDescription = {};
		subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescription.colorAttachmentCount = 1;
		subpassDescription.pColorAttachments = &colorReference;
		subpassDescription.pDepthStencilAttachment = &depthReference;

		// Use subpass dependencies for layout transitions
		std::array<VkSubpassDependency, 2> dependencies;

		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		// Create the actual renderpass
		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attchmentDescriptions.size());
		renderPassInfo.pAttachments = attchmentDescriptions.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpassDescription;
		renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
		renderPassInfo.pDependencies = dependencies.data();

		VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderpass));
		renderTexture = make_shared<Components::Textures::RenderableTexture2D>(framebufferWidth, framebufferHeight, renderpass);
		System::TextureList[renderTextureName] = renderTexture;
	}

	void createCommandBuffer() {
		/* For convenience, also create an offscreen command buffer */
		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = VKDK::commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		if (vkAllocateCommandBuffers(VKDK::device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate offscreen command buffer!");
		}
	}

	void createLightUniformBuffer() {
		VkDeviceSize bufferSize = sizeof(Entities::Lights::LightBufferObject);
		VKDK::CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, lightBuffer, lightBufferMemory);
	}

	void createCameraUniformBuffer() {
		VkDeviceSize bufferSize = sizeof(Entities::Cameras::CameraBufferObject);
		VKDK::CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, cameraBuffer, cameraBufferMemory);
	}

	void createGlobalUniformBuffers() {
		createLightUniformBuffer();
		createCameraUniformBuffer();
	}

	void cleanup() {
		/* First, cleanup all sub scenes*/
		for (auto i : subScenes) {
			i.second->cleanup();
		}

		/* Destroy any offscreen render passes */
		if (!useSwapchain) {
			vkDestroyRenderPass(VKDK::device, renderpass, nullptr);
		}

		/* Destroy camera and light uniform buffers */
		vkDestroyBuffer(VKDK::device, lightBuffer, nullptr);
		vkFreeMemory(VKDK::device, lightBufferMemory, nullptr);

		vkDestroyBuffer(VKDK::device, cameraBuffer, nullptr);
		vkFreeMemory(VKDK::device, cameraBufferMemory, nullptr);
	}

	void(*updateCallback) (Scene*) = nullptr;

	void updateLightBuffer() {
		Entities::Lights::LightBufferObject lbo = {};

		/* To do: fill light buffer object here */
		lbo.numLights = LightList.size();
		int i = 0;
		for (auto light : LightList) {
			lbo.lights[i].position = glm::vec4(light.second->transform.GetPosition(), 1.0);
			lbo.lights[i].ambient = glm::vec4(light.second->getAmbientColor(), 1.0);
			lbo.lights[i].diffuse = glm::vec4(light.second->getDiffuseColor(), 1.0);
			lbo.lights[i].specular = glm::vec4(light.second->getSpecularColor(), 1.0);
			++i;
		}

		/* Map uniform buffer, copy data directly, then unmap */
		void* data;
		vkMapMemory(VKDK::device, lightBufferMemory, 0, sizeof(lbo), 0, &data);
		memcpy(data, &lbo, sizeof(lbo));
		vkUnmapMemory(VKDK::device, lightBufferMemory);
	}

	/* Useful for altering the location of the scene eg for reflections */
	glm::mat4 SceneTransform = glm::mat4(1.0);

	void updateCameraBuffer() {
		if (!camera) return;

		/* Update uniform buffer */
		Entities::Cameras::CameraBufferObject cbo = {};

		/* To do: fill camera buffer object here */
		cbo.View = camera->getView() * SceneTransform;
		cbo.Projection = camera->getProjection();
		cbo.Projection[1][1] *= -1; // required so that image doesn't flip upside down.
		cbo.ProjectionInverse = glm::inverse(camera->getProjection()); // Todo: account for -1 here...
		cbo.ViewInverse = glm::inverse(camera->getView() * SceneTransform);

		/* Map uniform buffer, copy data directly, then unmap */
		void* data;
		vkMapMemory(VKDK::device, cameraBufferMemory, 0, sizeof(cbo), 0, &data);
		memcpy(data, &cbo, sizeof(cbo));
		vkUnmapMemory(VKDK::device, cameraBufferMemory);
	}

	virtual void updateCameraUBO() {
		///* First, update all sub scenes*/
		//for (auto i : subScenes) {
		//	i.second->update();
		//}


	}

	virtual void update() {
		/* First, update all sub scenes*/
		for (auto i : subScenes) {
			i.second->update();
		}

		/* Next, update UBO data for this scene */
		updateCameraBuffer();
		updateLightBuffer();

		/* Finally, call any update callbacks, and update the entity graph. */
		if (updateCallback) 
			updateCallback(this);

		if(camera)
			entities->update(glm::mat4(1.0), camera->getView() * SceneTransform, camera->getProjection());
	};

	void setClearColor(glm::vec4 clearColor) {
		this->clearColor = clearColor;
	}

	void recordRenderPass() {
		/* First, render all subscenes*/
		for (auto i : subScenes) {
			i.second->recordRenderPass();
		}

		/* Update VP matrix */
		if (camera)
			camera->setWindowSize(VKDK::swapChainExtent.width, VKDK::swapChainExtent.height);

		/* Now, record rendering this scene */
		if (useSwapchain) {
			/* For each command buffer used by the swap chain */
			for (int i = 0; i < VKDK::drawCmdBuffers.size(); ++i) {
				commandBuffer = VKDK::drawCmdBuffers[i];

				/* Starting command buffer recording */
				VkCommandBufferBeginInfo beginInfo = {};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
				beginInfo.pInheritanceInfo = nullptr; // Optional

				vkBeginCommandBuffer(commandBuffer, &beginInfo);

				/* information about this particular render pass */
				VkRenderPassBeginInfo renderPassInfo = {};
				renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				renderPassInfo.renderPass = VKDK::renderPass;
				renderPassInfo.framebuffer = VKDK::swapChainFramebuffers[i];
				renderPassInfo.renderArea.offset = { 0, 0 };
				renderPassInfo.renderArea.extent = VKDK::swapChainExtent;
				std::array<VkClearValue, 2> clearValues = {};
				clearValues[0].color = { clearColor.r, clearColor.g, clearColor.b, clearColor.a };
				clearValues[1].depthStencil = { 1.0f, 0 };

				renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
				renderPassInfo.pClearValues = clearValues.data();

				/* Start the render pass */
				vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

				/* Set viewport*/
				VkViewport viewport = vks::initializers::viewport((float)VKDK::swapChainExtent.width, (float)VKDK::swapChainExtent.height, 0.0f, 1.0f);
				vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

				/* Set Scissors */
				VkRect2D scissor = vks::initializers::rect2D(VKDK::swapChainExtent.width, VKDK::swapChainExtent.height, 0, 0);
				vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

				/* Record rendering of the scene */
				entities->render(this);

				vkCmdEndRenderPass(commandBuffer);

				VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
			}
		}
		else {			
			/* Render to offscreen texture */
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
			beginInfo.pInheritanceInfo = nullptr; // Optional
			vkBeginCommandBuffer(commandBuffer, &beginInfo);
			
			/* Information about this particular render pass */
			VkRenderPassBeginInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = renderpass;
			renderPassInfo.framebuffer = renderTexture->getFrameBuffer();
			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = { (uint32_t)renderTexture->getWidth(), (uint32_t)renderTexture->getHeight() };
			std::array<VkClearValue, 2> clearValues = {};
			clearValues[0].color = { clearColor.r, clearColor.g, clearColor.b, clearColor.a };
			clearValues[1].depthStencil = { 1.0f, 0 };
			renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
			renderPassInfo.pClearValues = clearValues.data();
			
			/* Start the render pass */
			vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			
			/* Set viewport*/
			VkViewport viewport = vks::initializers::viewport((float)renderTexture->getWidth(), (float)renderTexture->getHeight(), 0.0f, 1.0f);
			vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
			
			/* Set Scissors */
			VkRect2D scissor = vks::initializers::rect2D(renderTexture->getWidth(), renderTexture->getHeight(), 0, 0);
			vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
			
			/* Update VP matrix */
			camera->setWindowSize(renderTexture->getWidth(), renderTexture->getHeight());
			
			/* Record rendering of the scene */
			entities->render(this);
			
			/* End offscreen render */
			vkCmdEndRenderPass(commandBuffer);
			VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
		}
	}

	void beginRecordingRenderPass() {
		if (useSwapchain) {
			/* Starting command buffer recording */
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
			beginInfo.pInheritanceInfo = nullptr; // Optional

			vkBeginCommandBuffer(VKDK::drawCmdBuffers[VKDK::swapIndex], &beginInfo);

			/* information about this particular render pass */
			VkRenderPassBeginInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = VKDK::renderPass;
			renderPassInfo.framebuffer = VKDK::swapChainFramebuffers[VKDK::swapIndex];
			renderPassInfo.renderArea.offset = { 0, 0 };
			renderPassInfo.renderArea.extent = VKDK::swapChainExtent;
			std::array<VkClearValue, 2> clearValues = {};
			clearValues[0].color = { clearColor.r, clearColor.g, clearColor.b, clearColor.a };
			clearValues[1].depthStencil = { 1.0f, 0 };

			renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
			renderPassInfo.pClearValues = clearValues.data();

			/* Start the render pass */
			vkCmdBeginRenderPass(VKDK::drawCmdBuffers[VKDK::swapIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		}
	}

	void endRecordingRenderPass() {
			/* End this render pass */
		if (useSwapchain) {
			vkCmdEndRenderPass(VKDK::drawCmdBuffers[VKDK::swapIndex]);

			if (vkEndCommandBuffer(VKDK::drawCmdBuffers[VKDK::swapIndex]) != VK_SUCCESS) {
				throw std::runtime_error("failed to record command buffer!");
			}

			//if (subScenes.size() == 0) {
			//}
			//else {
			//	throw std::runtime_error("TODO: determine semaphore information for hierarchy!");
			//}
		}
	}

	VkCommandBuffer getCommandBuffer() {
		return commandBuffer;
	}

	VkRenderPass getRenderPass() {
		return renderpass;
	}

	VkBuffer getCameraBuffer() {
		return cameraBuffer;
	}

	VkBuffer getLightBuffer() {
		return lightBuffer;
	}

	struct SubmitToGraphicsQueueInfo {
		std::vector<VkSemaphore> waitSemaphores;
		VkPipelineStageFlags submitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		std::vector<VkCommandBuffer> commandBuffers;
		std::vector<VkSemaphore> signalSemaphores;
		VkQueue graphicsQueue;
	};
	void submitToGraphicsQueue(SubmitToGraphicsQueueInfo &submitToGraphicsQueueInfo) {
		/* Submit command buffer to graphics queue for rendering */
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.waitSemaphoreCount = submitToGraphicsQueueInfo.waitSemaphores.size();
		submitInfo.pWaitSemaphores = submitToGraphicsQueueInfo.waitSemaphores.data();
		submitInfo.pWaitDstStageMask = &submitToGraphicsQueueInfo.submitPipelineStages;
		submitInfo.commandBufferCount = submitToGraphicsQueueInfo.commandBuffers.size();
		submitInfo.pCommandBuffers = submitToGraphicsQueueInfo.commandBuffers.data();
		submitInfo.signalSemaphoreCount = submitToGraphicsQueueInfo.signalSemaphores.size();
		submitInfo.pSignalSemaphores = submitToGraphicsQueueInfo.signalSemaphores.data();

		if (vkQueueSubmit(submitToGraphicsQueueInfo.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}
	}
};