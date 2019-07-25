#pragma once

#include <unordered_map>

#include "Vulkan.h"
#include "Pipeline.h"

class Mesh
{
public:
	void loadObj(Vulkan * vk, std::string path, glm::vec3 forceNormal = glm::vec3(-1.0f));
	void loadTexture(Vulkan * vk, std::vector<std::string> path);

	void restoreTransformations() { m_modelMatrix = glm::mat4(1.0); }
	void rotate(float angle, glm::vec3 dir);
	void scale(glm::vec3 scale);
	void translate(glm::vec3 translation);

private:
	void createVertexBuffer(Vulkan * vk);
	void createIndexBuffer(Vulkan * vk);

	void createTextureImage(Vulkan * vk, std::string path);
	void createTextureImageView(Vulkan * vk);
	void createTextureSampler(Vulkan * vk);

public:
	std::vector<VkImageView> GetImageView() { return m_textureImageView; }
	VkSampler GetSampler() { return m_textureSampler; }
	VkBuffer GetVertexBuffer() { return m_vertexBuffer; }
	VkBuffer GetIndexBuffer() { return m_indexBuffer; }
	uint32_t GetNumIndices() { return static_cast<uint32_t>(m_indices.size()); }
	glm::mat4x4 GetModelMatrix() { return m_modelMatrix; }

	void SetImageView(int index, VkImageView imageView) { m_textureImageView[index] = imageView; }

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

