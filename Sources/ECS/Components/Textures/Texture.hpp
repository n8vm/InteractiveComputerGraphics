// ┌──────────────────────────────────────────────────────────────────┐
// │ Developer : n8vm                                                 |
// │   TextureInterface: Inheret and implement all virtual functions  |
// |     to create a texture usable by material components.           |
// |                                                                  |
// |   Texture: A component wrapper for a texture implementing        |
// |     the TextureInterface class.                                  |
// └──────────────────────────────────────────────────────────────────┘

#pragma once
#include "vkdk.hpp"
#include "stb_image.h"
#include "Components/Component.hpp"

namespace Components::Textures {
	class TextureInterface {
	public:
    virtual void cleanup() {
      /* Destroy frame buffer */
      if (framebuffer) vkDestroyFramebuffer(VKDK::device, framebuffer, nullptr);

      /* Destroy samplers */
      if (colorSampler) vkDestroySampler(VKDK::device, colorSampler, nullptr);
      if (depthSampler) vkDestroySampler(VKDK::device, depthSampler, nullptr);

      /* Destroy Image Views */
      if (colorImageView) vkDestroyImageView(VKDK::device, colorImageView, nullptr);
      if (depthImageView) vkDestroyImageView(VKDK::device, depthImageView, nullptr);

      /* Destroy Images */
      if (colorImage) vkDestroyImage(VKDK::device, colorImage, nullptr);
      if (depthImage) vkDestroyImage(VKDK::device, depthImage, nullptr);

      /* Free Memory */
      if (colorImageMemory) vkFreeMemory(VKDK::device, colorImageMemory, nullptr);
      if (depthImageMemory) vkFreeMemory(VKDK::device, depthImageMemory, nullptr);
    };
		virtual VkImageView getDepthImageView() { return depthImageView; };
		virtual VkImageView getColorImageView() { return colorImageView; };
    virtual VkFramebuffer getFramebuffer() { return framebuffer; };
    virtual VkSampler getColorSampler() { return colorSampler; };
    virtual VkSampler getDepthSampler() { return depthSampler; };
    virtual VkImageLayout getColorImageLayout() { return colorImageLayout; };
    virtual VkImageLayout getDepthImageLayout() { return depthImageLayout; };
    virtual VkImage getColorImage() { return colorImage; };
    virtual VkImage getDepthImage() { return depthImage; };
		virtual uint32_t getColorMipLevels() { return colorMipLevels; };
    virtual VkFormat getColorFormat() { return colorFormat; };
    virtual VkFormat getDepthFormat() { return depthFormat; };
    virtual uint32_t getWidth() { return width; }
    virtual uint32_t getHeight() { return height; }
    virtual uint32_t getDepth() { return depth; }
    virtual uint32_t getTotalLayers() { return 1; }

		// Create an image memory barrier for changing the layout of
		// an image and put it into an active command buffer
		// See chapter 11.4 "Image Layout" for details
		void setImageLayout(
			VkCommandBuffer cmdbuffer,
			VkImage image,
			VkImageLayout oldImageLayout,
			VkImageLayout newImageLayout,
			VkImageSubresourceRange subresourceRange,
			VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT)
		{
			// Create an image barrier object
			VkImageMemoryBarrier imageMemoryBarrier = vks::initializers::imageMemoryBarrier();
			imageMemoryBarrier.oldLayout = oldImageLayout;
			imageMemoryBarrier.newLayout = newImageLayout;
			imageMemoryBarrier.image = image;
			imageMemoryBarrier.subresourceRange = subresourceRange;

			// Source layouts (old)
			// Source access mask controls actions that have to be finished on the old layout
			// before it will be transitioned to the new layout
			switch (oldImageLayout)
			{
			case VK_IMAGE_LAYOUT_UNDEFINED:
				// Image layout is undefined (or does not matter)
				// Only valid as initial layout
				// No flags required, listed only for completeness
				imageMemoryBarrier.srcAccessMask = 0;
				break;

			case VK_IMAGE_LAYOUT_PREINITIALIZED:
				// Image is preinitialized
				// Only valid as initial layout for linear images, preserves memory contents
				// Make sure host writes have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				// Image is a color attachment
				// Make sure any writes to the color buffer have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				// Image is a depth/stencil attachment
				// Make sure any writes to the depth/stencil buffer have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				// Image is a transfer source 
				// Make sure any reads from the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				// Image is a transfer destination
				// Make sure any writes to the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				// Image is read by a shader
				// Make sure any shader reads from the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;
			default:
				// Other source layouts aren't handled (yet)
				break;
			}

			// Target layouts (new)
			// Destination access mask controls the dependency for the new image layout
			switch (newImageLayout)
			{
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				// Image will be used as a transfer destination
				// Make sure any writes to the image have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				// Image will be used as a transfer source
				// Make sure any reads from the image have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				// Image will be used as a color attachment
				// Make sure any writes to the color buffer have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				// Image layout will be used as a depth/stencil attachment
				// Make sure any writes to depth/stencil buffer have been finished
				imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				// Image will be read in a shader (sampler, input attachment)
				// Make sure any writes to the image have been finished
				if (imageMemoryBarrier.srcAccessMask == 0)
				{
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
				}
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;
			default:
				// Other source layouts aren't handled (yet)
				break;
			}

			// Put barrier inside setup command buffer
			vkCmdPipelineBarrier(
				cmdbuffer,
				srcStageMask,
				dstStageMask,
				0,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);
		}

		// Fixed sub resource on first mip level and layer
		void setImageLayout(
			VkCommandBuffer cmdbuffer,
			VkImage image,
			VkImageAspectFlags aspectMask,
			VkImageLayout oldImageLayout,
			VkImageLayout newImageLayout,
			VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT)
		{
			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = aspectMask;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = 1;
			subresourceRange.layerCount = 1;
			setImageLayout(cmdbuffer, image, oldImageLayout, newImageLayout, subresourceRange, srcStageMask, dstStageMask);
		}

    void createColorImageView() {
      // Create image view
      VkImageViewCreateInfo view = vks::initializers::imageViewCreateInfo();
      // Cube map view type
      view.viewType = viewType;
      view.format = colorFormat;
      view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
      view.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
      // 6 array layers (faces)
      view.subresourceRange.layerCount = layers;
      // Set number of mip levels
      view.subresourceRange.levelCount = colorMipLevels;
      view.image = colorImage;
      VK_CHECK_RESULT(vkCreateImageView(VKDK::device, &view, nullptr, &colorImageView));
    }

  protected:
    VkImage colorImage = VK_NULL_HANDLE, depthImage = VK_NULL_HANDLE;
    VkFormat colorFormat = VK_FORMAT_UNDEFINED, depthFormat = VK_FORMAT_UNDEFINED;
    VkDeviceMemory colorImageMemory = VK_NULL_HANDLE, depthImageMemory= VK_NULL_HANDLE;
    VkImageView colorImageView = VK_NULL_HANDLE, depthImageView = VK_NULL_HANDLE;
    VkImageLayout colorImageLayout = VK_IMAGE_LAYOUT_UNDEFINED, depthImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkSampler colorSampler = VK_NULL_HANDLE, depthSampler = VK_NULL_HANDLE;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    uint32_t width = 1, height = 1, depth = 1, colorMipLevels = 1, layers = 1;
    VkImageViewType viewType;
  };

	class Texture : public Component {
	public:
    std::shared_ptr<TextureInterface> texture;
		void cleanup() {
			texture->cleanup();
		}
	};
}
