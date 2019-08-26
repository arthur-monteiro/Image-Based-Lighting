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
		m_camera.update(m_vk.GetWindow());

		m_uboVPData.view = m_camera.getViewMatrix();
		m_uboVP.update(&m_vk, m_uboVPData);

		m_uboVPSkyboxData.view = glm::mat4(glm::mat3(m_camera.getViewMatrix()));
		m_uboVPSkybox.update(&m_vk, m_uboVPSkyboxData);

		m_uboLightsData.camPos = glm::vec4(m_camera.getPosition(), 1.0f);
		m_uboLight.update(&m_vk, m_uboLightsData);

		static auto startTime = std::chrono::steady_clock::now();

		auto currentTime = std::chrono::steady_clock::now();
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

	vkDeviceWaitIdle(m_vk.getDevice());

	return true;
}

void System::cleanup()
{
	std::cout << "Cleanup..." << std::endl;

	for (int i(0); i < m_spherelightMeshes.size(); ++i)
		m_spherelightMeshes[i].cleanup(m_vk.getDevice());

	m_skybox.cleanup(m_vk.getDevice());
	m_sphere.cleanup(m_vk.getDevice());

	m_swapChainRenderPass.cleanup(&m_vk);

	m_vk.cleanup();
}

void System::create(bool recreate)
{
	m_vk.initialize(1066, 600, "Vulkan Demo", recreateCallback, (void*)this, recreate);

	if (!recreate)
	{
		createRessources();
		m_camera.initialize(glm::vec3(0.0f, 2.0f, -2.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), 0.01f, 2.0f);
	}

	createPasses(recreate);
}

