#pragma once

#include <unordered_map>

#include "Vulkan.h"
#include "Pipeline.h"

class MeshBase
{
public:
	virtual ~MeshBase() {}
};

class MeshPBR : public MeshBase
{
public:
	void loadObj(Vulkan * vk, std::string path, glm::vec3 forceNormal = glm::vec3(-1.0f));
	void loadTexture(Vulkan * vk, std::vector<std::string> path);
	void loadCubemap(Vulkan* vk, std::vector < std::string > path);
	void loadHDRTexture(Vulkan* vk, std::vector < std::string > path);

	void restoreTransformations() { m_modelMatrix = glm::mat4(1.0); }
	void rotate(float angle, glm::vec3 dir);
	void scale(glm::vec3 scale);
	void translate(glm::vec3 translation);

private:
	void createVertexBuffer(Vulkan * vk);
	void createIndexBuffer(Vulkan * vk);

	void createTextureImage(Vulkan * vk, std::string path);
	void createTextureImageView(Vulkan * vk, VkFormat format);
	void createTextureSampler(Vulkan * vk);

public:
	std::vector<VkImageView> getImageView() { return m_textureImageView; }
	VkSampler getSampler() { return m_textureSampler; }
	VkBuffer getVertexBuffer() { return m_vertexBuffer; }
	VkBuffer getIndexBuffer() { return m_indexBuffer; }
	uint32_t getNumIndices() { return static_cast<uint32_t>(m_indices.size()); }
	glm::mat4x4 getModelMatrix() { return m_modelMatrix; }

	void setImageView(int index, VkImageView imageView) { m_textureImageView[index] = imageView; }

private:
	std::vector<Vertex> m_vertices;
	std::vector<uint32_t> m_indices;
	VkBuffer m_vertexBuffer;
	VkDeviceMemory m_vertexBufferMemory;
	VkBuffer m_indexBuffer;
	VkDeviceMemory m_indexBufferMemory;

	uint32_t m_mipLevels;
	std::vector<VkImage> m_textureImage;
	std::vector<VkDeviceMemory> m_textureImageMemory;
	std::vector<VkImageView> m_textureImageView;
	VkSampler m_textureSampler = NULL;

	glm::mat4x4 m_modelMatrix;
};