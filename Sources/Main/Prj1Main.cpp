// ┌──────────────────────────────────────────────────────────────────┐
// │ Developer : n8vm - Hello World                                   |
// │   Simple example of how to setup VKDK, Systems, add a component, |
// |   to clear the screen                                            |
// └──────────────────────────────────────────────────────────────────┘

#include "vkdk.hpp"
#include "ecs.hpp"

namespace ComponentManager = Systems::ComponentManager;
namespace Math = Components::Math;

namespace PRJ1 {

	void SetupComponents() {
		/* When setting up components, we always initialize the component manager, which adds a couple
			placeholder components, like a couple default textures, a default model, etc. */
		ComponentManager::Initialize();
	
		/* We'll then create a perspective for each Vulkan swapchain framebuffer. */
		Math::Perspective::Create("My Perspective",
			VKDK::renderPass, VKDK::drawCmdBuffers, VKDK::swapChainFramebuffers, 
			VKDK::swapChainExtent.width, VKDK::swapChainExtent.height);
	}

	void SetupEntities() {
		// Does nothing in this example. See PRJ2 - Transformations
	}

	void SetupSystems() {
		/* Systems are implemented using lambda functions.
			In general, we'll always have a render system and an event system. */

		Systems::RenderSystem = []() {
			bool refreshRequired = false;

			while (!VKDK::ShouldClose() && !Systems::quit) {
				/* Aquire a new image from the swapchain */
				refreshRequired |= VKDK::PrepareFrame();

				/* Kind of kludgy, but re-record the render pass each frame to update clear color. */
				glm::vec4 newColor = glm::vec4(Colors::hsvToRgb(glm::vec3(glfwGetTime() * .1, 1.0, 1.0)), 1.0);
				ComponentManager::Perspectives["My Perspective"]->recordRenderPass(newColor);

				/* Submit command buffer to graphics queue  */
				VKDK::SubmitToGraphicsQueueInfo info;
				info.commandBuffers = { VKDK::drawCmdBuffers[VKDK::swapIndex] };
				info.graphicsQueue = VKDK::graphicsQueue;
				info.signalSemaphores = { VKDK::semaphores.renderComplete };
				info.waitSemaphores = { VKDK::semaphores.presentComplete };
				VKDK::SubmitToGraphicsQueue(info);

				/* Submit the frame for presenting. */
				refreshRequired |= VKDK::SubmitFrame();

				/* If something like the screen size changed, recreate the swapchain and refresh the final perspective. */
				if (refreshRequired) {
					VKDK::RecreateSwapChain();

					/* Add a perspective to render the swapchain */
					ComponentManager::Perspectives["My Perspective"]->refresh(VKDK::renderPass, VKDK::drawCmdBuffers, VKDK::swapChainFramebuffers,
						VKDK::swapChainExtent.width, VKDK::swapChainExtent.height);
					refreshRequired = false;
				}
			}
			vkDeviceWaitIdle(VKDK::device);
		};

		Systems::EventSystem = []() {
			while (!VKDK::ShouldClose() && !Systems::quit) {
				glfwPollEvents();
			}
		};

		/* All systems are done asynchronously, but one system can be ran on the main thread.
			In this case, we'll run the event system on the main thread. */
		Systems::currentThreadType = Systems::SystemTypes::Event;
	}
}

void StartDemo1() {
	/* All these Vulkan demos will look similar to this. */

	/* First, I use a namespace called Vulkan Development Kit (VKDK), which can be initialized with the 
		InitializationParameters struct. VKDK contains a ton of boiler plate code, and loosely follows 
		Alexander Overvoorde's vulkan tutorials. */
	VKDK::InitializationParameters vkdkParams = { 1024, 1024, "Project 1 - Hello World", false, false, true };
	if (VKDK::Initialize(vkdkParams) != VK_SUCCESS) return;

	/* Then, we setup the components, then entities (none here), and then finally the systems. */
	PRJ1::SetupComponents();
	PRJ1::SetupEntities();
	PRJ1::SetupSystems();
	Systems::LaunchThreads();

	/* Once the systems return, we cleanup. */
	Systems::JoinThreads();
	ComponentManager::Cleanup();
	VKDK::Terminate();	
}

#ifndef NO_MAIN
int main() { StartDemo1(); }
#endif