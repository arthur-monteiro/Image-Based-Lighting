#include "Mesh.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

void Mesh::loadObj(Vulkan * vk, std::string path)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string err, warn;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str()))
		throw std::runtime_error(err);

#ifndef NDEBUG
	if (err != "")
		std::cout << "[Chargement objet] Erreur : " << err << " pour " << path << " !" << std::endl;
	if (warn != "")
		std::cout << "[Chargement objet]  Attention : " << warn << " pour " << path << " !" << std::endl;
#endif // !NDEBUG


	std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

	for (const auto& shape : shapes)
	{
		int numVertex = 0;
		for (const auto& index : shape.mesh.indices)
		{
			Vertex vertex = {};

			vertex.pos =
			{
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.texCoord =
			{
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			vertex.normal =
			{
				attrib.normals[3 * index.normal_index + 0],
				attrib.normals[3 * index.normal_index + 1],
				attrib.normals[3 * index.normal_index + 2]
			};

			if (uniqueVertices.count(vertex) == 0)
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(m_vertices.size());
				m_vertices.push_back(vertex);
			}

			m_indices.push_back(uniqueVertices[vertex]);

			numVertex++;
		}
	}
	
	std::array<Vertex, 3> tempTriangle;
	for (int i(0); i <= m_indices.size(); ++i)
	{
		if (i != 0 && i % 3 == 0)
		{
			glm::vec3 edge1 = tempTriangle[1].pos - tempTriangle[0].pos;
			glm::vec3 edge2 = tempTriangle[2].pos - tempTriangle[0].pos;
			glm::vec2 deltaUV1 = tempTriangle[1].texCoord - tempTriangle[0].texCoord;
			glm::vec2 deltaUV2 = tempTriangle[2].texCoord - tempTriangle[0].texCoord;

			float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

			glm::vec3 tangent;
			tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
			tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
			tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
			tangent = glm::normalize(tangent);

			for (int j(i - 1); j > i - 4; --j)
				m_vertices[m_indices[j]].tangent = tangent;
		}

		if (i == m_indices.size())
			break;
		
		tempTriangle[i % 3] = m_vertices[m_indices[i]];
	}

	createVertexBuffer(vk);
	createIndexBuffer(vk);
}

void Mesh::loadTexture(Vulkan * vk, std::vector<std::string> path)
{
	if (m_textureSampler == NULL)
		createTextureSampler(vk);
	for (int i(0); i < path.size(); ++i)
	{
		createTextureImage(vk, path[i]);
		createTextureImageView(vk);
	}
}

void Mesh::rotate(float angle, glm::vec3 dir)
{
	m_modelMatrix = glm::rotate(m_modelMatrix, angle, dir);
}

void Mesh::scale(glm::vec3 scale)
{
	m_modelMatrix = glm::scale(m_modelMatrix, scale);
}

void Mesh::translate(glm::vec3 translation)
{
	m_modelMatrix = glm::translate(m_modelMatrix, translation);
}

void Mesh::createVertexBuffer(Vulkan * vk)
{
	VkDeviceSize bufferSize = sizeof(m_vertices[0]) * m_vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	vk->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(vk->GetDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, m_vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(vk->GetDevice(), stagingBufferMemory);

	vk->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_vertexBuffer, m_vertexBufferMemory);

	vk->copyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);

	vkDestroyBuffer(vk->GetDevice(), stagingBuffer, nullptr);
	vkFreeMemory(vk->GetDevice(), stagingBufferMemory, nullptr);
}

void Mesh::createIndexBuffer(Vulkan * vk)
{
	VkDeviceSize bufferSize = sizeof(m_indices[0]) * m_indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	vk->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(vk->GetDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, m_indices.data(), (size_t)bufferSize);
	vkUnmapMemory(vk->GetDevice(), stagingBufferMemory);

	vk->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_indexBuffer, m_indexBufferMemory);

	vk->copyBuffer(stagingBuffer, m_indexBuffer, bufferSize);

	vkDestroyBuffer(vk->GetDevice(), stagingBuffer, nullptr);
	vkFreeMemory(vk->GetDevice(), stagingBufferMemory, nullptr);
}

void Mesh::createTextureImage(Vulkan * vk, std::string path)
{
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = texWidth * texHeight * 4;

	if (!pixels)
		throw std::runtime_error("Erreur : chargement de l'image " + path + " !");

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	vk->createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(vk->GetDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(vk->GetDevice(), stagingBufferMemory);

	stbi_image_free(pixels);

	m_textureImage.push_back(VkImage());
	m_textureImageMemory.push_back(VkDeviceMemory());

	vk->createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_textureImage[m_textureImage.size() - 1], m_textureImageMemory[m_textureImage.size() - 1]);

	vk->transitionImageLayout(m_textureImage[m_textureImage.size() - 1], VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		vk->copyBufferToImage(stagingBuffer, m_textureImage[m_textureImage.size() - 1], static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
	vk->transitionImageLayout(m_textureImage[m_textureImage.size() - 1], VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vkDestroyBuffer(vk->GetDevice(), stagingBuffer, nullptr);
	vkFreeMemory(vk->GetDevice(), stagingBufferMemory, nullptr);
}

void Mesh::createTextureImageView(Vulkan * vk)
{
	m_textureImageView.push_back(vk->createImageView(m_textureImage[m_textureImage.size() - 1], VK_FORMAT_R8G8B8A8_UNORM));
}

void Mesh::createTextureSampler(Vulkan * vk)
{
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.maxAnisotropy = 16;

	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	if (vkCreateSampler(vk->GetDevice(), &samplerInfo, nullptr, &m_textureSampler) != VK_SUCCESS)
		throw std::runtime_error("Erreur : texture sampler");
}
