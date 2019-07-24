#include "Text.h"

void Text::initialize(Vulkan * vk, int ySize, std::string path)
{
	m_maxYSize = ySize;

	int texWidth, texHeight, texChannels;
	FT_Library ft;
	if (FT_Init_FreeType(&ft))
		throw std::runtime_error("Erreur : initialisation de freeType");

	FT_Face face;
	if (FT_New_Face(ft, path.c_str(), 0, &face))
		throw std::runtime_error("Erreur : chargement de la police de charact�res");

	FT_Set_Pixel_Sizes(face, 0, ySize);

	for (wchar_t c = 0; c < 500; ++c)
	{
		if (c == 160 || c == 32)
			c++;
		Character character;

		if (FT_Load_Char(face, c, FT_LOAD_RENDER))
			throw std::runtime_error("Erreur : chargement du charact�re " + c);

		if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL)) /* convert to an anti-aliased bitmap */
			throw std::runtime_error("Erreur : render glyph " + c);

		texWidth = face->glyph->bitmap.width;
		texHeight = face->glyph->bitmap.rows;
		unsigned char* pixels = new unsigned char[texWidth * texHeight];
			memcpy(pixels, face->glyph->bitmap.buffer, texWidth * texHeight);
		VkDeviceSize imageSize = texWidth * texHeight;

		character.xSize = texWidth;
		character.ySize = texHeight;

		if (!pixels)
			throw std::runtime_error("Erreur : chargement de la texture du charact�re " + c);

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		vk->createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(vk->GetDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
			memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(vk->GetDevice(), stagingBufferMemory);

		delete(pixels);

		vk->createImage(texWidth, texHeight, VK_FORMAT_R8_UNORM, VK_IMAGE_TILING_OPTIMAL, 
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
			character.image, character.imageMemory);

		vk->transitionImageLayout(character.image, VK_FORMAT_R8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			vk->copyBufferToImage(stagingBuffer, character.image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
		vk->transitionImageLayout(character.image, VK_FORMAT_R8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		vkDestroyBuffer(vk->GetDevice(), stagingBuffer, nullptr);
		vkFreeMemory(vk->GetDevice(), stagingBufferMemory, nullptr);

		character.imageView = vk->createImageView(character.image, VK_FORMAT_R8_UNORM);

		m_characters[c] = character;
	}

	FT_Done_Face(face);
	FT_Done_FreeType(ft);

	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

	if (vkCreateSampler(vk->GetDevice(), &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS)
		throw std::runtime_error("Erreur : cr�ation d'un sampler");

	std::vector<uint32_t> indices = { 0, 2, 1, 1, 2, 3};
	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	vk->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(vk->GetDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices.data(), (size_t)bufferSize);
	vkUnmapMemory(vk->GetDevice(), stagingBufferMemory);

	vk->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_indexBuffer, m_indexBufferMemory);

	vk->copyBuffer(stagingBuffer, m_indexBuffer, bufferSize);

	vkDestroyBuffer(vk->GetDevice(), stagingBuffer, nullptr);
	vkFreeMemory(vk->GetDevice(), stagingBufferMemory, nullptr);
}

int Text::addText(Vulkan * vk, std::wstring text, glm::vec2 pos, float maxSize)
{
	m_texts.push_back(createTextStruct(vk, text, pos, maxSize));
	return (int)m_texts.size() - 1;
}

void Text::changeText(Vulkan * vk, std::wstring text, int textID)
{
	if (textID >= m_texts.size())
		throw std::runtime_error("Erreur : Changement d'un texte non allou�");

	glm::vec2 pos = m_texts[textID].pos;
	float maxSize = m_texts[textID].maxSize;

	for (int i = 0; i < m_texts[textID].vertexBufferMemories.size(); ++i)
	{
		vkDestroyBuffer(vk->GetDevice(), m_texts[textID].vertexBuffers[i], nullptr);
		vkFreeMemory(vk->GetDevice(), m_texts[textID].vertexBufferMemories[i], nullptr);
	}
	m_texts[textID].character.clear();
	m_texts[textID].vertexBufferMemories.clear();
	m_texts[textID].vertexBuffers.clear();

	m_texts[textID] = createTextStruct(vk, text, pos, maxSize);

	m_needUpdate = textID;
}

Text::TextStruct Text::createTextStruct(Vulkan * vk, std::wstring text, glm::vec2 pos, float maxSize)
{
	TextStruct textStruct;
	textStruct.pos = pos;
	textStruct.maxSize = maxSize;

	std::vector<std::array<TextVertex, 4>> vertices;
	float offsetX = 0.0f;
	glm::vec2 offset = /*glm::vec3(-1.0f, -1.0f, 0.0f) +*/ pos;
	float scale = (vk->GetSwapChainExtend().height / (float)m_maxYSize) * maxSize * 2.0f;

	for (int i = 0; i < text.size(); ++i)
	{
		if (text[i] == ' ')
			offsetX += m_characters['a'].xSize * 0.5f;
		else
		{
			glm::vec2 topLeft = { offsetX, m_maxYSize - m_characters[text[i]].ySize };
			glm::vec2 topRight = { offsetX + m_characters[text[i]].xSize, m_maxYSize - m_characters[text[i]].ySize };
			glm::vec2 botLeft = { offsetX, m_maxYSize };
			glm::vec2 botRight = { offsetX + m_characters[text[i]].xSize, m_maxYSize };

			std::array<TextVertex, 4> vert;
			vert[0].pos = scale * topLeft / glm::vec2(vk->GetSwapChainExtend().width, vk->GetSwapChainExtend().height) + offset;
			vert[0].texCoord = glm::vec2(0.0f, 0.0f);
			vert[1].pos = scale * topRight / glm::vec2(vk->GetSwapChainExtend().width, vk->GetSwapChainExtend().height) + offset;
			vert[1].texCoord = glm::vec2(1.0f, 0.0f);
			vert[2].pos = scale * botLeft / glm::vec2(vk->GetSwapChainExtend().width, vk->GetSwapChainExtend().height) + offset;
			vert[2].texCoord = glm::vec2(0.0f, 1.0f);
			vert[3].pos = scale * botRight / glm::vec2(vk->GetSwapChainExtend().width, vk->GetSwapChainExtend().height) + offset;
			vert[3].texCoord = glm::vec2(1.0f, 1.0f);

			vertices.push_back(vert);

			offsetX += m_characters[text[i]].xSize;
			offsetX += m_characters['a'].xSize * 0.1f;

			textStruct.character.push_back(text[i]);
		}
	}

	VkDeviceSize bufferSize = 4 * sizeof(TextVertex);

	for (int i = 0; i < vertices.size(); ++i)
	{
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		vk->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(vk->GetDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices[i].data(), (size_t)bufferSize);
		vkUnmapMemory(vk->GetDevice(), stagingBufferMemory);

		VkBuffer vertexBuffer;
		VkDeviceMemory vertexBufferMemory;
		vk->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

		vk->copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

		vkDestroyBuffer(vk->GetDevice(), stagingBuffer, nullptr);
		vkFreeMemory(vk->GetDevice(), stagingBufferMemory, nullptr);

		textStruct.vertexBuffers.push_back(vertexBuffer);
		textStruct.vertexBufferMemories.push_back(vertexBufferMemory);
	}

	vertices.clear();

	return textStruct;
}