void System::createRessources()
{
	m_text.initialize(&m_vk, 48, "Fonts/arial.ttf");
	m_fpsCounterTextID = m_text.addText(&m_vk, L"FPS : 0", glm::vec2(-0.99f, 0.85f), 0.065f);

	m_sphere.loadObj(&m_vk, "Models/sphere.obj");
	//m_meshes[0]->loadTextureFromFile(&m_vk, { "Textures/bamboo-wood-semigloss-albedo.png", "Textures/bamboo-wood-semigloss-normal.png",  "Textures/bamboo-wood-semigloss-roughness.png",
	//	"Textures/bamboo-wood-semigloss-metal.png", "Textures/bamboo-wood-semigloss-ao.png" });

	m_skybox.loadObj(&m_vk, "Models/cube.obj");
	//m_skybox.loadCubemapFromFile(&m_vk, { "Textures/skybox/right.jpg", "Textures/skybox/left.jpg", "Textures/skybox/top.jpg", "Textures/skybox/bottom.jpg", "Textures/skybox/front.jpg",
	//	"Textures/skybox/back.jpg" });
	//m_meshes[1]->loadHDRTexture(&m_vk, { "Textures/simons_town_rocks_4k.hdr" });

	RenderPass tempCubemapCreation;
#define CUBEMAP_SIZE_X 1024
	tempCubemapCreation.initialize(&m_vk, true, { CUBEMAP_SIZE_X, CUBEMAP_SIZE_X }, false, VK_SAMPLE_COUNT_8_BIT, 6);

	MeshPBR tempCube;
	tempCube.loadObj(&m_vk, "Models/cube.obj");
	tempCube.loadHDRTexture(&m_vk, { "Textures/simons_town_rocks_4k.hdr" });

	glm::mat4 captureViews[] =
	{
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
		glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
	};
	std::vector<UniformBufferObject<UniformBufferObjectVP>> tempUboVP(6);
	for (int i(0); i < 6; ++i)
	{
		UniformBufferObjectVP tempUboVPData;
		tempUboVPData.proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
		tempUboVPData.view = captureViews[i];
		
		tempUboVP[i].load(&m_vk, tempUboVPData, VK_SHADER_STAGE_VERTEX_BIT);

		tempCubemapCreation.addMesh(&m_vk, { { &tempCube, { &tempUboVP[i] } } }, "Shaders/vertCubemapCreation.spv", "Shaders/fragCubemapCreation.spv", 1, i);
	}

	tempCubemapCreation.recordDraw(&m_vk);
	tempCubemapCreation.drawCall(&m_vk);

	vkQueueWaitIdle(m_vk.getGraphicalQueue());

	m_skybox.loadCubemapFromImages(&m_vk, { tempCubemapCreation.getFrameBuffer(0).image, tempCubemapCreation.getFrameBuffer(1).image, tempCubemapCreation.getFrameBuffer(2).image,
		tempCubemapCreation.getFrameBuffer(3).image , tempCubemapCreation.getFrameBuffer(4).image , tempCubemapCreation.getFrameBuffer(5).image }, CUBEMAP_SIZE_X, CUBEMAP_SIZE_X);
	tempCubemapCreation.cleanup(&m_vk);
	tempCube.cleanup(m_vk.getDevice());

	RenderPass tempConvolutionCreation;
	tempConvolutionCreation.initialize(&m_vk, true, { 32, 32 }, false, VK_SAMPLE_COUNT_8_BIT, 6);
	for (int i(0); i < 6; ++i)
	{
		tempConvolutionCreation.addMesh(&m_vk, { { &m_skybox, { &tempUboVP[i] } } }, "Shaders/vertConvolution.spv", "Shaders/fragConvolution.spv", 1, i);
	}
	tempConvolutionCreation.recordDraw(&m_vk);
	tempConvolutionCreation.drawCall(&m_vk);

	vkQueueWaitIdle(m_vk.getGraphicalQueue());

	m_sphere.loadCubemapFromImages(&m_vk, { tempConvolutionCreation.getFrameBuffer(0).image, tempConvolutionCreation.getFrameBuffer(1).image, tempConvolutionCreation.getFrameBuffer(2).image,
		tempConvolutionCreation.getFrameBuffer(3).image , tempConvolutionCreation.getFrameBuffer(4).image , tempConvolutionCreation.getFrameBuffer(5).image }, 32, 32);
	tempConvolutionCreation.cleanup(&m_vk);

#define MAX_REFLECTION_TEXTURE_SIZE_X 256
	UniformBufferObject<UniformBufferSingleFloat> uboRoughness;
	UniformBufferSingleFloat uboRoughnessData;
	uboRoughnessData.floatVal = 0.0f;
	uboRoughness.load(&m_vk, uboRoughnessData, VK_SHADER_STAGE_FRAGMENT_BIT);

	//int texID = m_sphere.createTexture(&m_vk, MAX_REFLECTION_TEXTURE_SIZE_X, MAX_REFLECTION_TEXTURE_SIZE_X, 5, 6);
	//for (int mipLevel = 0; mipLevel < 5; ++mipLevel)
	//{
	int mipLevel = 0;

	RenderPass tempReflectionConvolutionCreation;
	VkExtent2D extent = { MAX_REFLECTION_TEXTURE_SIZE_X * std::pow(0.5, mipLevel), MAX_REFLECTION_TEXTURE_SIZE_X * std::pow(0.5, mipLevel) };
	tempReflectionConvolutionCreation.initialize(&m_vk, true, extent, false, VK_SAMPLE_COUNT_8_BIT, 6);
	uboRoughnessData.floatVal = (float)mipLevel / 4.0f;
	uboRoughness.update(&m_vk, uboRoughnessData);
	for (int face(0); face < 6; ++face)
	{
		tempReflectionConvolutionCreation.addMesh(&m_vk, { { &m_skybox, { &tempUboVP[face], &uboRoughness } } }, "Shaders/vert.spv", "Shaders/frag.spv", 1, face);
	}
	tempReflectionConvolutionCreation.recordDraw(&m_vk);
	tempReflectionConvolutionCreation.drawCall(&m_vk);

	vkQueueWaitIdle(m_vk.getGraphicalQueue());
	m_sphere.loadCubemapFromImages(&m_vk, { tempReflectionConvolutionCreation.getFrameBuffer(0).image, tempReflectionConvolutionCreation.getFrameBuffer(1).image,
		tempReflectionConvolutionCreation.getFrameBuffer(2).image,	tempReflectionConvolutionCreation.getFrameBuffer(3).image , tempReflectionConvolutionCreation.getFrameBuffer(4).image ,
		tempReflectionConvolutionCreation.getFrameBuffer(5).image }, extent.height, extent.width/*, texID, mipLevel*/);

	tempReflectionConvolutionCreation.cleanup(&m_vk);
	//}

	RenderPass tempBrdfLUTCreation;
#define BRDF_LUT_TEXTURE_SIZE_X 512
	tempBrdfLUTCreation.initialize(&m_vk, true, { BRDF_LUT_TEXTURE_SIZE_X, BRDF_LUT_TEXTURE_SIZE_X }, false, VK_SAMPLE_COUNT_8_BIT, 1);

	MeshPBR square;
	square.loadObj(&m_vk, "Models/square.obj", glm::vec3(0.0f, 0.0f, 1.0f));

	tempBrdfLUTCreation.addMesh(&m_vk, { {&square, { } } }, "Shaders/vertBrdfLUT.spv", "Shaders/fragbrdfLUT.spv", 0);
	tempBrdfLUTCreation.recordDraw(&m_vk);
	tempBrdfLUTCreation.drawCall(&m_vk);

	vkQueueWaitIdle(m_vk.getGraphicalQueue());

	m_sphere.loadTextureFromImages(&m_vk, { tempBrdfLUTCreation.getFrameBuffer(0).image }, BRDF_LUT_TEXTURE_SIZE_X, BRDF_LUT_TEXTURE_SIZE_X);

	tempBrdfLUTCreation.cleanup(&m_vk);
	square.cleanup(m_vk.getDevice());

	//m_skybox.setImageView(0, m_sphere.getImageView(1));
}

