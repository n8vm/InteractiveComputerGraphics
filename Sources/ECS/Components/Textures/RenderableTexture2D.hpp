#pragma once
#include "vkdk.hpp"
#include "Texture2D.hpp"
#include <assert.h>

#include "Systems/ComponentManager.hpp"

namespace Components::Textures {
	class RenderableTexture2D : public TextureInterface {
  private:
    VkRenderPass renderPass;

	public:
		static std::shared_ptr<Texture>Create(std::string name, int width, int height, VkRenderPass &renderPass) 
		{
			std::cout << "ComponentManager: Adding RenderableTexture2D \"" << name << "\"" << std::endl;

			auto tex = std::make_shared<Texture>();
			auto rt2d = std::make_shared<RenderableTexture2D>(width, height, renderPass);
			tex->texture = rt2d;
			Systems::ComponentManager::Textures[name] = tex;
			return tex;
		}

		/* Constructors */
		RenderableTexture2D(int width, int height, VkRenderPass &renderPass) {
			/* Save the image/framebuffer width and height. */
			this->width = width;
			this->height = height;
      this->layers = 1;
      this->colorMipLevels = 1;
      this->viewType = VK_IMAGE_VIEW_TYPE_2D;
			this->renderPass = renderPass;

			createColorImageResources();
			createDepthStencilResources();
			createFrameBuffer();
		}

		void createColorImageResources() {
			colorFormat = VK_FORMAT_R8G8B8A8_UNORM;

			VkImageCreateInfo imageInfo = {};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.format = colorFormat; // Could be parameterized...
			imageInfo.extent.width = width;
			imageInfo.extent.height = height;
			imageInfo.extent.depth = depth;
			imageInfo.mipLevels = colorMipLevels;
			imageInfo.arrayLayers = layers;
			imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;

			/* We will sample directly from the color attachment*/
			imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

			VkMemoryAllocateInfo memAlloc = {};
			memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			VkMemoryRequirements memReqs;

			VK_CHECK_RESULT(vkCreateImage(VKDK::device, &imageInfo, nullptr, &colorImage));
			vkGetImageMemoryRequirements(VKDK::device, colorImage, &memReqs);
			memAlloc.allocationSize = memReqs.size;
			memAlloc.memoryTypeIndex = VKDK::FindMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT); // this might not be right
			
			/* Now create memory for that image */
			VK_CHECK_RESULT(vkAllocateMemory(VKDK::device, &memAlloc, nullptr, &colorImageMemory));
			VK_CHECK_RESULT(vkBindImageMemory(VKDK::device, colorImage, colorImageMemory, 0));

			/* Create the image view */
			VkImageViewCreateInfo colorImageViewInfo = {};
			colorImageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			colorImageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			colorImageViewInfo.format = colorFormat; // could be parameterize;
			colorImageViewInfo.subresourceRange = {};
			colorImageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			colorImageViewInfo.subresourceRange.baseMipLevel = 0;
			colorImageViewInfo.subresourceRange.levelCount = 1;
			colorImageViewInfo.subresourceRange.baseArrayLayer = 0;
			colorImageViewInfo.subresourceRange.layerCount = 1;
			colorImageViewInfo.image = colorImage;
			VK_CHECK_RESULT(vkCreateImageView(VKDK::device, &colorImageViewInfo, nullptr, &colorImageView));

			/* Create a sampler to sample from the attachment in the fragment shader */
			VkSamplerCreateInfo samplerInfo = {};
			samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			samplerInfo.magFilter = VK_FILTER_NEAREST;
			samplerInfo.minFilter = VK_FILTER_NEAREST;
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			samplerInfo.mipLodBias = 0.0;
			samplerInfo.maxAnisotropy = 1.0;
			samplerInfo.minLod = 0.0;
			samplerInfo.maxLod = 1.0;
			samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
			VK_CHECK_RESULT(vkCreateSampler(VKDK::device, &samplerInfo, nullptr, &colorSampler));
		}

		void createDepthStencilResources() {
      // Find a suitable depth format
      VkBool32 validDepthFormat = vks::tools::getSupportedDepthFormat(VKDK::physicalDevice, &depthFormat);
      assert(validDepthFormat);

			/* Create depth image */
			VkImageCreateInfo imageInfo = {};
			imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageInfo.imageType = VK_IMAGE_TYPE_2D;
			imageInfo.format = depthFormat;
			imageInfo.extent.width = width;
			imageInfo.extent.height = height;
			imageInfo.extent.depth = 1;
			imageInfo.mipLevels = 1;
			imageInfo.arrayLayers = 1;
			imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;

			/* I want to be able to sample from this depth image */
			imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			VK_CHECK_RESULT(vkCreateImage(VKDK::device, &imageInfo, nullptr, &depthImage));

			VkMemoryAllocateInfo memAlloc = {};
			memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			VkMemoryRequirements memReqs;

			/* Bind memory to depth image */
			vkGetImageMemoryRequirements(VKDK::device, depthImage, &memReqs);
			memAlloc.allocationSize = memReqs.size;
			memAlloc.memoryTypeIndex = VKDK::FindMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT); // this also may not be right
			VK_CHECK_RESULT(vkAllocateMemory(VKDK::device, &memAlloc, nullptr, &depthImageMemory));
			VK_CHECK_RESULT(vkBindImageMemory(VKDK::device, depthImage, depthImageMemory, 0));

			/* Create image view */
			VkImageViewCreateInfo depthStencilViewInfo = {};
			depthStencilViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			depthStencilViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			depthStencilViewInfo.format = depthFormat;
			depthStencilViewInfo.flags = 0;
			depthStencilViewInfo.subresourceRange = {};
			depthStencilViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT /*| VK_IMAGE_ASPECT_STENCIL_BIT*/;
			depthStencilViewInfo.subresourceRange.baseMipLevel = 0;
			depthStencilViewInfo.subresourceRange.levelCount = 1;
			depthStencilViewInfo.subresourceRange.baseArrayLayer = 0;
			depthStencilViewInfo.subresourceRange.layerCount = 1;
			depthStencilViewInfo.image = depthImage;
			VK_CHECK_RESULT(vkCreateImageView(VKDK::device, &depthStencilViewInfo, nullptr, &depthImageView));
		}

		void createFrameBuffer() {
			VkImageView attachments[2];
			attachments[0] = colorImageView;
			attachments[1] = depthImageView;

			VkFramebufferCreateInfo fbufCreateInfo = vks::initializers::framebufferCreateInfo();
			fbufCreateInfo.renderPass = renderPass;
			fbufCreateInfo.attachmentCount = 2;
			fbufCreateInfo.pAttachments = attachments;
			fbufCreateInfo.width = width;
			fbufCreateInfo.height = height;
			fbufCreateInfo.layers = 1;

			VK_CHECK_RESULT(vkCreateFramebuffer(VKDK::device, &fbufCreateInfo, nullptr, &framebuffer));
		}
	};
}