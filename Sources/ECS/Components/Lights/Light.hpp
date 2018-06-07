#pragma once
#include "Components/Component.hpp"
#include <memory>
#include <glm/glm.hpp>

namespace Components::Lights {
	class LightInterface {
		public:
			virtual glm::vec4 getColor() = 0;
	};

	class Light : public Component {
	public:
		std::shared_ptr<LightInterface> light;
	};
}