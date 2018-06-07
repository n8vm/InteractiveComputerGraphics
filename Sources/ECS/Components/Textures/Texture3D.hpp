// ┌──────────────────────────────────────────────────────────────────┐
// │ Developer : n8vm                                                 |
// │   Texture3D: Creates a texture component from a given image.     |
// |                                                                  |
// |  In general, the process for creating a texture is as follows:   |
// |    1. Load image data from .ktx (.ktx only for 3D tex)           |
// |    2. Determine if the image format is supported                 |
// |    3. Create a staging buffer and upload image data to that.     |
// |    4. Create an optimal image handle, and bind memory to it.     |
// |    5. Transfer contents from staging to final image memory       |
// |    6. Create a 3D image view                                     |
// |    7. Create a 3D image view Sampler                             |
// └──────────────────────────────────────────────────────────────────┘

#pragma once
#include "vkdk.hpp"
#include "Texture.hpp"
#include <assert.h>
#include <gli/gli.hpp>

#include "Components/Component.hpp"

#include "Systems/ComponentManager.hpp"

#include "Tools/FileReader.hpp"
namespace Components::Textures {
	class Texture3D : public TextureInterface {
	public:
		static void RawToKTX(std::string rawfilepath, gli::format format, gli::extent3d extent) {

			auto volumeData = readFile(rawfilepath);

			auto texture = gli::texture(gli::target::TARGET_3D, format, extent, 1, 1, 1);
			memcpy(texture.data(), volumeData.data(), volumeData.size() * sizeof(unsigned char));

			gli::save_ktx(texture, ResourcePath "output.ktx");
		}

		static std::shared_ptr<Texture> Create(std::string name, std::string filePath = ResourcePath "Defaults/missing-volume.ktx", bool genMipmaps = false) {
			std::cout << "ComponentManager: Adding Texture3D \"" << name << "\"" << std::endl;

			auto tex3D = std::make_shared<Texture3D>(filePath, genMipmaps);
			auto texComponent = std::make_shared<Texture>();
			texComponent->texture = tex3D;
			Systems::ComponentManager::Textures[name] = texComponent;
			return texComponent;
		}

		static std::shared_ptr<Texture> Create(std::string name, uint32_t width, uint32_t height, uint32_t depth, bool genMipmaps = false) {
			std::cout << "ComponentManager: Adding Texture3D \"" << name << "\"" << std::endl;

			auto tex3D = std::make_shared<Texture3D>(width, height, depth, genMipmaps);
			auto texComponent = std::make_shared<Texture>();
			texComponent->texture = tex3D;
			Systems::ComponentManager::Textures[name] = texComponent;
			return texComponent;
		}

		Texture3D(std::string imagePath = ResourcePath "Defaults/missing-volume.ktx", bool genMipmaps = false) {
      viewType = VK_IMAGE_VIEW_TYPE_3D;
      struct stat st;
			if (stat(imagePath.c_str(), &st) != 0) {
				std::cout<< imagePath + " does not exist!" << std::endl;
				imagePath = ResourcePath "Defaults/missing-volume.ktx";
			}

			if (imagePath.substr(imagePath.find_last_of(".") + 1) == "ktx") {
				createTextureImageKTX(imagePath, genMipmaps);
			}
			else {
				throw std::runtime_error("Only ktx supported for 3D textures ");
			}
			createColorImageView();
			createImageSampler();
		}

		Texture3D(uint32_t width, uint32_t height, uint32_t depth, bool genMipmaps = false) {
      viewType = VK_IMAGE_VIEW_TYPE_3D;
			createTextureImage(width, height, depth, genMipmaps);
			createImageSampler();
			createColorImageView();
		}

		void createTextureImage(uint32_t width, uint32_t height, uint32_t depth, bool genMipmaps = false) {
			VkFormatProperties formatProperties;
			colorFormat = VK_FORMAT_R8G8B8A8_UNORM;

			this->width = width;
			this->height = height;
			this->depth = depth;
			colorMipLevels = 1;
			if (genMipmaps) colorMipLevels = (uint32_t)(std::floor(std::log2(std::max(std::max(width, height), depth))) + 1);

			// Get device properites for the requested texture format
			vkGetPhysicalDeviceFormatProperties(VKDK::physicalDevice, colorFormat, &formatProperties);

			// Create optimal tiled target image
			VkImageCreateInfo imageCreateInfo = {};
			imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageCreateInfo.imageType = VK_IMAGE_TYPE_3D;
			imageCreateInfo.format = colorFormat;
			imageCreateInfo.mipLevels = colorMipLevels;
			imageCreateInfo.arrayLayers = 1;
			imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			// Set initial layout of the image to undefined
			imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageCreateInfo.extent = { width, height, depth };
			imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;

			VK_CHECK_RESULT(vkCreateImage(VKDK::device, &imageCreateInfo, nullptr, &colorImage));

			/* Allocate and bind memory for the texture*/
			VkMemoryAllocateInfo memAllocInfo = {};
			memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			VkMemoryRequirements memReqs = {};

			vkGetImageMemoryRequirements(VKDK::device, colorImage, &memReqs);

			memAllocInfo.allocationSize = memReqs.size;
			memAllocInfo.memoryTypeIndex = VKDK::FindMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			VK_CHECK_RESULT(vkAllocateMemory(VKDK::device, &memAllocInfo, nullptr, &colorImageMemory));
			VK_CHECK_RESULT(vkBindImageMemory(VKDK::device, colorImage, colorImageMemory, 0));

			/* Transition between formats */
			VkCommandBuffer cmdBuffer = VKDK::CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

			// Image barrier for optimal image

			// The sub resource range describes the regions of the image we will be transition
			VkImageSubresourceRange subresourceRange = {};
			// Image only contains color data
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			// Start at first mip level
			subresourceRange.baseMipLevel = 0;
			// We will transition on all mip levels
			subresourceRange.levelCount = colorMipLevels;
			// The 2D texture only has one layer
      subresourceRange.layerCount = layers;

			/* Initially the texture is undefined. We transfer layout to general by default. */
			setImageLayout(
				cmdBuffer,
				colorImage,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_GENERAL,
				subresourceRange);

			colorImageLayout = VK_IMAGE_LAYOUT_GENERAL;
			VKDK::FlushCommandBuffer(cmdBuffer, VKDK::graphicsQueue, true);
		}

