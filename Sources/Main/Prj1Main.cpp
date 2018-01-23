#include "vkdk.hpp"
#include "System/System.hpp"
#include "glm/glm.hpp"

glm::vec3 hsvToRgb(glm::vec3 hsv) {
	glm::vec3 rgb;
	int i = floorf(hsv[0] * 6.0);
	float f = hsv[0] * 6 - i;
	float p = hsv[2] * (1 - hsv[1]);
	float q = hsv[2] * (1 - f * hsv[1]);
	float t = hsv[2] * (1 - (1 - f) * hsv[1]);

	switch (i % 6) {
	case 0: rgb[0] = hsv[2], rgb[1] = t, rgb[2] = p; break;
	case 1: rgb[0] = q, rgb[1] = hsv[2], rgb[2] = p; break;
	case 2: rgb[0] = p, rgb[1] = hsv[2], rgb[2] = t; break;
	case 3: rgb[0] = p, rgb[1] = q, rgb[2] = hsv[2]; break;
	case 4: rgb[0] = t, rgb[1] = p, rgb[2] = hsv[2]; break;
	case 5: rgb[0] = hsv[2], rgb[1] = p, rgb[2] = q; break;
	}

	return rgb;
}

float x = 0.0;
glm::vec3 color;
void System::UpdateLoop() {
	while (!System::quit) {
		x += .0000001;
		if (x >= 1.0) x = 0;
		color = hsvToRgb(glm::vec3(x, 1.0, 1.0));
	}
}

void System::RenderLoop() {
	while (glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_ESCAPE) != GLFW_PRESS && !glfwWindowShouldClose(VKDK::DefaultWindow)) {
		VKDK::SetClearColor(color.r, color.g, color.b, 1.0);
		glfwPollEvents();
		VKDK::TestDraw1();
	}

	vkDeviceWaitIdle(VKDK::device);
}

void System::Start() {
	if (System::quit) return;
	
	/* Update on seperate thread */
	System::UpdateThread = new std::thread(System::UpdateLoop);

	/* Render on the current thread */
	System::RenderLoop();
}

void System::Terminate() {
	/* Quit */
	System::quit = true;

	System::UpdateThread->join();
	
	delete(System::UpdateThread);
}

int main() {
		VKDK::InitializationParameters vkdkParams = { 512, 512, "Project 1 - Hello World", false, false, true };
		if (VKDK::Initialize(vkdkParams) != VK_SUCCESS) System::quit = true;

		System::Start();
		System::Terminate();

		VKDK::Terminate();	
}