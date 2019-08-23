#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>
#include <chrono>
#include <memory>
#include <algorithm>

#include "Vulkan.h"
#include "RenderPass.h"
#include "Mesh.h"
#include "Text.h"
#include "UniformBufferObject.h"
#include "Camera.h"
#include "Instance.h"

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
	MeshPBR m_skybox;
	MeshPBR m_sphere;
	Instance m_sphereInstance;
	std::vector<MeshPBR> m_sphereMeshes;
	std::vector<MeshPBR> m_spherelightMeshes;
	Text m_text;

	int m_fpsCounterTextID = -1;
	int m_fpsCount = 0;
	std::chrono::steady_clock::time_point m_startTimeFPSCounter = std::chrono::steady_clock::now();

	double m_oldMousePosX = 0;
	double m_oldMousePosY = 0;
	bool m_wasClickPressed = 0;

	int m_skyboxID;

	UniformBufferObject<UniformBufferObjectVP> m_uboVP;
	UniformBufferObjectVP m_uboVPData;
	UniformBufferObject<UniformBufferObjectVP> m_uboVPSkybox;
	UniformBufferObjectVP m_uboVPSkyboxData;
	UniformBufferObject<UniformBufferObjectLights> m_uboLight;
	UniformBufferObjectLights m_uboLightsData;
	UniformBufferObject<UniformBufferObjectModel> m_uboModelBox;
	std::vector<UniformBufferObject<UniformBufferObjectModel>> m_uboSpheres;

	Camera m_camera;
};

