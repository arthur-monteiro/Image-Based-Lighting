#include "System.h"

System::System()
{
}

System::~System()
{
	//cleanup();
}

void System::initialize()
{
	create();
}

bool System::mainLoop()
{
	while (!glfwWindowShouldClose(m_vk.GetWindow()))
	{
		glfwPollEvents();

		m_swapChainRenderPass.updateUniformBuffer(&m_vk);

		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		m_fpsCount++;
		if (std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - m_startTimeFPSCounter).count() > 1000.0f)
		{
			std::wstring text = L"FPS : " + std::to_wstring(m_fpsCount);
			m_text.changeText(&m_vk, text, m_fpsCounterTextID);

			m_fpsCount = 0;
			m_startTimeFPSCounter = currentTime;
		}

		float time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;

		m_swapChainRenderPass.drawCall(&m_vk);
	}

	vkDeviceWaitIdle(m_vk.GetDevice());

	return true;
}

void System::cleanup()
{
	m_vk.cleanup();
	m_swapChainRenderPass.cleanup(&m_vk);
}

void System::create(bool recreate)
{
	m_vk.initialize(1066, 600, "Vulkan Demo", recreateCallback, (void*)this, recreate);

	if (!recreate)
		createRessources();

	createPasses(recreate);
}

void System::createRessources()
{
	m_text.initialize(&m_vk, 48, "Fonts/arial.ttf");
	m_fpsCounterTextID = m_text.addText(&m_vk, L"FPS : 0", glm::vec2(-0.99f, 0.85f), 0.065f);

	m_meshes.resize(10);
	for (int i(0); i < m_meshes.size(); ++i)
		m_meshes[i] = std::unique_ptr<Mesh>(new Mesh);

	/*m_meshes[0]->loadObj(&m_vk, "Models/lantern_obj.obj");
	m_meshes[0]->loadTexture(&m_vk, { "Textures/lantern_Base_Color.jpg", "Textures/lantern_Normal_OpenGL.jpg",  "Textures/lantern_Roughness.jpg",
		"Textures/lantern_Metallic.jpg", "Textures/lantern_Mixed_AO.jpg" });*/

	m_meshes[0]->loadObj(&m_vk, "Models/cube.obj");
	m_meshes[0]->loadTexture(&m_vk, { "Textures/bamboo-wood-semigloss-albedo.png", "Textures/bamboo-wood-semigloss-normal.png",  "Textures/bamboo-wood-semigloss-roughness.png",
		"Textures/bamboo-wood-semigloss-metal.png", "Textures/bamboo-wood-semigloss-ao.png" });
}

void System::createPasses(bool recreate)
{
	if(recreate) m_swapChainRenderPass.cleanup(&m_vk);
	m_swapChainRenderPass.initialize(&m_vk, false, { 0, 0 }, true, VK_SAMPLE_COUNT_8_BIT);

	// on envoi le pointeur pour modifier la matrice model où on veut
	m_swapChainRenderPass.addText(&m_vk, &m_text);

	std::vector<std::pair<glm::vec3, glm::vec3>> pointLights;
	pointLights.push_back({ glm::vec3(1.5f, 0.5f, -0.5f), glm::vec3(10.0f, 0.0f, 0.0f) });
	pointLights.push_back({ glm::vec3(-1.5f, 0.5f, -0.5f), glm::vec3(0.0f, 10.0f, 0.0f) });
	//pointLights.push_back({ glm::vec3(0.0f, 0.5f, -1.5f), glm::vec3(0.0f, 0.0f, 10.0f) });
	//pointLights.push_back({ glm::vec3(0.0f, 1.5f, 0.0f), glm::vec3(1.0f) });
	//pointLights.push_back({ glm::vec3(0.0f, -0.5f, -1.5f), glm::vec3(10.0f, 10.0f, 0.0f) });

	std::vector<Mesh*> spheres;
	for (int i(0); i < pointLights.size(); ++i)
	{
		m_swapChainRenderPass.addPointLight(&m_vk, pointLights[i].first, pointLights[i].second);

		m_meshes[i + 1]->loadObj(&m_vk, "Models/sphere.obj", pointLights[i].second);

		m_meshes[i + 1]->restoreTransformations();
		m_meshes[i + 1]->translate(pointLights[i].first);
		m_meshes[i + 1]->scale(glm::vec3(0.002f));

		spheres.push_back(m_meshes[i + 1].get());
	}

	//m_meshes[0].get()->SetImageView(0, m_offScreenRenderPass.GetFrameBuffer().imageView);
	m_swapChainRenderPass.addMesh(&m_vk, { m_meshes[0].get() }, "Shaders/vert.spv", "Shaders/frag.spv", 1, 5);
	m_swapChainRenderPass.addMesh(&m_vk, spheres, "Shaders/vertSphere.spv", "Shaders/fragSphere.spv", 1, 0);
	m_swapChainRenderPass.recordDraw(&m_vk);

	m_meshes[0]->restoreTransformations();
	//m_meshes[0]->translate(glm::vec3(0.0f, -0.8f, 0.0f));
	//m_meshes[0]->scale(glm::vec3(0.02f));

	m_swapChainRenderPass.updateUniformBuffer(&m_vk);
}