		void createTextureImageKTX(std::string imagePath, bool genMipmaps = false) {
			/* Load the texture */
			colorFormat = VK_FORMAT_R8G8B8A8_UNORM;
			gli::texture3d tex3D(gli::load(imagePath));
			assert(!tex3D.empty());

			VkFormatProperties formatProperties;

			width = static_cast<uint32_t>(tex3D[0].extent().x);
			height = static_cast<uint32_t>(tex3D[0].extent().y);
			depth = static_cast<uint32_t>(tex3D[0].extent().z);
			colorMipLevels = static_cast<uint32_t>(tex3D.levels());

			if (genMipmaps) colorMipLevels = (uint32_t)(std::floor(std::log2(std::max(std::max(width, height), depth))) + 1);

			// Get device properites for the requested texture format
			vkGetPhysicalDeviceFormatProperties(VKDK::physicalDevice, colorFormat, &formatProperties);

			/* Create staging buffer */
			VkBuffer stagingBuffer;
			VkDeviceMemory stagingBufferMemory;
			VKDK::CreateBuffer(tex3D.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

			// Copy texture data into staging buffer
			void* data;
			vkMapMemory(VKDK::device, stagingBufferMemory, 0, tex3D.size(), 0, &data);
			memcpy(data, tex3D.data(), static_cast<size_t>(tex3D.size()));
			vkUnmapMemory(VKDK::device, stagingBufferMemory);

			// Setup buffer copy regions for each mip level
			std::vector<VkBufferImageCopy> bufferCopyRegions;
			uint32_t offset = 0;

			for (uint32_t i = 0; i < tex3D.levels(); i++)
			{
				VkBufferImageCopy bufferCopyRegion = {};
				bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				bufferCopyRegion.imageSubresource.mipLevel = i;
				bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
				bufferCopyRegion.imageSubresource.layerCount = 1;
				bufferCopyRegion.imageExtent.width = static_cast<uint32_t>(tex3D[i].extent().x);
				bufferCopyRegion.imageExtent.height = static_cast<uint32_t>(tex3D[i].extent().y);
				bufferCopyRegion.imageExtent.depth = static_cast<uint32_t>(tex3D[i].extent().z);
				bufferCopyRegion.bufferOffset = offset;

				bufferCopyRegions.push_back(bufferCopyRegion);

				offset += static_cast<uint32_t>(tex3D[i].size());

				/* If we're going to generate our own mipmaps, just copy the first level */
				if (genMipmaps) break;
			}

			// Create optimal tiled target image
			VkImageCreateInfo imageCreateInfo = {};
			imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageCreateInfo.imageType = VK_IMAGE_TYPE_3D;
			imageCreateInfo.format = colorFormat;
			imageCreateInfo.mipLevels = colorMipLevels;
			imageCreateInfo.arrayLayers = 1;
			imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			// Set initial layout of the image to undefined
			imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageCreateInfo.extent = { width, height, depth };
			imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;

			VK_CHECK_RESULT(vkCreateImage(VKDK::device, &imageCreateInfo, nullptr, &colorImage));

			/* Allocate and bind memory for the texture*/
			VkMemoryAllocateInfo memAllocInfo = {};
			memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			VkMemoryRequirements memReqs = {};

			vkGetImageMemoryRequirements(VKDK::device, colorImage, &memReqs);

			memAllocInfo.allocationSize = memReqs.size;
			memAllocInfo.memoryTypeIndex = VKDK::FindMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			
			VK_CHECK_RESULT(vkAllocateMemory(VKDK::device, &memAllocInfo, nullptr, &colorImageMemory));
			VK_CHECK_RESULT(vkBindImageMemory(VKDK::device, colorImage, colorImageMemory, 0));

			/* Transition between formats */
			VkCommandBuffer cmdBuffer = VKDK::CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

			// Image barrier for optimal image

			// The sub resource range describes the regions of the image we will be transition
			VkImageSubresourceRange subresourceRange = {};
			// Image only contains color data
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			// Start at first mip level
			subresourceRange.baseMipLevel = 0;
			// We will transition on all mip levels
			subresourceRange.levelCount = colorMipLevels;
			// The 2D texture only has one layer
			subresourceRange.layerCount = layers;

			// Optimal image will be used as destination for the copy, so we must transfer from our
			// initial undefined image layout to the transfer destination layout
			setImageLayout(
				cmdBuffer,
				colorImage,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				subresourceRange);

			// Copy mip levels from staging buffer
			vkCmdCopyBufferToImage(
				cmdBuffer,
				stagingBuffer,
				colorImage,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				static_cast<uint32_t>(bufferCopyRegions.size()),
				bufferCopyRegions.data());

			// Change texture image layout to shader read after all mip levels have been copied
			colorImageLayout = VK_IMAGE_LAYOUT_GENERAL; /* Forcing this to be general for image store in shader */
			setImageLayout(
				cmdBuffer,
				colorImage,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				colorImageLayout,
				subresourceRange);

			if (genMipmaps) generateColorMipMap(cmdBuffer);

			VKDK::FlushCommandBuffer(cmdBuffer, VKDK::graphicsQueue, true);

			// Clean up staging resources
			vkDestroyBuffer(VKDK::device, stagingBuffer, nullptr);
			vkFreeMemory(VKDK::device, stagingBufferMemory, nullptr);
		}
    		
		void createImageSampler() {
			// Create a texture sampler
			// In Vulkan textures are accessed by samplers
			// This separates all the sampling information from the texture data. This means you could have multiple sampler objects for the same texture with different settings
			// Note: Similar to the samplers available with OpenGL 3.3
			VkSamplerCreateInfo sampler = vks::initializers::samplerCreateInfo();
			sampler.magFilter = VK_FILTER_LINEAR;
			sampler.minFilter = VK_FILTER_LINEAR;
			sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			sampler.mipLodBias = 0.0f;
			sampler.compareOp = VK_COMPARE_OP_NEVER;
			sampler.minLod = 0.0f;
			// Set max level-of-detail to mip level count of the texture
			sampler.maxLod = (float)colorMipLevels;
			// Enable anisotropic filtering
			// This feature is optional, so we must check if it's supported on the device
			if (VKDK::deviceFeatures.samplerAnisotropy)
			{
				// Use max. level of anisotropy for this example
				sampler.maxAnisotropy = VKDK::deviceProperties.limits.maxSamplerAnisotropy;
				sampler.anisotropyEnable = VK_TRUE;
			}
			else
			{
				// The device does not support anisotropic filtering
				sampler.maxAnisotropy = 1.0;
				sampler.anisotropyEnable = VK_FALSE;
			}
			sampler.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
			VK_CHECK_RESULT(vkCreateSampler(VKDK::device, &sampler, nullptr, &colorSampler));
		}

    /* Todo: move to interface */
		void generateColorMipMap(VkCommandBuffer cmdBuffer) {
			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.levelCount = 1;
			subresourceRange.layerCount = layers;

			setImageLayout(
				cmdBuffer,
				colorImage,
				colorImageLayout,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				subresourceRange);

			for (uint32_t i = 1; i < colorMipLevels; i++)
			{
				VkImageBlit imageBlit{};

				// Source
				imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageBlit.srcSubresource.layerCount = 1;
				imageBlit.srcSubresource.mipLevel = i - 1;
				imageBlit.srcOffsets[1].x = int32_t(width >> (i - 1));
				imageBlit.srcOffsets[1].y = int32_t(height >> (i - 1));
				imageBlit.srcOffsets[1].z = int32_t(height >> (i - 1));

				// Destination
				imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageBlit.dstSubresource.layerCount = 1;
				imageBlit.dstSubresource.mipLevel = i;
				imageBlit.dstOffsets[1].x = int32_t(width >> i);
				imageBlit.dstOffsets[1].y = int32_t(height >> i);
				imageBlit.dstOffsets[1].z = int32_t(depth >> i);

				VkImageSubresourceRange mipSubRange = {};
				mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				mipSubRange.baseMipLevel = i;
				mipSubRange.levelCount = 1;
				mipSubRange.layerCount = 1;

				// Transition current mip level to transfer dest
				setImageLayout(
					cmdBuffer,
					colorImage,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					mipSubRange);

				vkCmdBlitImage(
					cmdBuffer,
					colorImage,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					colorImage,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1,
					&imageBlit,
          VK_FILTER_LINEAR);

				setImageLayout(
					cmdBuffer,
          colorImage,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					mipSubRange);
			}

			/* Finally, transfer back to general */
			subresourceRange.levelCount = colorMipLevels;
			setImageLayout(
				cmdBuffer,
				colorImage,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_IMAGE_LAYOUT_GENERAL,
				subresourceRange);
		}
	};	
}