//#pragma once
#include <thread>

#include "Entities/Entity.hpp"
#include "Components/Mesh.hpp"
#include "Components/Materials/Material.hpp"

#include "vkdk.hpp"
#include <unordered_map>


/* For some reason, if i dont use standard here, the system namespace doesn't appear anywhere else */
using namespace std;

namespace System {
//using namespace Entities;
//using namespace Materials;
	
	/* Threads */
	extern thread *UpdateThread;
	extern thread *RaycastThread;
	extern bool quit;

	extern int UpdateRate;
	extern int FrameRate;
	extern int currentImageIndex;

	extern Entities::Entity World;
	//extern Entity Screen;

	/* The entity whose forward vector is used for ray casting. */
	//extern std::shared_ptr<Entity> Raycaster;
	//extern bool MouseDown[5]; // TODO: fix this kludge


	/* To do: add camera here */

	//extern unordered_map<string, std::shared_ptr<Texture>> TextureList;
	//extern unordered_map<std::string, std::shared_ptr<Components::Materials::Material>> MaterialList;
	extern unordered_map<std::string, std::shared_ptr<Components::Mesh>> MeshList;

	extern void UpdateLoop();
	extern void RaycastLoop();
	extern void RenderLoop();

	extern void SetupOpenGLState();
	extern void SetupComponents();
	extern void SetupEntities();
	extern void Start();
	extern void Terminate();
};