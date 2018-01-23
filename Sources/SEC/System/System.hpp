#pragma once
#include <thread>

//#include "Entities/Entity.h"
#include <unordered_map>

//using namespace Entities;
//using namespace Materials;
using namespace std;

namespace System {
	/* Threads */
	extern thread *UpdateThread;
	extern thread *RaycastThread;
	extern bool quit;

	extern int UpdateRate;
	extern int FrameRate;

	//extern Entity World;
	//extern Entity Screen;

	/* The entity whose forward vector is used for ray casting. */
	//extern std::shared_ptr<Entity> Raycaster;
	//extern bool MouseDown[5]; // TODO: fix this kludge


	/* To do: add camera here */

	//extern unordered_map<string, std::shared_ptr<Texture>> TextureList;
	//extern unordered_map<string, std::shared_ptr<Material>> MaterialList;
	//extern unordered_map<string, std::shared_ptr<obj::Model>> ModelList;

	extern void UpdateLoop();
	extern void RaycastLoop();
	extern void RenderLoop();

	extern void SetupOpenGLState();
	extern void SetupComponents();
	extern void SetupEntities();
	extern void Start();
	extern void Terminate();
};