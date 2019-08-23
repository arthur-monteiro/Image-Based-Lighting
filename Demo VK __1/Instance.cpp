#include "Instance.h"

void Instance::load(Vulkan* vk, VkDeviceSize bufferSize, void* data)
{
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	vk->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* pData;
	vkMapMemory(vk->GetDevice(), stagingBufferMemory, 0, bufferSize, 0, &pData);
	memcpy(pData, data, (size_t)bufferSize);
	vkUnmapMemory(vk->GetDevice(), stagingBufferMemory);

	vk->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_instanceBuffer, m_instanceBufferMemory);

	vk->copyBuffer(stagingBuffer, m_instanceBuffer, bufferSize);

	vkDestroyBuffer(vk->GetDevice(), stagingBuffer, nullptr);
	vkFreeMemory(vk->GetDevice(), stagingBufferMemory, nullptr);
}