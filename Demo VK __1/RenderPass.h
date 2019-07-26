#pragma once

#include <array>
#include <chrono>

#include "Vulkan.h"
#include "Pipeline.h"
#include "Mesh.h"
#include "Text.h"
#include "Camera.h"

struct UniformBufferObjectMatrices
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;

	VkBuffer uniformBuffer;
	VkDeviceMemory uniformBufferMemory;

	void cleanup(VkDevice device)
	{
		vkDestroyBuffer(device, uniformBuffer, nullptr);
		vkFreeMemory(device, uniformBufferMemory, nullptr);
	}
};

const int MAX_POINTLIGHTS = 32;
const int MAX_DIRLIGHTS = 1;

struct UniformBufferObjectLights
{
	glm::vec4 camPos = glm::vec4(2.0f);

	std::array<glm::vec4, MAX_POINTLIGHTS> pointLightsPositions;
	std::array<glm::vec4, MAX_POINTLIGHTS> pointLightsColors;
	uint32_t nbPointLights = 0;

	std::array<glm::vec4, MAX_DIRLIGHTS> dirLightsDirections;
	std::array<glm::vec4, MAX_DIRLIGHTS> dirLightsColors;
	uint32_t nbDirLights = 0;

	VkBuffer uniformBuffer;
	VkDeviceMemory uniformBufferMemory;

	void cleanup(VkDevice device)
	{
		vkDestroyBuffer(device, uniformBuffer, nullptr);
		vkFreeMemory(device, uniformBufferMemory, nullptr);
	}
};

class RenderPass
{
public:
	void initialize(Vulkan* vk, bool createFrameBuffer = false, VkExtent2D extent = { 0, 0 }, bool present = true, VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT);
	UniformBufferObjectMatrices createUboMatrices(Vulkan * vk);
	UniformBufferObjectLights createUboLights(Vulkan * vk);

	int addMesh(Vulkan * vk, std::vector<Mesh*> mesh, std::string vertPath, std::string fragPath, int nbUbo, int nbTexture);
	int addText(Vulkan * vk, Text * text);
	int addPointLight(Vulkan * vk, glm::vec3 position, glm::vec3 color);
	int addDirLight(Vulkan* vk, glm::vec3 direction, glm::vec3 color);

	void recordDraw(Vulkan * vk);

	void updateUniformBuffer(Vulkan * vk);
	void drawCall(Vulkan * vk);

	void cleanup(Vulkan * vk);

private:
	void createRenderPass(VkDevice device, VkImageLayout finalLayout);
	void createColorResources(Vulkan * vk);
	VkDescriptorSetLayout createDescriptorSetLayout(VkDevice device, int nbUbo, int nbTexture);
	void createDescriptorPool(VkDevice device);
	VkDescriptorSet createDescriptorSet(VkDevice device, VkDescriptorSetLayout decriptorSetLayout, std::vector<VkImageView> imageView,
		VkSampler sampler, std::vector<VkBuffer> uniformBuffer, int nbUbo, int nbTexture, bool useUboLights = true);
	void fillCommandBuffer(Vulkan * vk);
	void drawFrame(Vulkan * vk);

public:
	VkRenderPass GetRenderPass() { return m_renderPass; }
	FrameBuffer GetFrameBuffer() { return m_frameBuffer; }

private:
	VkFormat m_format;
	VkFormat m_depthFormat;

	VkRenderPass m_renderPass;
	VkDescriptorPool m_descriptorPool;
	
	std::vector<MeshPipeline> m_meshesPipeline;

	std::vector<int> m_textID;
	Text * m_text = nullptr;
	std::vector<Mesh*> m_meshes;
	std::vector<UniformBufferObjectMatrices> m_ubosMatrices;
	UniformBufferObjectLights m_uboLights;

	Pipeline m_textPipeline;
	VkDescriptorSetLayout m_textDescriptorSetLayout;

	int m_commandBufferStatic;
	std::vector<int> m_commandBufferDynamics;

	bool m_useSwapChain = true;
	bool m_firstDraw = true;
	FrameBuffer m_frameBuffer;
	VkExtent2D m_extent;
	VkCommandBuffer m_commandBuffer;
	VkSemaphore m_renderCompleteSemaphore;
	VkCommandPool m_commandPool;

	VkSampleCountFlagBits m_msaaSamples = VK_SAMPLE_COUNT_1_BIT;
	VkImage m_colorImage;
	VkDeviceMemory m_colorImageMemory;
	VkImageView m_colorImageView;

	Camera m_camera;
};

