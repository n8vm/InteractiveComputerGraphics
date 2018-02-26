#pragma once
#include "vkdk.hpp"
#include "Texture.hpp"
#include <assert.h>
#include <gli/gli.hpp>

namespace Components::Textures {
	class TextureCube : public Texture {
	public:

		/* Note: assume khronos texture format */
		TextureCube(std::string cubemapPath, VkFormat format) {
			if (format == VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK && !VKDK::deviceFeatures.textureCompressionETC2) {
				throw std::runtime_error("ETC2 texture compression not supported!");
			}
			else if (format == VK_FORMAT_ASTC_8x8_UNORM_BLOCK && !VKDK::deviceFeatures.textureCompressionASTC_LDR) {
				throw std::runtime_error("ASTC texture compression not supported!");
			} 
			else if (format == VK_FORMAT_BC2_UNORM_BLOCK && !VKDK::deviceFeatures.textureCompressionBC) {
				throw std::runtime_error("BC2 texture compression not supported!");
			}
			this->format = format;

			struct stat st;
			if (stat(cubemapPath.c_str(), &st) != 0)
				throw std::runtime_error(cubemapPath + " does not exist!");

			createTextureImageKTX(cubemapPath);
			createImageSampler();
			createImageView();
			
		}

		void createTextureImageKTX(std::string cubemapPath) {
			/* Load the texture */
			gli::texture_cube texCube(gli::load(cubemapPath));
			assert(!texCube.empty());

			width = texCube.extent().x;
			height = texCube.extent().y;
			mipLevels = texCube.levels();

			VkMemoryAllocateInfo memAllocInfo = {};
			memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			VkMemoryRequirements memReqs;

			// Create a host-visible staging buffer that contains the raw image data
			VkBuffer stagingBuffer;
			VkDeviceMemory stagingBufferMemory;

			VkBufferCreateInfo bufferCreateInfo = {};
			bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferCreateInfo.size = texCube.size();
			bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			VK_CHECK_RESULT(vkCreateBuffer(VKDK::device, &bufferCreateInfo, nullptr, &stagingBuffer));

			// Get memory requirements for the staging buffer (alignment, memory type bits)
			vkGetBufferMemoryRequirements(VKDK::device, stagingBuffer, &memReqs);
			memAllocInfo.allocationSize = memReqs.size;
			// Get memory type index for a host visible buffer
			memAllocInfo.memoryTypeIndex = VKDK::FindMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			VK_CHECK_RESULT(vkAllocateMemory(VKDK::device, &memAllocInfo, nullptr, &stagingBufferMemory));
			VK_CHECK_RESULT(vkBindBufferMemory(VKDK::device, stagingBuffer, stagingBufferMemory, 0));

			// Copy texture data into staging buffer
			uint8_t *data;
			VK_CHECK_RESULT(vkMapMemory(VKDK::device, stagingBufferMemory, 0, memReqs.size, 0, (void **)&data));
			memcpy(data, texCube.data(), texCube.size());
			vkUnmapMemory(VKDK::device, stagingBufferMemory);

			// Create optimal tiled target image
			VkImageCreateInfo imageCreateInfo = {};
			imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
			imageCreateInfo.format = format;
			imageCreateInfo.mipLevels = mipLevels;
			imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
			imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageCreateInfo.extent = { width, height, 1 };
			imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			// Cube faces count as array layers in Vulkan
			imageCreateInfo.arrayLayers = 6;
			// This flag is required for cube map images
			imageCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
			VK_CHECK_RESULT(vkCreateImage(VKDK::device, &imageCreateInfo, nullptr, &textureImage));

			vkGetImageMemoryRequirements(VKDK::device, textureImage, &memReqs);
			memAllocInfo.allocationSize = memReqs.size;
			memAllocInfo.memoryTypeIndex = VKDK::FindMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			VK_CHECK_RESULT(vkAllocateMemory(VKDK::device, &memAllocInfo, nullptr, &textureImageMemory));
			VK_CHECK_RESULT(vkBindImageMemory(VKDK::device, textureImage, textureImageMemory, 0));

			// Setup buffer copy regions for each face including all of it's miplevels
			VkCommandBuffer copyCmd = VKDK::CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
			std::vector<VkBufferImageCopy> bufferCopyRegions;
			uint32_t offset = 0;

			for (int face = 0; face < 6; ++face) {
				for (int level = 0; level < mipLevels; ++level) {
					VkBufferImageCopy bufferCopyRegion = {};
					bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					bufferCopyRegion.imageSubresource.mipLevel = level;
					bufferCopyRegion.imageSubresource.baseArrayLayer = face;
					bufferCopyRegion.imageSubresource.layerCount = 1;
					bufferCopyRegion.imageExtent.width = texCube[face][level].extent().x;
					bufferCopyRegion.imageExtent.height = texCube[face][level].extent().y;
					bufferCopyRegion.imageExtent.depth = 1;
					bufferCopyRegion.bufferOffset = offset;

					bufferCopyRegions.push_back(bufferCopyRegion);

					// Increase offset into staging buffer for next level / face
					offset += texCube[face][level].size();
				}
			}

			// Image barrier for optimal image (target)
			// Set initial layout for all array layers (faces) of the optimal (target) tiled texture
			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = mipLevels;
			subresourceRange.layerCount = 6;

			setImageLayout(
				copyCmd,
				textureImage,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				subresourceRange);

			// Copy the cube map faces from the staging buffer to the optimal tiled image
			vkCmdCopyBufferToImage(
				copyCmd,
				stagingBuffer,
				textureImage,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				static_cast<uint32_t>(bufferCopyRegions.size()),
				bufferCopyRegions.data()
			);

			// Change texture image layout to shader read after all faces have been copied
			textureImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			setImageLayout(
				copyCmd,
				textureImage,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				textureImageLayout,
				subresourceRange);

			VKDK::FlushCommandBuffer(copyCmd, VKDK::graphicsQueue, true);

			// Clean up staging resources
			vkFreeMemory(VKDK::device, stagingBufferMemory, nullptr);
			vkDestroyBuffer(VKDK::device, stagingBuffer, nullptr);
		}

