#pragma once
#include "vkdk.hpp"
#include "stb_image.h"

namespace Components::Textures {
	class Texture {
	public:
		virtual void cleanup() = 0;
		virtual VkImageView getImageView() = 0;
		virtual VkSampler getSampler() = 0;
		
		//virtual GLuint getID() = 0;
		//virtual uint32_t getWidth() = 0;
		//virtual uint32_t getHeight() = 0;
		//virtual uint32_t getDepth() = 0;
		///*Specifies the number of color components in the texture. */
		//virtual GLuint getInternalFormat() = 0;
		///*Specifies the format of the pixel data on the GPU. */
		//virtual GLuint getFormat() = 0;
		///*Specifies the per pixel host type.*/
		//virtual GLuint getType() = 0;
		//virtual GLvoid* getData() = 0;
	};
}

#include "Texture2D.hpp"
#include "RenderableTexture2D.hpp"
//#include "Texture3D.hpp"