// ┌──────────────────────────────────────────────────────────────────┐
// │ Developer : n8vm                                                 |
// │   Texture2D: Creates a texture component from a given image.     |
// |                                                                  |
// |  In general, the process for creating a texture is as follows:   |
// |    1. Load image data from either .png or .ktx                   |
// |    2. Determine if the image format is supported                 |
// |    3. Create a staging buffer and upload image data to that.     |
// |    4. Create an optimal image handle, and bind memory to it.     |
// |    5. Transfer contents from staging to final image memory       |
// |    6. Create a 2D image view                                     |
// |    7. Create a 2D image view Sampler                             |
// └──────────────────────────────────────────────────────────────────┘

#pragma once
#include "vkdk.hpp"
#include "Texture.hpp"

#include <gli/gli.hpp>

#include "Systems/ComponentManager.hpp"

namespace Components::Textures {
  class Texture2D : public TextureInterface {

  public:
    /* Component Generator */
    static std::shared_ptr<Texture> Create(std::string name, std::string imagePath = ResourcePath "Defaults/UV_Grid_Sm.ktx") {
      std::cout << "ComponentManager: Adding Texture2D \"" << name << "\"" << std::endl;

      auto tex2d = std::make_shared<Texture2D>(imagePath);
      auto texComponent = std::make_shared<Texture>();
      texComponent->texture = tex2d;
      Systems::ComponentManager::Textures[name] = texComponent;
      return texComponent;
    }

    /* Constructors */
    Texture2D(std::string imagePath = ResourcePath "Defaults/missing-texture.ktx") {
      viewType = VK_IMAGE_VIEW_TYPE_2D;
      struct stat st;
      if (stat(imagePath.c_str(), &st) != 0) {
        std::cout << imagePath + " does not exist!" << std::endl;
        imagePath = ResourcePath "Defaults/missing-texture.ktx";
      }
      if (imagePath.substr(imagePath.find_last_of(".") + 1) == "ktx") {
        createTextureImageKTX(imagePath);
      }
      else {
        createTextureImagePNG(imagePath);
      }
      createImageView();
      createImageSampler();
    }

    void createTextureImageKTX(std::string imagePath) {
      /* Load the texture */
      gli::texture2d tex2D(gli::load(imagePath));
      assert(!tex2D.empty());

      VkFormatProperties formatProperties;

      width = (uint32_t)(tex2D[0].extent().x);
      height = (uint32_t)(tex2D[0].extent().y);
      colorMipLevels = (uint32_t)(tex2D.levels());
      colorFormat = (VkFormat)tex2D.format();

      // Get device properites for the requested texture format
      vkGetPhysicalDeviceFormatProperties(VKDK::physicalDevice, colorFormat, &formatProperties);

      if (formatProperties.bufferFeatures == 0
        && formatProperties.linearTilingFeatures == 0
        && formatProperties.optimalTilingFeatures == 0)
      {
        std::cout << "Unsupported image format for " << imagePath << std::endl;
        createTextureImageKTX(ResourcePath "Defaults/missing-texture.ktx");
        return;
      }

      /* Create staging buffer */
      VkBuffer stagingBuffer;
      VkDeviceMemory stagingBufferMemory;
      VKDK::CreateBuffer(tex2D.size(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
        stagingBuffer, stagingBufferMemory);

      // Copy texture data into staging buffer
      void* data;
      vkMapMemory(VKDK::device, stagingBufferMemory, 0, tex2D.size(), 0, &data);
      memcpy(data, tex2D.data(), static_cast<size_t>(tex2D.size()));
      vkUnmapMemory(VKDK::device, stagingBufferMemory);

      // Setup buffer copy regions for each mip level
      std::vector<VkBufferImageCopy> bufferCopyRegions;
      uint32_t offset = 0;

      for (uint32_t i = 0; i < colorMipLevels; i++)
      {
        VkBufferImageCopy bufferCopyRegion = {};
        bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        bufferCopyRegion.imageSubresource.mipLevel = i;
        bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
        bufferCopyRegion.imageSubresource.layerCount = 1;
        bufferCopyRegion.imageExtent.width = static_cast<uint32_t>(tex2D[i].extent().x);
        bufferCopyRegion.imageExtent.height = static_cast<uint32_t>(tex2D[i].extent().y);
        bufferCopyRegion.imageExtent.depth = 1;
        bufferCopyRegion.bufferOffset = offset;

        bufferCopyRegions.push_back(bufferCopyRegion);

        offset += static_cast<uint32_t>(tex2D[i].size());
      }

      // Create optimal tiled target image
      VkImageCreateInfo imageCreateInfo = {};
      imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
      imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
      imageCreateInfo.format = colorFormat;
      imageCreateInfo.mipLevels = colorMipLevels;
      imageCreateInfo.arrayLayers = 1;
      imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
      imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
      imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      // Set initial layout of the image to undefined
      imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      imageCreateInfo.extent = { width, height, 1 };
      imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

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
      VkCommandBuffer copyCmd = VKDK::CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);


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
      subresourceRange.layerCount = 1;

      // Optimal image will be used as destination for the copy, so we must transfer from our
      // initial undefined image layout to the transfer destination layout
      setImageLayout(
        copyCmd,
        colorImage,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        subresourceRange);

      // Copy mip levels from staging buffer
      vkCmdCopyBufferToImage(
        copyCmd,
        stagingBuffer,
        colorImage,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        static_cast<uint32_t>(bufferCopyRegions.size()),
        bufferCopyRegions.data());

      // Change texture image layout to shader read after all mip levels have been copied
      colorImageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      setImageLayout(
        copyCmd,
        colorImage,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        colorImageLayout,
        subresourceRange);

      VKDK::FlushCommandBuffer(copyCmd, VKDK::graphicsQueue, true);

      // Clean up staging resources
      vkDestroyBuffer(VKDK::device, stagingBuffer, nullptr);
      vkFreeMemory(VKDK::device, stagingBufferMemory, nullptr);
    }

