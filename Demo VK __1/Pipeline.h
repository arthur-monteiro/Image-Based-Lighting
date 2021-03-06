#pragma once

#include <iostream>
#include <fstream> 
#include <array>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include "Vulkan.h"

struct ModelInstance
{
	glm::mat4 model;
	glm::vec3 albedo;
	glm::float32 roughness; 
	glm::float32 metallic;

	static VkVertexInputBindingDescription getBindingDescription(uint32_t binding)
	{
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = binding;
		bindingDescription.stride = sizeof(ModelInstance);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

		return bindingDescription;
	}

	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions(uint32_t binding, uint32_t startLocation)
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions(7);

		attributeDescriptions[0].binding = binding;
		attributeDescriptions[0].location = startLocation;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[0].offset = 0;

		attributeDescriptions[1].binding = binding;
		attributeDescriptions[1].location = startLocation + 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[1].offset = 4 * sizeof(float);

		attributeDescriptions[2].binding = binding;
		attributeDescriptions[2].location = startLocation + 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[2].offset = 8 * sizeof(float);

		attributeDescriptions[3].binding = binding;
		attributeDescriptions[3].location = startLocation + 3;
		attributeDescriptions[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[3].offset = 12 * sizeof(float);

		attributeDescriptions[4].binding = binding;
		attributeDescriptions[4].location = startLocation + 4;
		attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[4].offset = offsetof(ModelInstance, albedo);

		attributeDescriptions[5].binding = binding;
		attributeDescriptions[5].location = startLocation + 5;
		attributeDescriptions[5].format = VK_FORMAT_R32_SFLOAT;
		attributeDescriptions[5].offset = offsetof(ModelInstance, roughness);

		attributeDescriptions[6].binding = binding;
		attributeDescriptions[6].location = startLocation + 6;
		attributeDescriptions[6].format = VK_FORMAT_R32_SFLOAT;
		attributeDescriptions[6].offset = offsetof(ModelInstance, metallic);

		return attributeDescriptions;
	}
};

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec3 tangent;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription getBindingDescription(uint32_t binding)
	{
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = binding;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions(uint32_t binding)
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions(4);

		attributeDescriptions[0].binding = binding;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = binding;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, normal);

		attributeDescriptions[2].binding = binding;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, tangent);

		attributeDescriptions[3].binding = binding;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[3].offset = offsetof(Vertex, texCoord);

		return attributeDescriptions;
	}

	bool operator==(const Vertex& other) const
	{
		return pos == other.pos && normal == other.normal && texCoord == other.texCoord && tangent == other.tangent;
	}
};

struct TextVertex
{
	glm::vec2 pos;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(TextVertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions()
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(TextVertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(TextVertex, texCoord);

		return attributeDescriptions;
	}

	bool operator==(const TextVertex& other) const
	{
		return pos == other.pos && texCoord == other.texCoord;
	}
};

namespace std
{
	template<> struct hash<Vertex>
	{
		size_t operator()(Vertex const& vertex) const
		{
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};

	template<> struct hash<TextVertex>
	{
		size_t operator()(TextVertex const& vertex) const
		{
			return ((hash<glm::vec2>()(vertex.pos) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1)) >> 1);
		}
	};
}

class Pipeline
{
public:
	void initialize(Vulkan* vk, VkDescriptorSetLayout* descriptorSetLayout, VkRenderPass renderPass, std::string vertPath, 
		std::string fragPath, bool alphaBlending, VkSampleCountFlagBits msaaSamples, std::vector<VkVertexInputBindingDescription> vertexInputDescription,
		std::vector<VkVertexInputAttributeDescription> attributeInputDescription, VkExtent2D extent);

private:
	static std::vector<char> readFile(const std::string& filename);
	VkShaderModule createShaderModule(const std::vector<char>& code, VkDevice device);

public:
	VkPipeline GetGraphicsPipeline() { return m_graphicsPipeline; }
	VkPipelineLayout GetPipelineLayout() { return m_pipelineLayout; }

private:
	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_graphicsPipeline;
};