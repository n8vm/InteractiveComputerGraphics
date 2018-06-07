#include "ComponentManager.hpp"

#include "Components/Textures/Texture.hpp"
#include "Components/Math/Transform.hpp"
#include "Components/Math/Perspective.hpp"
#include "Components/Materials/Material.hpp"
#include "Components/Meshes/Mesh.hpp"
#include "Components/Lights/Light.hpp"
#include "Components/Callbacks/Callbacks.hpp"

#include "Components/Textures/Textures.hpp"

#include "Components/Materials/PipelineParameters.hpp"
#include "Components/Lights/PointLight/PointLight.hpp"
#include "Components/Meshes/Meshes.hpp"

namespace Systems::ComponentManager {
	std::unordered_map<PipelineKey, std::shared_ptr<PipelineParameters>> PipelineSettings;
	std::unordered_map<std::string, std::shared_ptr<Components::Textures::Texture>> Textures;
	std::unordered_map<std::string, std::shared_ptr<Components::Meshes::Mesh>> Meshes;
	std::unordered_map<std::string, std::shared_ptr<Components::Lights::Light>> Lights;
	std::unordered_map<std::string, std::shared_ptr<Components::Materials::Material>> Materials;
	std::unordered_map<std::string, std::shared_ptr<Components::Math::Transform>> Transforms;
	std::unordered_map<std::string, std::shared_ptr<Components::Math::Perspective>> Perspectives;
	std::unordered_map<std::string, std::shared_ptr<Components::Callbacks>> Callbacks;

	void Initialize() {
		/* Initialize Lights */
		Components::Lights::PointLights::Initialize();

		/* By default, load placeholder textures */
		Components::Textures::Texture2D::Create("DefaultTexture");
		Components::Textures::Texture3D::Create("DefaultTexture3D");
		Components::Textures::TextureCube::Create("DefaultTextureCube");

		/* By default, load the following meshes */
		Components::Meshes::Sphere::Create("Sphere");
		Components::Meshes::Plane::Create("Plane");
		Components::Meshes::Cube::Create("Cube");
	}

	void Cleanup() {
		for (auto &pair : Textures)
			pair.second->cleanup();
		for (auto &pair : Meshes)
			pair.second->cleanup();
		for (auto &pair : Lights)
			pair.second->cleanup();
		for (auto &pair : Materials)
			pair.second->cleanup();
		for (auto &pair : Transforms)
			pair.second->cleanup();
		for (auto &pair : Perspectives)
			pair.second->cleanup();
		for (auto &pair : Callbacks)
			pair.second->cleanup();

		/* Destroy Light Resources */
		Components::Lights::PointLights::Destroy();
	}
}