    void createTextureImagePNG(std::string imagePath) {
      /* For PNG, we assume the following format */
      colorFormat = VK_FORMAT_R8G8B8A8_UNORM;

      /* Load the texture */
      int texWidth, texHeight, texChannels;
      stbi_set_flip_vertically_on_load(true);
      stbi_uc* pixels = stbi_load(imagePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
      VkDeviceSize imageSize = texWidth * texHeight * 4;
      width = texWidth; height = texHeight;

      if (!pixels) { throw std::runtime_error("failed to load texture image!"); }

      /* Create staging buffer */
      VkBuffer stagingBuffer;
      VkDeviceMemory stagingBufferMemory;
      VKDK::CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
        stagingBuffer, stagingBufferMemory);

      void* data;
      vkMapMemory(VKDK::device, stagingBufferMemory, 0, imageSize, 0, &data);
      memcpy(data, pixels, static_cast<size_t>(imageSize));
      vkUnmapMemory(VKDK::device, stagingBufferMemory);

      /* Clean up original image array */
      stbi_image_free(pixels);

      createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, 
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorImage, colorImageMemory);

      transitionImageLayout(colorImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, 
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
      copyBufferToImage(stagingBuffer, colorImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

      transitionImageLayout(colorImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

      vkDestroyBuffer(VKDK::device, stagingBuffer, nullptr);
      vkFreeMemory(VKDK::device, stagingBufferMemory, nullptr);
    }

    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
      VkCommandBuffer commandBuffer = VKDK::beginSingleTimeCommands();

      VkBufferImageCopy region = {};
      region.bufferOffset = 0;
      region.bufferRowLength = 0;
      region.bufferImageHeight = 0;

      region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      region.imageSubresource.mipLevel = 0;
      region.imageSubresource.baseArrayLayer = 0;
      region.imageSubresource.layerCount = 1;

      region.imageOffset = { 0, 0, 0 };
      region.imageExtent = {
        width,
        height,
        1
      };

      vkCmdCopyBufferToImage(
        commandBuffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
      );

      VKDK::endSingleTimeCommands(commandBuffer);
    }

    /* Todo use base class function */
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
      VkCommandBuffer commandBuffer = VKDK::beginSingleTimeCommands();

      VkImageMemoryBarrier barrier = {};
      barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      barrier.oldLayout = oldLayout;
      barrier.newLayout = newLayout;
      barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.image = image;
      barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      barrier.subresourceRange.baseMipLevel = 0;
      barrier.subresourceRange.levelCount = 1;
      barrier.subresourceRange.baseArrayLayer = 0;
      barrier.subresourceRange.layerCount = 1;
      barrier.srcAccessMask = 0; // TODO
      barrier.dstAccessMask = 0; // TODO

      VkPipelineStageFlags sourceStage;
      VkPipelineStageFlags destinationStage;

      if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
      }
      else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      }
      else {
        throw std::invalid_argument("unsupported layout transition!");
      }

      vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
      );

      VKDK::endSingleTimeCommands(commandBuffer);
    }

    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
      VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
      VkImageCreateInfo imageInfo = {};
      imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
      imageInfo.imageType = VK_IMAGE_TYPE_2D;
      imageInfo.extent.width = width;
      imageInfo.extent.height = height;
      imageInfo.extent.depth = 1;
      imageInfo.mipLevels = 1;
      imageInfo.arrayLayers = 1;
      imageInfo.format = format;
      imageInfo.tiling = tiling;
      imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      imageInfo.usage = usage;
      imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
      imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

      if (vkCreateImage(VKDK::device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
      }

      VkMemoryRequirements memRequirements;
      vkGetImageMemoryRequirements(VKDK::device, image, &memRequirements);

      VkMemoryAllocateInfo allocInfo = {};
      allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      allocInfo.allocationSize = memRequirements.size;
      allocInfo.memoryTypeIndex = VKDK::FindMemoryType(memRequirements.memoryTypeBits, properties);

      if (vkAllocateMemory(VKDK::device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
      }

      vkBindImageMemory(VKDK::device, image, imageMemory, 0);
    }

    void createImageView() {
      // Create image view
      // Textures are not directly accessed by the shaders and
      // are abstracted by image views containing additional
      // information and sub resource ranges
      VkImageViewCreateInfo view = vks::initializers::imageViewCreateInfo();
      view.viewType = VK_IMAGE_VIEW_TYPE_2D;
      view.format = colorFormat;
      view.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
      // The subresource range describes the set of mip levels (and array layers) that can be accessed through this image view
      // It's possible to create multiple image views for a single image referring to different (and/or overlapping) ranges of the image
      view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      view.subresourceRange.baseMipLevel = 0;
      view.subresourceRange.baseArrayLayer = 0;
      view.subresourceRange.layerCount = 1;
      // Linear tiling usually won't support mip maps
      // Only set mip map count if optimal tiling is used
      view.subresourceRange.levelCount = colorMipLevels;
      // The view will be based on the texture's image
      view.image = colorImage;
      VK_CHECK_RESULT(vkCreateImageView(VKDK::device, &view, nullptr, &colorImageView));
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
      sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
      sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
      sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
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
      sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
      VK_CHECK_RESULT(vkCreateSampler(VKDK::device, &sampler, nullptr, &colorSampler));
    }
  };
}