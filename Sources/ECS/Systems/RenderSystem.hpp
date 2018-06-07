#pragma once

#include "System/ComponentManager.hpp"

namespace Systems {
	class RenderSystem {
	public:
		struct RenderItem {
			VkDescriptorSet descriptorSet;
			std::shared_ptr<Components::Meshes::Mesh> mesh;
			std::shared_ptr<Components::Materials::Material> material;
		};

		void addToQueue(VkRenderPass renderPass, std::shared_ptr<Components::Meshes::Mesh> mesh, VkDescriptorSet descriptorSet) {

		}

		void record(VkRenderPass renderPass, VkCommandBuffer commandBuffer, glm::vec4 clearColor = glm::vec4(0.0), float clearDepth = 1.0f, uint32_t clearStencil = 0) {
			/* For each perspective */
			for (auto perspective : ComponentManager::PerspectiveList) {
				VkCommandBufferBeginInfo beginInfo = {};
				beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
				beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
				beginInfo.pInheritanceInfo = nullptr; // Optional
				vkBeginCommandBuffer(commandBuffer, &beginInfo);

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

				/* Render the scene */
				//scene->render(commandBuffers[i], renderpass);

				/* End the render pass */
				vkCmdEndRenderPass(commandBuffers[i]);
				VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffers[i]));


				// For each render pass
				// For each entry in this renderpass's queue
				// Render that entry

				//for (int i = 0; i < perspectives.size(); ++i) {
				//	perspectives[i].recordRenderPass(shared_from_this(), clearColor, clearDepth, clearStencil);
				//}
			}
		}

	private:
		std::vector<RenderItem> queue;
	};
};