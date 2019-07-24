#include "System.h"

System::System()
{
}

System::~System()
{
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

		m_meshes[0]->restoreTransformations();
		m_meshes[0]->rotate(time * glm::radians(20.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		/*m_meshes[0]->translate(glm::vec3(0.0f, 0.0f, -0.3f));
		m_meshes[0]->scale(glm::vec3(1.5f));*/

		m_swapChainRenderPass.updateUniformBuffer(&m_vk);

		m_swapChainRenderPass.drawCall(&m_vk);
	}

	vkDeviceWaitIdle(m_vk.GetDevice());

	return true;
}

void System::cleanup()
{
	m_vk.cleanup();
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

	m_meshes.resize(2);
	for (int i(0); i < m_meshes.size(); ++i)
		m_meshes[i] = std::unique_ptr<Mesh>(new Mesh);

	m_meshes[0]->loadObj(&m_vk, "Models/cube.obj");
	m_meshes[0]->loadTexture(&m_vk, { "Textures/bamboo-wood-semigloss-albedo.png", "Textures/bamboo-wood-semigloss-normal.png",  "Textures/bamboo-wood-semigloss-roughness.png",
		"Textures/bamboo-wood-semigloss-metal.png", "Textures/bamboo-wood-semigloss-ao.png" });
}

void System::createPasses(bool recreate)
{
	if(recreate) m_swapChainRenderPass.cleanup(&m_vk);
	m_swapChainRenderPass.initialize(&m_vk);

	// on envoi le pointeur pour modifier la matrice model où on veut
	m_swapChainRenderPass.addText(&m_vk, &m_text);

	m_swapChainRenderPass.addPointLight(&m_vk, glm::vec3(1.0f), glm::vec3(2.0f));
	m_swapChainRenderPass.addPointLight(&m_vk, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f));

	//m_meshes[0].get()->SetImageView(0, m_offScreenRenderPass.GetFrameBuffer().imageView);
	m_swapChainRenderPass.addMesh(&m_vk, std::vector<Mesh*>(1, m_meshes[0].get()), "Shaders/vert.spv", "Shaders/frag.spv", 1, 5);
	m_swapChainRenderPass.recordDraw(&m_vk);
}
