#include "Scene.hpp"
#include "Entities/Entity.hpp"
#include "Entities/Cameras/Camera.hpp"
#include "Entities/Lights/Light.hpp"
#include "Components/Textures/RenderableTexture2D.hpp"
#include "System.hpp"
#include "Entities/Lights/LightBufferObject.hpp"
#include "Entities/Cameras/CameraBufferObject.hpp"


Scene::Scene(bool useSwapchain, std::string renderTextureName, int framebufferWidth, int framebufferHeight) {
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

void Scene::createRenderPass(std::string renderTextureName, uint32_t framebufferWidth, uint32_t framebufferHeight) {
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
	attchmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
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
	renderTexture = std::make_shared<Components::Textures::RenderableTexture2D>(framebufferWidth, framebufferHeight, renderpass);
	System::TextureList[renderTextureName] = renderTexture;
}

void Scene::createCommandBuffer() {
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

void Scene::createLightUniformBuffer() {
	VkDeviceSize bufferSize = sizeof(LightBufferObject);
	VKDK::CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, lightBuffer, lightBufferMemory);
}

void Scene::createCameraUniformBuffer() {
	VkDeviceSize bufferSize = sizeof(CameraBufferObject);
	VKDK::CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, cameraBuffer, cameraBufferMemory);
}

void Scene::setPerspective(Components::Perspectives newPerspective) {
	perspective = newPerspective;
}


void Scene::createGlobalUniformBuffers() {
	createLightUniformBuffer();
	createCameraUniformBuffer();
}

void Scene::cleanup() {
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

void Scene::updateLightBuffer() {
	LightBufferObject lbo = {};

	/* To do: fill light buffer object here */
	lbo.numLights = LightList.size();
	int i = 0;
	for (auto light : LightList) {
		lbo.lights[i].position = glm::vec4(light.second->transform.GetPosition(), 1.0);
		lbo.lights[i].ambient = glm::vec4(light.second->getAmbientColor(), 1.0);
		lbo.lights[i].diffuse = glm::vec4(light.second->getDiffuseColor(), 1.0);
		lbo.lights[i].specular = glm::vec4(light.second->getSpecularColor(), 1.0);
		lbo.lights[i].model = glm::mat4(1.0);
		lbo.lights[i].view = light.second->transform.ParentToLocalMatrix();
		lbo.lights[i].fproj = light.second->transform.ForwardProjection();
		lbo.lights[i].fproj[1][1] *= -1;// required so that image doesn't flip upside down.
		lbo.lights[i].bproj = light.second->transform.BackwardProjection();
		lbo.lights[i].bproj[1][1] *= -1;// required so that image doesn't flip upside down.
		lbo.lights[i].uproj = light.second->transform.TopProjection();
		lbo.lights[i].uproj[1][1] *= -1;// required so that image doesn't flip upside down.
		lbo.lights[i].dproj = light.second->transform.BottomProjection();
		lbo.lights[i].dproj[1][1] *= -1;// required so that image doesn't flip upside down.
		lbo.lights[i].lproj = light.second->transform.LeftProjection();
		lbo.lights[i].lproj[1][1] *= -1;// required so that image doesn't flip upside down.
		lbo.lights[i].rproj = light.second->transform.RightProjection();
		lbo.lights[i].rproj[1][1] *= -1;// required so that image doesn't flip upside down.
		++i;
	}

	/* Map uniform buffer, copy data directly, then unmap */
	void* data;
	vkMapMemory(VKDK::device, lightBufferMemory, 0, sizeof(lbo), 0, &data);
	memcpy(data, &lbo, sizeof(lbo));
	vkUnmapMemory(VKDK::device, lightBufferMemory);
}

void Scene::updateCameraBuffer() {
	if (!camera) return;

	/* Update uniform buffer */
	CameraBufferObject cbo = {};
	
	glm::mat4 projection = glm::mat4(1.0);

	switch (perspective) {
	case Components::Perspectives::Forward:
		projection = camera->transform.ForwardProjection(); break;
	case Components::Perspectives::Backward:
		projection = camera->transform.BackwardProjection(); break;
	case Components::Perspectives::Left:
		projection = camera->transform.LeftProjection(); break;
	case Components::Perspectives::Right:
		projection = camera->transform.RightProjection(); break;
	case Components::Perspectives::Top:
		projection = camera->transform.TopProjection(); break;
	case Components::Perspectives::Bottom:
		projection = camera->transform.BottomProjection(); break;
	}

	/* To do: fill camera buffer object here */
	cbo.View = camera->transform.ParentToLocalMatrix() * SceneTransform;
	cbo.Projection = projection;
	cbo.Projection[1][1] *= -1; // required so that image doesn't flip upside down.
	cbo.ProjectionInverse = glm::inverse(projection); // Todo: account for -1 here...
	cbo.ViewInverse = glm::inverse(camera->transform.ParentToLocalMatrix() * SceneTransform);
	cbo.nearPos = camera->transform.getNear();
	cbo.farPos = camera->transform.getFar();

	/* Map uniform buffer, copy data directly, then unmap */
	void* data;
	vkMapMemory(VKDK::device, cameraBufferMemory, 0, sizeof(cbo), 0, &data);
	memcpy(data, &cbo, sizeof(cbo));
	vkUnmapMemory(VKDK::device, cameraBufferMemory);
}

void Scene::updateCameraUBO() {
	///* First, update all sub scenes*/
	//for (auto i : subScenes) {
	//	i.second->update();
	//}


}

void Scene::update() {
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

	if (camera)
		entities->update(this, glm::mat4(1.0), camera->transform.ParentToLocalMatrix() * SceneTransform, camera->transform.ForwardProjection());
};

void Scene::setClearColor(glm::vec4 clearColor) {
	this->clearColor = clearColor;
}

void Scene::recordRenderPass() {
	/* First, render all subscenes*/
	for (auto i : subScenes) {
		i.second->recordRenderPass();
	}

	/* Update VP matrix */

	/* Todo, update this to account for updated view stuff*/
	//if (camera)
	//camera->setWindowSize(VKDK::swapChainExtent.width, VKDK::swapChainExtent.height);

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
		/* Todo: account for this */
		//camera->setWindowSize(renderTexture->getWidth(), renderTexture->getHeight());

		/* Record rendering of the scene */
		entities->render(this);

		/* End offscreen render */
		vkCmdEndRenderPass(commandBuffer);
		VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
	}
}

void Scene::beginRecordingRenderPass() {
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

void Scene::endRecordingRenderPass() {
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

VkCommandBuffer Scene::getCommandBuffer() {
	return commandBuffer;
}

VkRenderPass Scene::getRenderPass() {
	return renderpass;
}

VkBuffer Scene::getCameraBuffer() {
	return cameraBuffer;
}

VkBuffer Scene::getLightBuffer() {
	return lightBuffer;
}

void Scene::submitToGraphicsQueue(SubmitToGraphicsQueueInfo &submitToGraphicsQueueInfo) {
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