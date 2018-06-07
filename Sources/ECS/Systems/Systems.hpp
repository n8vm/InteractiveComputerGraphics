// ┌──────────────────────────────────────────────────────────────────┐
// │ Developer : n8vm                                                 |
// │  Workers: Handles common callbacks for rendering, updating,      |
// |    polling events, raycasting, and potentially more.             |
// └──────────────────────────────────────────────────────────────────┘
#pragma once

#include "Systems/Engine.hpp"
#include <thread>
#include <atomic>
#include <functional>

namespace Systems {
	extern std::unordered_map<std::string, std::shared_ptr<Engine>> Engines;

	

	enum SystemTypes {
		Update, Event, Render, Raycast, None
	};

	static inline SystemTypes currentThreadType = Event;
	static inline std::function<void()> UpdateSystem;
	static inline std::function<void()> EventSystem;
	static inline std::function<void()> RenderSystem;
	static inline std::function<void()> RaycastSystem;
	
	/* Threads */
	static inline std::thread *UpdateThread;
	static inline std::thread *EventThread;
	static inline std::thread *RenderThread;
	static inline std::thread *RaycastThread;
	
	static inline int UpdateRate = 90;
	static inline int FrameRate = 90;
	static inline std::atomic<bool> quit = false;

	/* Starts and stops worker threads. */
	inline void LaunchThreads() {
		quit = false;
		/* Call the callbacks */
		if (UpdateSystem && currentThreadType != Update)
			UpdateThread = new std::thread(UpdateSystem);

		if (EventSystem && currentThreadType != Event)
			EventThread = new std::thread(EventSystem);

		if (RaycastSystem && currentThreadType != Raycast)
			RaycastThread = new std::thread(RaycastSystem);

		if (RenderSystem && currentThreadType != Render)
			RenderThread = new std::thread(RenderSystem);

		/* Call the appropriate callback on this thread */
		switch (currentThreadType) {
		case Update: UpdateSystem(); break;
		case Render: RenderSystem(); break;
		case Event: EventSystem(); break;
		case Raycast: RaycastSystem(); break;
		default: break;
		}
	}

	inline void Stop() { quit = true; }

	inline void JoinThreads() {
		if (UpdateThread) UpdateThread->join();
		if (EventThread) EventThread->join();
		if (RaycastThread) RaycastThread->join();
		if (RenderThread) RenderThread->join();
	}
}