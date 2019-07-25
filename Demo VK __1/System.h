#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <chrono>
#include <memory>

#include "Vulkan.h"
#include "RenderPass.h"
#include "Mesh.h"
#include "Text.h"

class System
{
public:
	System();
	~System();

	void initialize();
	bool mainLoop();
	void cleanup();

	static void recreateCallback(void* instance) { reinterpret_cast<System*>(instance)->create(true); }
private:
	void create(bool recreate = false);
	void createRessources();
	void createPasses(bool recreate = false);

private:
	Vulkan m_vk;

	RenderPass m_swapChainRenderPass;
	std::vector<std::unique_ptr<Mesh>> m_meshes;
	Text m_text;

	int m_fpsCounterTextID;
	int m_fpsCount;
	std::chrono::steady_clock::time_point m_startTimeFPSCounter = std::chrono::high_resolution_clock::now();

	double m_oldMousePosX = 0;
	double m_oldMousePosY = 0;
	bool m_wasClickPressed = 0;
};

