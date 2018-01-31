#include "Entities/Entity.hpp"

namespace Entities::Cameras {
	/* A camera is an entity which defines an interface for obtaining a projection matrix for rendering the scene.
		This camera class cannot be instantiated directly. */
	class Camera : public Entity {
		virtual glm::mat4 getView() = 0;
		virtual glm::mat4 getProjection() = 0;
	};
}