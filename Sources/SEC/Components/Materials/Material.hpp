#pragma once
#include "vkdk.hpp"
#include "Components/Meshes/Mesh.hpp"

class Scene;

namespace Components {
	namespace Materials {
		/* A material defines a render interface, and cannot be instantiated directly. */
		class Material {
		public:
			virtual void render(Scene *scene, std::shared_ptr<Components::Meshes::Mesh> mesh) = 0;
			virtual void update(glm::mat4 model, glm::mat4 view, glm::mat4 projection) = 0;
			virtual void cleanup() = 0;

		protected:
			/* Helper function for reading in SPIR-V files */
			static std::vector<char> readFile(const std::string& filename) {
				std::ifstream file(filename, std::ios::ate | std::ios::binary);

				if (!file.is_open()) {
					throw std::runtime_error("failed to open file!");
				}

				size_t fileSize = (size_t)file.tellg();
				std::vector<char> buffer(fileSize);

				file.seekg(0);
				file.read(buffer.data(), fileSize);
				file.close();

				return buffer;
			}
		};
	}
}

#include "Components/Materials/HaloMaterials/UniformColoredPoints.hpp"

#include "Components/Materials/SurfaceMaterials/UniformColoredSurface.hpp"
#include "Components/Materials/SurfaceMaterials/NormalSurface.hpp"
#include "Components/Materials/SurfaceMaterials/TexCoordSurface.hpp"
#include "Components/Materials/SurfaceMaterials/BlinnSurface.hpp"
#include "Components/Materials/SurfaceMaterials/TexturedBlinnSurface.hpp"

	//inline void InitializeAllMaterials(int maxOfEach) {
	//	Components::Materials::SurfaceMaterials::NormalSurface::Initialize(maxOfEach);
	//	Components::Materials::SurfaceMaterials::TexCoordSurface::Initialize(maxOfEach);
	//	Components::Materials::SurfaceMaterials::UniformColoredSurface::Initialize(maxOfEach);
	//	Components::Materials::SurfaceMaterials::BlinnSurface::Initialize(maxOfEach);
	//	Components::Materials::SurfaceMaterials::TexturedBlinnSurface::Initialize(maxOfEach);
	//}
