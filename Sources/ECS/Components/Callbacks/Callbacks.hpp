#pragma once
#include "Components/Component.hpp"
#include "Systems/ComponentManager.hpp"
#include <functional>

namespace Entities { class Entity; }

namespace Components {
	class Callbacks : public Component {
	public:
		static std::shared_ptr<Callbacks> Create(std::string name) {
			auto callbacks = std::make_shared<Callbacks>();
			Systems::ComponentManager::Callbacks[name] = callbacks;
			return callbacks;
		}

		std::function<void(std::shared_ptr<Entities::Entity>) > update = nullptr;
	};
}
