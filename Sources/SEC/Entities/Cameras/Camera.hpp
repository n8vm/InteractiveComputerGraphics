#pragma once
#include "Entities/Entity.hpp"

namespace Entities::Cameras {
	struct CameraBufferObject {
		glm::mat4 View;
		glm::mat4 Projection;
	};

	/* A camera is an entity which defines an interface for obtaining a projection matrix for rendering the scene.
		This camera class cannot be instantiated directly. */
	class Camera : public Entity {
	public:
		virtual glm::mat4 getView() = 0;
		virtual glm::mat4 getProjection() = 0;
		virtual float getNear() = 0;
		virtual void setWindowSize(int width, int height) {};
	};
}