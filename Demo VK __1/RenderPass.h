#pragma once

#include <array>
#include <chrono>

#include "Vulkan.h"
#include "Pipeline.h"
#include "Mesh.h"
#include "Text.h"
#include "UniformBufferObject.h"
#include "Instance.h"

struct MeshRender
{
	MeshPBR* mesh;
	std::vector<UboBase*> ubos;
	Instance* instance = nullptr;
};

class RenderPass
{
public:
	void initialize(Vulkan* vk, bool createFrameBuffer = false, VkExtent2D extent = { 0, 0 }, bool present = true, VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT);

	int addMesh(Vulkan * vk, std::vector<MeshRender> mesh, std::string vertPath, std::string fragPath, int nbTexture);
	int addMeshInstanced(Vulkan* vk, std::vector<MeshRender> meshes, std::string vertPath, std::string fragPath, int nbTexture);
	int addText(Vulkan * vk, Text * text);

	void recordDraw(Vulkan * vk);

	void drawCall(Vulkan * vk);

	void cleanup(Vulkan * vk);

private:
	void createRenderPass(VkDevice device, VkImageLayout finalLayout);
	void createColorResources(Vulkan * vk);
	VkDescriptorSetLayout createDescriptorSetLayout(VkDevice device, std::vector<UboBase*> uniformBuffers, int nbTexture);
	void createDescriptorPool(VkDevice device);
	VkDescriptorSet createDescriptorSet(VkDevice device, VkDescriptorSetLayout decriptorSetLayout, std::vector<VkImageView> imageView,
		VkSampler sampler, std::vector<UboBase*> uniformBuffers, int nbTexture);
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
	std::vector<MeshRender> m_meshes;

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
};

