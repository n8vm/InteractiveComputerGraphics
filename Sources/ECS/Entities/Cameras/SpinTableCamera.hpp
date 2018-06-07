#include "vkdk.hpp"

#include "Entities/Entity.hpp"
#include "Systems/ComponentManager.hpp"

namespace Entities::Cameras {
	/* A perspective gamera*/
	class SpinTableCamera : public Entity {
	public:
		static std::shared_ptr<SpinTableCamera> Create(
			std::string name,
			glm::vec3 initialPos = glm::vec3(0.0),
			glm::vec3 rotatePoint = glm::vec3(0),
			glm::vec3 up = glm::vec3(0.0, 1.0, 0.0),
			bool alternate = false, float fov = 45) 
		{
			auto camera = std::make_shared<SpinTableCamera>(name, initialPos, rotatePoint, up, alternate, fov);
			Systems::SceneGraph::Entities[name] = camera;
			return camera;
		}

		SpinTableCamera(
			std::string name,
			glm::vec3 initialPos = glm::vec3(0.0),
			glm::vec3 rotatePoint = glm::vec3(0),
			glm::vec3 up = glm::vec3(0.0, 1.0, 0.0),
			bool alternate = false, float fov = 45) : Entity(name)
		{
			using namespace glm;

			this->up = up;
			this->fov = fov;
			this->initialPos = initialPos;
			this->initialRotPoint = rotatePoint;
			this->rotatePoint = rotatePoint;
			this->alternate = alternate;

			/* Move this camera's transform to look at the specified point */
			transform->SetPosition(initialPos);
			initialRot = conjugate(glm::toQuat(lookAt(initialPos, rotatePoint, up)));
			transform->SetRotation(initialRot);

			callbacks->update = [](std::shared_ptr<Entities::Entity> entity) {
				auto camera = std::static_pointer_cast<SpinTableCamera>(entity);
				camera->update();
			};
		}

