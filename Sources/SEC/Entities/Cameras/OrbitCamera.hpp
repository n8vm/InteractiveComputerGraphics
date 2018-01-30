#include "Entities/Cameras/Camera.hpp"
#include "vkdk.hpp"

namespace Entities::Cameras {
	/* A perspective gamera*/
	class OrbitCamera : public Camera {
	public:
		OrbitCamera(glm::vec3 initialPos = glm::vec3(0.0), glm::vec3 rotatePoint = glm::vec3(0), glm::quat initialRot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f)) {
			transform.SetPosition(initialPos);
			transform.SetRotation(initialRot);

			this->initialPos = initialPos;
			this->initialRot = initialRot;
			this->rotatePoint = rotatePoint;
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
			/* GLFW doesn't give hold info, so we have to handle it ourselves here. */
			/* GLFW also doesn't supply delta cursor position, so we compute it. */

			if (glfwGetMouseButton(VKDK::DefaultWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
				if (!mousePrevPressed) {
					glfwGetCursorPos(VKDK::DefaultWindow, &oldXPos, &oldYPos);
					mousePrevPressed = true;
				}
				else {
					glfwGetCursorPos(VKDK::DefaultWindow, &newXPos, &newYPos);
					yawVelocity += (oldXPos - newXPos) * rotationAcceleration;
					pitchVelocity += (oldYPos - newYPos) * rotationAcceleration;
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
					zoomVelocity += (oldYPos - newYPos) * zoomAcceleration * .1;
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
				transform.SetPosition(initialPos);
				transform.SetRotation(initialRot);
				pitchVelocity = yawVelocity = 0;
				zoomVelocity = 0;
			}
		}

		void update() {
			handleArrowKeys();
			handleMouse();
			handleZoom();
			handleReset();
			
			zoomVelocity -= zoomVelocity * zoomResistance;
			yawVelocity -= yawVelocity * rotateResistance;
			pitchVelocity -= pitchVelocity * rotateResistance;

			transform.AddPosition(glm::normalize(rotatePoint - transform.position) * zoomVelocity);

			glm::vec3 currentRight = transform.right;
			glm::vec3 currentUp = transform.up;

			transform.RotateAround(rotatePoint, currentRight, pitchVelocity);
			transform.RotateAround(rotatePoint, currentUp, yawVelocity);

			/* Construct object space lookat */
			V = glm::lookAt(transform.position, transform.position + transform.forward, transform.up);

			/* Create perspective transformation */
			P = glm::perspective(fov, aspectRatio, nearClippingPlane, farClippingPlane);
		}

		void setWindowSize(int width, int height) {
			float aspectRatio = width / (float)height;

			assert(aspectRatio > 0);
			if (aspectRatio > 0) {
				this->aspectRatio = aspectRatio;
			}
		}

		glm::mat4 getModel() {
			return glm::mat4(1.0); 
		}

		glm::mat4 getView() {
			return V;
		}

		glm::mat4 getProjection() {
			return P;
		}

	private: 
		glm::vec3 initialPos;
		glm::quat initialRot;
		glm::vec3 rotatePoint;

		float zoomVelocity = 0.0f;
		float zoomAcceleration = .1f;
		float zoomResistance = .1f;

		float yawVelocity = 0.0f;
		float pitchVelocity = 0.0f;
		float rotationAcceleration = 0.03f;
		float rotateResistance = .1f;

		bool mousePrevPressed = false;
		double oldXPos, oldYPos, newXPos, newYPos;

		glm::mat4 P, V;

		float fov = 45;
		float aspectRatio = 1.0;
		float nearClippingPlane = .1;
		float farClippingPlane = 100.0;
	};
}