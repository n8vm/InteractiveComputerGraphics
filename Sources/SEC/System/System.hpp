//#pragma once
#include <thread>

#include "Entities/Entity.hpp"
#include "Entities/Cameras/Camera.hpp"
#include "Components/Meshes/Mesh.hpp"
#include "Components/Materials/Material.hpp"
#include "Components/Textures/Texture.hpp"
#include "Entities/Lights/Light.hpp" // Might need to move this down a bit 

#include "vkdk.hpp"
#include <unordered_map>

#include "Scene.hpp"

/* For some reason, if i dont use standard here, the system namespace doesn't appear anywhere else */
using namespace std;

namespace System {
	
	/* Threads */
	extern thread *UpdateThread;
	extern thread *EventThread;
	extern thread *RenderThread;
	extern thread *RaycastThread;
	extern atomic<bool> quit;

	extern int UpdateRate;
	extern int FrameRate;

	extern std::shared_ptr<Scene> MainScene;

	extern unordered_map<string, std::shared_ptr<Components::Textures::Texture>> TextureList;
	extern unordered_map<std::string, std::shared_ptr<Components::Meshes::Mesh>> MeshList;

	extern void UpdateLoop();
	extern void RaycastLoop();
	extern void RenderLoop();
	extern void EventLoop();

	/* Call but dont define these */
	extern void Initialize();
	extern void Terminate();

	/* Define these */
	extern void SetupAdditionalRenderPasses();
	extern void SetupComponents();
	extern void SetupEntities();
	extern void Start();
	extern void Cleanup();
};