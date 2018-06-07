#include "SceneGraph.hpp"

#include "Entities/Entity.hpp"

namespace Systems::SceneGraph {
	std::unordered_map<std::string, std::shared_ptr<Entities::Entity>> Entities;
}