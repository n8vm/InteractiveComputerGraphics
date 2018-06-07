// ┌──────────────────────────────────────────────────────────────────┐
// │ Developer : n8vm                                                 |
// │  ComponentManager: Contains global simulation data, like a list  | 
// |    of textures, meshes, a scene graph, a list of lights, etc     |
// └──────────────────────────────────────────────────────────────────┘
#pragma once
#include <unordered_map>
#include <memory>

/* Forward Declarations */
namespace Entities { class Entity; }
namespace Components::Textures { class Texture; }
namespace Components::Meshes { class Mesh; }
namespace Components::Materials { class Material; }
namespace Components { class Callbacks; }
namespace Components::Math { class Transform; }
namespace Components::Math { class Perspective; }
namespace Components::Lights { class Light; }
struct PipelineKey;
struct PipelineParameters;

namespace Systems::ComponentManager {
	extern void Initialize();
	extern std::unordered_map<PipelineKey, std::shared_ptr<PipelineParameters>> PipelineSettings;
	extern std::unordered_map<std::string, std::shared_ptr<Components::Textures::Texture>> Textures;
	extern std::unordered_map<std::string, std::shared_ptr<Components::Meshes::Mesh>> Meshes;
	extern std::unordered_map<std::string, std::shared_ptr<Components::Lights::Light>> Lights;
	extern std::unordered_map<std::string, std::shared_ptr<Components::Materials::Material>> Materials;
	extern std::unordered_map<std::string, std::shared_ptr<Components::Math::Transform>> Transforms;
	extern std::unordered_map<std::string, std::shared_ptr<Components::Math::Perspective>> Perspectives;
	extern std::unordered_map<std::string, std::shared_ptr<Components::Callbacks>> Callbacks;
	extern void Cleanup();

	template <class T>
	std::shared_ptr<T> GetMaterial(std::string key) {
		auto materialComponent = ComponentManager::Materials[key];
		return std::dynamic_pointer_cast<T>(materialComponent->material);
	}
}