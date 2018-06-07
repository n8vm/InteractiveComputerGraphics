#pragma once

#include <memory>
#include <string>
#include <unordered_map>

/* Forward Declarations */
namespace Entities { class Entity; }

namespace Systems::SceneGraph {
	extern std::unordered_map<std::string, std::shared_ptr<Entities::Entity>> Entities;
}