#include "Entities/Cameras/Camera.hpp"
#include "vkdk.hpp"

namespace Entities::Cameras {
	/* A perspective camera */
	class PerspectiveCamera : public Camera {
	public:
		PerspectiveCamera(glm::vec3 eye = glm::vec3(0.0), glm::vec3 at = glm::vec3(0), glm::vec3 up = glm::vec3(0.0, 1.0, 0.0)) {
			this->up = up;
			using namespace glm;
			transform.SetPosition(eye);

			initialRot = conjugate(glm::toQuat(lookAt(eye, at, up)));
			transform.SetRotation(initialRot);

			this->initialPos = eye;
			this->initialRot = initialRot;
			this->rotatePoint = at;

			/* Create perspective transformation */
			P = glm::perspective(fov, aspectRatio, nearClippingPlane, farClippingPlane);
		}

		void update(Scene *scene, glm::mat4 model, glm::mat4 view, glm::mat4 proj) {
			M = model;
		}

		void setWindowSize(int width, int height) {
			float aspectRatio = width / (float)height;

			assert(aspectRatio > 0);
			if (aspectRatio > 0) {
				this->aspectRatio = aspectRatio;
				P = glm::perspective(fov, aspectRatio, nearClippingPlane, farClippingPlane);
			}
		}

		glm::mat4 getView() {
			return transform.ParentToLocalMatrix() * glm::inverse(M);
		}

		glm::mat4 getProjection() {
			return P;
		}

		float getNear() {
			return nearClippingPlane;
		}
		float getFar() {
			return farClippingPlane;
		}
	private:
		glm::vec3 initialPos;
		glm::quat initialRot;
		glm::vec3 rotatePoint;
		glm::vec3 up;
		glm::mat4 P, V, M;

		float fov = 45;
		float aspectRatio = 1.0;
		float nearClippingPlane = .1;
		float farClippingPlane = 100.0;
	};
}