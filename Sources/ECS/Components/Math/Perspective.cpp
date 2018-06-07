#include "Perspective.hpp"

#include "Entities/Entity.hpp" 
#include "Components/Materials/Material.hpp"
#include "Components/Materials/Materials.hpp"
#include "Components/Meshes/Meshes.hpp"
#include "Components/Lights/PointLight/PointLight.hpp"

void Components::Math::Perspective::recordRenderPass(glm::vec4 clearColor, float clearDepth, uint32_t clearStencil) {
	for (int i = 0; i < commandBuffers.size(); ++i) {
		/* Render to offscreen texture */
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		beginInfo.pInheritanceInfo = nullptr; // Optional
		vkBeginCommandBuffer(commandBuffers[i], &beginInfo);
    
    /* Information about this particular render pass */
    VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderpass;
		renderPassInfo.framebuffer = frameBuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = { framebufferWidth, framebufferHeight };
		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { clearColor.r, clearColor.g, clearColor.b, clearColor.a };
		clearValues[1].depthStencil = { clearDepth, clearStencil };
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		if (preRenderPassCallback) {
			preRenderPassCallback(commandBuffers[i]);
		}

		/* Start the render pass */
		vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		/* Set viewport*/
		VkViewport viewport{};
		viewport.width = (float)framebufferWidth;
		viewport.height = (float)framebufferHeight;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vkCmdSetViewport(commandBuffers[i], 0, 1, &viewport);

		/* Set Scissors */
		VkRect2D rect2D{};
		rect2D.extent.width = framebufferWidth;
		rect2D.extent.height = framebufferHeight;
		rect2D.offset.x = 0;
		rect2D.offset.y = 0;

		vkCmdSetScissor(commandBuffers[i], 0, 1, &rect2D);
		
		/* Todo: implement this */
		int totalRenderPasses = 1;

		/* For each subpass */
		for (int subpassIdx = 0; subpassIdx < totalRenderPasses; ++subpassIdx) {
			// TODO
			//if (subpassIdx != 0)
				//vkCmdNextSubpass(...)

			/* For each entity */
			for (auto pair : Systems::SceneGraph::Entities) {
				auto materialComponents = pair.second->getComponents<Components::Materials::Material>();
				auto meshComponent = pair.second->getFirstComponent<Components::Meshes::Mesh>();

				/* If an entity has all of the above components */
				if (materialComponents.size() > 0 && meshComponent) {
					for (int matIdx = 0; matIdx < materialComponents.size(); ++matIdx) {
						PipelineKey matPipelineKey = materialComponents[matIdx]->material->getPipelineKey();

						/* If the material's key doesnt match the current pass/subpass, continue. */
						if (matPipelineKey.renderpass != renderpass
							|| matPipelineKey.subpass != subpassIdx) continue;

						Components::Materials::UBOSet uboset = {};
						uboset.transformUBO = pair.second->transform->getUBO();
						uboset.perspectiveUBO = perspectiveUBO;
						uboset.pointLightUBO = Components::Lights::PointLights::GetUBO();
						VkDescriptorSet descriptor = materialComponents[matIdx]->material->getDescriptorSet(uboset);
						materialComponents[matIdx]->material->render(matPipelineKey, commandBuffers[i], descriptor, meshComponent);
					}
				}
			}
		}
		
		/* End the render pass */
		vkCmdEndRenderPass(commandBuffers[i]);
		VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffers[i]));
	}
}