void System::createPasses(bool recreate)
{
	if(recreate) m_swapChainRenderPass.cleanup(&m_vk);
	m_swapChainRenderPass.initialize(&m_vk, false, { 0, 0 }, true, VK_SAMPLE_COUNT_8_BIT);

	// on envoi le pointeur pour modifier la matrice model où on veut
	m_swapChainRenderPass.addText(&m_vk, &m_text);

	std::vector<std::pair<glm::vec3, glm::vec3>> pointLights;
	pointLights.push_back({ glm::vec3(-1.f, 0.0f, -10.f), glm::vec3(100.0f) }); // pos , color
	pointLights.push_back({ glm::vec3(-5.0f, 1.0f, -10.f), glm::vec3(100.0f) });
	pointLights.push_back({ glm::vec3(-1.0f, 5.0f, -10.f), glm::vec3(100.0f) });
	pointLights.push_back({ glm::vec3(-5.0f, 5.0f, -10.f), glm::vec3(100.0f) });

	std::vector<MeshRender> spheres;
	m_uboSpheres.resize(pointLights.size());
	m_spherelightMeshes.resize(pointLights.size());
	for (int i(0); i < pointLights.size(); ++i)
	{
		m_spherelightMeshes[i].loadObj(&m_vk, "Models/sphere.obj", pointLights[i].second);

		m_spherelightMeshes[i].restoreTransformations();
		m_spherelightMeshes[i].translate(pointLights[i].first);
		m_spherelightMeshes[i].scale(glm::vec3(0.002f));

		MeshRender meshRender;
		meshRender.mesh = &m_spherelightMeshes[i];

		m_uboSpheres[i].load(&m_vk, { m_spherelightMeshes[i].getModelMatrix() }, VK_SHADER_STAGE_VERTEX_BIT);

		meshRender.ubos = { &m_uboSpheres[i], &m_uboVP };
		spheres.push_back(meshRender);
	}

	m_uboVPData.proj = glm::perspective(glm::radians(45.0f), m_vk.getSwapChainExtend().width / (float)m_vk.getSwapChainExtend().height, 0.1f, 100.0f);
	m_uboVPData.proj[1][1] *= -1;
	m_uboVPData.view = m_camera.getViewMatrix();
	m_uboVP.load(&m_vk, m_uboVPData, VK_SHADER_STAGE_VERTEX_BIT);

	m_uboVPSkyboxData.proj = glm::perspective(glm::radians(45.0f), m_vk.getSwapChainExtend().width / (float)m_vk.getSwapChainExtend().height, 0.1f, 10.0f);
	m_uboVPSkyboxData.proj[1][1] *= -1;
	m_uboVPSkyboxData.view = glm::mat4(glm::mat3(m_camera.getViewMatrix()));
	m_uboVPSkybox.load(&m_vk, m_uboVPSkyboxData, VK_SHADER_STAGE_VERTEX_BIT);

	m_uboLightsData.nbPointLights = pointLights.size();
	for (int i(0); i < pointLights.size(); ++i)
	{
		m_uboLightsData.pointLightsColors[i] = glm::vec4(pointLights[i].second, 1.0f);
		m_uboLightsData.pointLightsPositions[i] = glm::vec4(pointLights[i].first, 1.0f);
	}
	m_uboLight.load(&m_vk, m_uboLightsData, VK_SHADER_STAGE_FRAGMENT_BIT);

	std::vector <ModelInstance> perInstance;
	for (int i(0); i < 10; ++i)
	{
		for (int j(0); j < 10; ++j)
		{
			ModelInstance mi;
			mi.model = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(-i * 0.5f, j * 0.5f, 0.0f)), glm::vec3(0.01f));
			mi.albedo = glm::vec3(1.0f, 0.0f, 0.0f);
			mi.roughness = glm::clamp((float)i / 10.0f, 0.05f, 1.0f);
			mi.metallic = j / 10.0f;

			perInstance.push_back(mi);
		}
	}
	m_sphereInstance.load(&m_vk, sizeof(perInstance[0]) * perInstance.size(), perInstance.data());

	m_swapChainRenderPass.addMeshInstanced(&m_vk, { { &m_sphere, { &m_uboVP, &m_uboLight }, &m_sphereInstance } }, "Shaders/vertPBR.spv", "Shaders/fragPBR.spv", 3);
	m_swapChainRenderPass.addMesh(&m_vk, spheres, "Shaders/vertSphere.spv", "Shaders/fragSphere.spv", 0);
	m_skyboxID = m_swapChainRenderPass.addMesh(&m_vk, { { &m_skybox, { &m_uboVPSkybox } } }, "Shaders/vertSkybox.spv", "Shaders/fragSkybox.spv", 1);
	m_swapChainRenderPass.recordDraw(&m_vk);
}