#include "UniformBufferObject.h"

template<typename T>
void UniformBufferObject<T>::cleanup(VkDevice device)
{
	vkDestroyBuffer(device, m_uniformBuffer, nullptr);
	vkFreeMemory(device, m_uniformBufferMemory, nullptr);
}
