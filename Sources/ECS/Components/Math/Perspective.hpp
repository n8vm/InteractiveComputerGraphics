#pragma once 

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif 

#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

#include "vkdk.hpp"
#include <glm/glm.hpp>
#include <glm/common.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

#include "Systems/ComponentManager.hpp"
#include "Components/Textures/RenderableTexture2D.hpp"
#include "Components/Textures/RenderableTextureCube.hpp"
#include "Components/Component.hpp"

#include <array>

namespace Entities { class Entity; }

namespace Components::Math {
	struct PerspectiveObject {
		glm::mat4 View;
		glm::mat4 Projection;
		glm::mat4 ViewInverse;
		glm::mat4 ProjectionInverse;
		float nearPos;
    float farPos;
    float pad1, pad2;
	};
#define MAX_MULTIVIEW 6
  struct PerspectiveBufferObject {
    PerspectiveObject Perspectives[MAX_MULTIVIEW];
  };
	
	class Perspective : public Component {
	public:
		float nearPos = .01f;
		float farPos = 1000.f;
		float fov = 90.f;
		glm::mat4 views[MAX_MULTIVIEW];
		glm::mat4 projections[MAX_MULTIVIEW];

		std::function<void(VkCommandBuffer)> preRenderPassCallback;

		bool canRender = false;

		/* If a perspective uses the swapchain, it'll use VKDK properties to render directly to the swapchain images.
			Otherwise, a perspective will render to it's own texture with its own render pass and command buffer.
		*/
		bool useSwapchain;

		VkRenderPass renderpass;
		std::vector<VkCommandBuffer> commandBuffers;
		std::vector<VkFramebuffer> frameBuffers;
		uint32_t framebufferWidth, framebufferHeight;
		VkBuffer perspectiveUBO;
		VkDeviceMemory perspectiveUBOMemory;
		std::shared_ptr<Components::Textures::Texture> renderTexture = nullptr;

	public:
		static std::shared_ptr<Perspective> Create(
      std::string name, VkRenderPass renderpass, 
      std::vector<VkCommandBuffer> commandBuffers, 
      std::vector<VkFramebuffer> frameBuffers, 
      int framebufferWidth, int framebufferHeight) {
			std::cout << "ComponentManager: Adding Perspective \"" << name << "\"" << std::endl;
			auto perspective = std::make_shared<Perspective>(renderpass, commandBuffers, frameBuffers, framebufferWidth, framebufferHeight);
			Systems::ComponentManager::Perspectives[name] = perspective;
			return perspective;
		}

    /* Generates a 2D or Cube texture */
		static std::shared_ptr<Perspective> Create(
      std::string name, uint32_t framebufferWidth, uint32_t framebufferHeight, bool cubemap = false) {
			std::cout << "ComponentManager: Adding Perspective \"" << name << "\"" << std::endl;
			auto perspective = std::make_shared<Perspective>(name, framebufferWidth, framebufferHeight, cubemap);
			Systems::ComponentManager::Perspectives[name] = perspective;
			return perspective;
		}

		/* This constructor will generate a renderpass and command buffer on its own, and will render to a 2D texture */
		Perspective(std::string textureName, uint32_t framebufferWidth, uint32_t framebufferHeight, bool cubemap) {
			this->framebufferWidth = framebufferWidth;
			this->framebufferHeight = framebufferHeight;
			createRenderPass(framebufferWidth, framebufferHeight, (cubemap) ? 6 : 1);
			createCommandBuffer();
			createUniformBuffer();
      if (cubemap) {
        renderTexture = Components::Textures::RenderableTextureCube::Create(
          textureName, framebufferWidth, framebufferHeight, renderpass);
      }
      else {
        renderTexture = Components::Textures::RenderableTexture2D::Create(
          textureName, framebufferWidth, framebufferHeight, renderpass);
      }
			frameBuffers.push_back(renderTexture->texture->getFramebuffer());
			canRender = true;
		}

		/* This constructor will use a preconstructed renderpass, command buffer, framebuffer, and extent */
		Perspective(VkRenderPass renderpass, std::vector<VkCommandBuffer> commandBuffers, std::vector<VkFramebuffer> frameBuffers, int framebufferWidth, int framebufferHeight) {
			this->framebufferWidth = framebufferWidth;
			this->framebufferHeight = framebufferHeight;
			this->renderpass = renderpass;
			this->commandBuffers = commandBuffers;
			this->frameBuffers = frameBuffers;

			createUniformBuffer();
			canRender = true;
			useSwapchain = true;
		}

		/* This constructor just creates a uniform buffer, but assumes render pass information will be provided later. */
		Perspective() {
			createUniformBuffer();
		}