		void createImageSampler() {
			// Create sampler
			VkSamplerCreateInfo sampler = vks::initializers::samplerCreateInfo();
			sampler.magFilter = VK_FILTER_LINEAR;
			sampler.minFilter = VK_FILTER_LINEAR;
			sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			sampler.addressModeV = sampler.addressModeU;
			sampler.addressModeW = sampler.addressModeU;
			sampler.mipLodBias = 0.0f;
			sampler.compareOp = VK_COMPARE_OP_NEVER;
			sampler.minLod = 0.0f;
			sampler.maxLod = mipLevels;
			sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
			sampler.maxAnisotropy = 1.0f;
			if (VKDK::deviceFeatures.samplerAnisotropy)
			{
				sampler.maxAnisotropy = VKDK::deviceProperties.limits.maxSamplerAnisotropy;
				sampler.anisotropyEnable = VK_TRUE;
			}
			VK_CHECK_RESULT(vkCreateSampler(VKDK::device, &sampler, nullptr, &textureSampler));
		}

		void createImageView() {
			// Create image view
			VkImageViewCreateInfo view = vks::initializers::imageViewCreateInfo();
			// Cube map view type
			view.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
			view.format = format;
			view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
			view.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			// 6 array layers (faces)
			view.subresourceRange.layerCount = 6;
			// Set number of mip levels
			view.subresourceRange.levelCount = mipLevels;
			view.image = textureImage;
			VK_CHECK_RESULT(vkCreateImageView(VKDK::device, &view, nullptr, &textureImageView));
		}

		void cleanup() {
			vkDestroySampler(VKDK::device, textureSampler, nullptr);
			vkDestroyImageView(VKDK::device, textureImageView, nullptr);

			vkDestroyImage(VKDK::device, textureImage, nullptr);
			vkFreeMemory(VKDK::device, textureImageMemory, nullptr);
		};
		VkImageView getImageView() { return textureImageView; };
		VkSampler getSampler() { return textureSampler; };
		VkImageLayout getLayout() {
			return textureImageLayout;
		}

	private:
		VkImage textureImage;
		VkFormat format;
		VkDeviceMemory textureImageMemory;
		VkImageView textureImageView;
		VkImageLayout textureImageLayout;
		VkSampler textureSampler;
		uint32_t width, height, mipLevels = 1;


	};
}