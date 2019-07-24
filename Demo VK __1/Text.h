#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H  

#include <map>

#include "Vulkan.h"
#include "Pipeline.h"

struct Character
{
	VkImage image;
	VkDeviceMemory imageMemory;
	VkImageView imageView;
	int xSize; // en pixel
	int ySize;
};

class Text
{
public:
	void initialize(Vulkan * vk, int ySize, std::string path);
	int addText(Vulkan * vk, std::wstring text, glm::vec2 pos, float maxSize);

	void changeText(Vulkan * vk, std::wstring text, int textID);

public:
	int GetNbTexts() { return (int)m_texts.size(); }
	int GetNbCharacters(int index) { return (int)m_texts[index].character.size(); }
	VkBuffer GetVertexBuffer(int indexI, int indexJ) { return m_texts[indexI].vertexBuffers[indexJ]; }
	VkBuffer GetIndexBuffer() { return m_indexBuffer; }
	VkImageView GetImageView(int indexI, int indexJ) { return m_characters[m_texts[indexI].character[indexJ]].imageView; }
	VkSampler GetSampler() { return m_sampler; }

	int NeedUpdate() { return m_needUpdate; }
	void UpdateDone() { m_needUpdate = -1; }

private:
	struct TextStruct
	{
		glm::vec2 pos;
		float maxSize;
		std::vector<VkBuffer> vertexBuffers;
		std::vector<VkDeviceMemory> vertexBufferMemories;
		std::vector<wchar_t> character;
	};

	TextStruct createTextStruct(Vulkan * vk, std::wstring text, glm::vec2 pos, float maxSize);

private:
	std::map<wchar_t, Character> m_characters;
	VkSampler m_sampler;
	int m_maxYSize;

	std::vector<TextStruct> m_texts;
	int m_needUpdate = -1;

	VkBuffer m_indexBuffer;
	VkDeviceMemory m_indexBufferMemory;
};

