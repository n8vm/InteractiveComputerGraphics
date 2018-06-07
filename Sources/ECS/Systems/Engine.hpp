#pragma once

#include <thread>

class Engine {
	std::function<void()> ReadUpdate;
	std::function<void()> WriteUpdate;
};