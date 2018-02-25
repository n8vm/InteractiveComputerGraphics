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

void System::EventLoop() {
	while (glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_ESCAPE) != GLFW_PRESS && !glfwWindowShouldClose(VKDK::DefaultWindow)) {
		glfwPollEvents();
	}
}

float x = 0.0;
glm::vec4 clearColor;
void System::UpdateLoop() {
	auto lastTime = glfwGetTime();
	while (glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_ESCAPE) != GLFW_PRESS && !glfwWindowShouldClose(VKDK::DefaultWindow)) {
		auto currentTime = glfwGetTime();
		if (currentTime - lastTime > 1.0 / UpdateRate) {
			x += .001;
			if (x >= 1.0) x = 0;
			clearColor = glm::vec4(hsvToRgb(glm::vec3(x, 1.0, 1.0)), 0.0);
			lastTime = currentTime;
		}
	}
}

void System::RenderLoop() {
	using namespace VKDK;
	while (glfwGetKey(VKDK::DefaultWindow, GLFW_KEY_ESCAPE) != GLFW_PRESS && !glfwWindowShouldClose(VKDK::DefaultWindow)) {
		/* Aquire a new image from the swapchain */
		PrepareFrame();
		
		/* Kind of kludgy, but re-record command buffers to update clear color. */
		System::MainScene->setClearColor(clearColor);
		System::MainScene->recordRenderPass();

		/* Submit to graphics queue  */
		Scene::SubmitToGraphicsQueueInfo info;
		info.commandBuffers = { VKDK::drawCmdBuffers[VKDK::swapIndex] };
		info.graphicsQueue = VKDK::graphicsQueue;
		info.signalSemaphores = { VKDK::semaphores.renderComplete };
		info.waitSemaphores = { VKDK::semaphores.presentComplete };
		System::MainScene->submitToGraphicsQueue(info);

		/* Submit the frame for presenting. */
		SubmitFrame();
	}

	vkDeviceWaitIdle(device);
}

void System::Start() {
	if (System::quit) return;
	
	/* Render on seperate thread */
	System::RenderThread = new std::thread(System::RenderLoop);

	/* Render on seperate thread */
	System::UpdateThread = new std::thread(System::UpdateLoop);

	/* Update events on the current thread */
	System::EventLoop();
}

void System::Cleanup() {
	/* Quit */
	System::quit = true;

	System::RenderThread->join();
	System::UpdateThread->join();
	
	delete(System::RenderThread);
	delete(System::UpdateThread);
}

void System::Initialize() {
	MainScene = make_shared<Scene>(true);
}

void System::Terminate() {
	MainScene->cleanup();
}

int main() {
		VKDK::InitializationParameters vkdkParams = { 1024, 1024, "Project 1 - Hello World", false, false, true };
		if (VKDK::Initialize(vkdkParams) != VK_SUCCESS) System::quit = true;

		System::Initialize();
		System::Start();
		System::Terminate();
		System::Cleanup();

		VKDK::Terminate();	
}