		void handleArrowKeys() {
			float arrowSpeed = 10;
			if (glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_UP)) {
				pitchVelocity += arrowSpeed * rotationAcceleration;
			}
			if (glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_DOWN)) {
				pitchVelocity -= arrowSpeed * rotationAcceleration;
			}
			if (glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_LEFT)) {
				yawVelocity += arrowSpeed * rotationAcceleration;
			}
			if (glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_RIGHT)) {
				yawVelocity -= arrowSpeed * rotationAcceleration;
			}
		}

		void handleMouse() {
			//if (glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_RIGHT_CONTROL) || glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_LEFT_CONTROL)) return;
			/* GLFW doesn't give hold info, so we have to handle it ourselves here. */
			/* GLFW also doesn't supply delta cursor position, so we compute it. */

			if (glfwGetMouseButton(VKDK::DefaultWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
				if (!mousePrevPressed) {
					glfwGetCursorPos(VKDK::DefaultWindow, &oldXPos, &oldYPos);
					mousePrevPressed = true;
				}
				else {
					glfwGetCursorPos(VKDK::DefaultWindow, &newXPos, &newYPos);

					if (glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_LEFT_CONTROL)) {
						frwdVelocity += float(-(newYPos - oldYPos) * zoomResistance * .1);
					}
					else if (glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_LEFT_ALT)) {
						rightVelocity += float(-(newXPos - oldXPos) * zoomResistance * .1);
						upVelocity += float((newYPos - oldYPos) * zoomResistance * .1);
					}
					else {
						yawVelocity += float(-(newXPos - oldXPos) * rotationAcceleration);
						pitchVelocity += float(-(newYPos - oldYPos) * rotationAcceleration);
					}
					oldXPos = newXPos;
					oldYPos = newYPos;
				}
			}
			else if (glfwGetMouseButton(VKDK::DefaultWindow, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
			{
				if (!mousePrevPressed) {
					glfwGetCursorPos(VKDK::DefaultWindow, &oldXPos, &oldYPos);
					mousePrevPressed = true;
				}
				else {
					glfwGetCursorPos(VKDK::DefaultWindow, &newXPos, &newYPos);
					zoomVelocity += float((oldYPos - newYPos) * zoomAcceleration * .1);
					oldXPos = newXPos;
					oldYPos = newYPos;
				}
			}

			if ((glfwGetMouseButton(VKDK::DefaultWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE)
				&& (glfwGetMouseButton(VKDK::DefaultWindow, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE)) {
				mousePrevPressed = false;
			}
		}

		void handleZoom() {
			float arrowSpeed = 1;
			if (glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_MINUS)) {
				zoomVelocity -= arrowSpeed * zoomAcceleration;
			}
			if (glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_EQUAL)) {
				zoomVelocity += arrowSpeed * zoomAcceleration;
			}
		}

		void handleReset() {
			if (glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_R)) {
				transform->SetPosition(initialPos);
				transform->SetRotation(initialRot);
				rotatePoint = initialRotPoint;
				pitchVelocity = yawVelocity = 0;
				zoomVelocity = 0;
			}
		}

		void update() {
			//if ((!alternate && !(glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_LEFT_ALT)))
			//	|| (alternate && (glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_LEFT_ALT))))
			//{

				handleArrowKeys();
				handleMouse();
				handleZoom();
				handleReset();

				zoomVelocity -= zoomVelocity * zoomResistance;
				yawVelocity -= yawVelocity * rotateResistance;
				pitchVelocity -= pitchVelocity * rotateResistance;
				frwdVelocity -= frwdVelocity * rotateResistance;
				upVelocity -= upVelocity * rotateResistance;
				rightVelocity -= rightVelocity * rotateResistance;
        //yawVelocity = .2;

				transform->AddPosition(glm::normalize(rotatePoint - transform->GetPosition()) * zoomVelocity);
				
				glm::vec3 additionalPos = (transform->forward.load() * frwdVelocity) + (transform->up.load() * upVelocity) + (transform->right.load() * rightVelocity);
				transform->AddPosition(additionalPos);
				rotatePoint += additionalPos;

				glm::vec3 currentRight = transform->right.load();
				glm::vec3 currentUp = transform->up.load();

				transform->RotateAround(rotatePoint, currentRight, pitchVelocity);
				transform->RotateAround(rotatePoint, up, yawVelocity);

				aspectRatio = VKDK::CurrentWindowSize[0] / (float)VKDK::CurrentWindowSize[1];

				auto perspective = getFirstComponent<Components::Math::Perspective>();

				if (perspective) {
          perspective->projections[0] = glm::perspective(fov, aspectRatio, nearClippingPlane, farClippingPlane);
          perspective->nearPos = nearClippingPlane;
          perspective->farPos = farClippingPlane;
					perspective->views[0] = getWorldToLocalMatrix();
				}
			//}
		}

		void setWindowSize(int width, int height) {
			float aspectRatio = width / (float)height;

			assert(aspectRatio > 0);
			auto perspective = getFirstComponent<Components::Math::Perspective>();

			if (perspective) {
        perspective->projections[0] = glm::perspective(fov, aspectRatio, nearClippingPlane, farClippingPlane);
        perspective->nearPos = nearClippingPlane;
        perspective->farPos = farClippingPlane;
        perspective->views[0] = getWorldToLocalMatrix();
			}
		}

		void setFieldOfView(float newFOV) {
			fov = newFOV;
		}

		void setZoomAcceleration(float newZoomAcceleration) {
			zoomAcceleration = newZoomAcceleration;
		}

	private:
		glm::vec3 initialPos;
		glm::quat initialRot;
		glm::vec3 initialRotPoint;
		glm::vec3 rotatePoint;
		glm::vec3 up;

		float zoomVelocity = 0.0f;
		float zoomAcceleration = .01f;
		float zoomResistance = .1f;

		float yawVelocity = 0.0f;
		float pitchVelocity = 0.0f;

		float frwdVelocity = 0.0f;
		float rightVelocity = 0.0f;
		float upVelocity = 0.0f;

		float rotationAcceleration = 0.03f;
		float rotateResistance = .1f;

		bool mousePrevPressed = false;
		double oldXPos, oldYPos, newXPos, newYPos;
		bool alternate;

		float fov = 45;
		float aspectRatio = 1.0;
		float nearClippingPlane = .1f;
		float farClippingPlane = 1000.0;
	};
}