		void refresh(VkRenderPass renderpass, std::vector<VkCommandBuffer> commandBuffers, std::vector<VkFramebuffer> frameBuffers, int framebufferWidth, int framebufferHeight) {
			this->framebufferWidth = framebufferWidth;
			this->framebufferHeight = framebufferHeight;
			this->renderpass = renderpass;
			this->commandBuffers = commandBuffers;
			this->frameBuffers = frameBuffers;
			canRender = true;
			useSwapchain = true;
		}
		
		void createRenderPass(uint32_t framebufferWidth, uint32_t framebufferHeight, int layers = 1) {
			using namespace VKDK;

			// Create a separate render pass for the offscreen rendering as it may differ from the one used for scene rendering
			std::array<VkAttachmentDescription, 2> attchmentDescriptions = {};

			// Color attachment
			attchmentDescriptions[0].format = VK_FORMAT_R8G8B8A8_UNORM;
			attchmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
			attchmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attchmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			attchmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			attchmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			attchmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			attchmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			// Depth attachment
      VkFormat depthFormat; // Potential bug here...
      VkBool32 validDepthFormat = vks::tools::getSupportedDepthFormat(VKDK::physicalDevice, &depthFormat);
      assert(validDepthFormat);
			attchmentDescriptions[1].format = depthFormat;
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

      uint32_t mask = 0;
      for (int i = 0; i < layers; ++i) 
        mask |= 1 << i;

      // Support for multiview
      const uint32_t viewMasks[] = { mask };
      //const int32_t viewOffsets[] = { 1 };
      const uint32_t correlationMasks[] = { mask };
      
      VkRenderPassMultiviewCreateInfo renderPassMultiviewInfo = {};
      renderPassMultiviewInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO;
      renderPassMultiviewInfo.subpassCount = 1;
      renderPassMultiviewInfo.pViewMasks = viewMasks;
      renderPassMultiviewInfo.dependencyCount = 0;
      renderPassMultiviewInfo.pViewOffsets = NULL;// viewOffsets;
      renderPassMultiviewInfo.correlationMaskCount = 1;
      renderPassMultiviewInfo.pCorrelationMasks = correlationMasks;

			// Create the actual renderpass
			VkRenderPassCreateInfo renderPassInfo = {};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.attachmentCount = static_cast<uint32_t>(attchmentDescriptions.size());
			renderPassInfo.pAttachments = attchmentDescriptions.data();
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpassDescription;
			renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
			renderPassInfo.pDependencies = dependencies.data();
      renderPassInfo.pNext = &renderPassMultiviewInfo;
      

			VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderpass));
			useSwapchain = false;
		}

		void createCommandBuffer() {
			/* For convenience, also create an offscreen command buffer */
			VkCommandBufferAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfo.commandPool = VKDK::commandPool;
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocInfo.commandBufferCount = 1;
			VkCommandBuffer commandBuffer;

			if (vkAllocateCommandBuffers(VKDK::device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
				throw std::runtime_error("failed to allocate offscreen command buffer!");
			}

			commandBuffers.push_back(commandBuffer);
		}

		void createUniformBuffer() {
			VkDeviceSize bufferSize = sizeof(PerspectiveBufferObject);
			VKDK::CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, perspectiveUBO, perspectiveUBOMemory);
		}

		void recordRenderPass(/*std::shared_ptr<Entities::Entity> scene, */glm::vec4 clearColor, float clearDepth = 1.0f, uint32_t clearStencil = 0);

		void uploadUBO() {
			/* Update uniform buffer */
			PerspectiveBufferObject pbo = {};
      for (int i = 0; i < MAX_MULTIVIEW; ++i) {
			  pbo.Perspectives[i].View = views[i];
			  pbo.Perspectives[i].ViewInverse = glm::inverse(pbo.Perspectives[0].View);
			  pbo.Perspectives[i].Projection = projections[i];
			  pbo.Perspectives[i].Projection[1][1] *= -1; // required so that image doesn't flip upside down.
			  pbo.Perspectives[i].ProjectionInverse = glm::inverse(projections[i]); // Todo: account for -1 here...
			  pbo.Perspectives[i].nearPos = getNear();
			  pbo.Perspectives[i].farPos = getFar();
      }

			/* Map uniform buffer, copy data directly, then unmap */
			void* data;
			vkMapMemory(VKDK::device, perspectiveUBOMemory, 0, sizeof(pbo), 0, &data);
			memcpy(data, &pbo, sizeof(pbo));
			vkUnmapMemory(VKDK::device, perspectiveUBOMemory);
		}

		void cleanup() {
			vkDestroyBuffer(VKDK::device, perspectiveUBO, nullptr);
			vkFreeMemory(VKDK::device, perspectiveUBOMemory, nullptr);

			if (useSwapchain) return;

			vkDestroyRenderPass(VKDK::device, renderpass, nullptr);
		}

		VkBuffer getUBO() {
			return perspectiveUBO;
		}

		float getNear() {
			return nearPos;
		}

		float getFar() {
			return farPos;
		}
	};
}