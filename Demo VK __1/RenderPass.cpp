#include "RenderPass.h"

void RenderPass::initialize(Vulkan* vk, bool createFrameBuffer, VkExtent2D extent, bool present, VkSampleCountFlagBits msaaSamples)
{
	m_text = nullptr;
	m_msaaSamples = msaaSamples;

	m_format = vk->GetSwapChainImageFormat();
	m_depthFormat = vk->findDepthFormat();

	if(present)
		createRenderPass(vk->GetDevice(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	else
		createRenderPass(vk->GetDevice(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	createDescriptorPool(vk->GetDevice());
	
	createColorResources(vk);
	if (createFrameBuffer)
		m_frameBuffer = vk->createFrameBuffer(extent, m_renderPass);
	else
		vk->createSwapchainFramebuffers(m_renderPass, m_msaaSamples, m_colorImageView);

	m_textDescriptorSetLayout = createDescriptorSetLayout(vk->GetDevice(), 0, 1);
	m_textPipeline.initialize(vk, &m_textDescriptorSetLayout, m_renderPass, "Shaders/TextVert.spv", 
		"Shaders/TextFrag.spv", true, true, m_msaaSamples);

	m_uboLights = createUboLights(vk);

	m_useSwapChain = !createFrameBuffer;
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	if (vkCreateSemaphore(vk->GetDevice(), &semaphoreInfo, nullptr, &m_renderCompleteSemaphore) != VK_SUCCESS)
		throw std::runtime_error("Erreur : création de la sémaphores");
	m_extent = extent;

	vk->SetRenderFinishedLastRenderPassSemaphore(m_renderCompleteSemaphore);

	m_commandPool = vk->createCommandPool();
}

UniformBufferObjectMatrices RenderPass::createUboMatrices(Vulkan * vk)
{
	VkDeviceSize bufferSize = sizeof(UniformBufferObjectMatrices);
	VkBuffer uniformBuffer;
	VkDeviceMemory uniformBufferMemory;
	vk->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffer, uniformBufferMemory);

	UniformBufferObjectMatrices ubo;
	ubo.uniformBuffer = uniformBuffer;
	ubo.uniformBufferMemory = uniformBufferMemory;
	return ubo;
}

UniformBufferObjectLights RenderPass::createUboLights(Vulkan * vk)
{
	VkDeviceSize bufferSize = sizeof(UniformBufferObjectLights);
	VkBuffer uniformBuffer;
	VkDeviceMemory uniformBufferMemory;
	vk->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffer, 
		uniformBufferMemory);

	UniformBufferObjectLights ubo;
	ubo.uniformBuffer = uniformBuffer;
	ubo.uniformBufferMemory = uniformBufferMemory;
	return ubo;
}

int RenderPass::addMesh(Vulkan * vk, std::vector<Mesh *> meshes, std::string vertPath, std::string fragPath, 
	int nbUbo, int nbTexture)
{
	/* Ici tous les meshes sont rendus avec les mêmes shaders */
	for (int i(0); i < meshes.size(); ++i)
	{
		m_meshes.push_back(meshes[i]);
	}
	MeshPipeline meshesPipeline;

	VkDescriptorSetLayout descriptorSetLayout = createDescriptorSetLayout(vk->GetDevice(), nbUbo, nbTexture);

	Pipeline pipeline;
	pipeline.initialize(vk, &descriptorSetLayout, m_renderPass, vertPath, fragPath, false, false, m_msaaSamples);
	meshesPipeline.pipeline = pipeline.GetGraphicsPipeline();
	meshesPipeline.pipelineLayout = pipeline.GetPipelineLayout();
	
	for (int i(0); i < meshes.size(); ++i)
	{
		m_ubosMatrices.push_back(createUboMatrices(vk));

		meshesPipeline.vertexBuffer.push_back(meshes[i]->GetVertexBuffer());
		meshesPipeline.indexBuffer.push_back(meshes[i]->GetIndexBuffer());
		meshesPipeline.nbIndices.push_back(meshes[i]->GetNumIndices());

#ifndef NDEBUG
		if (meshes[i]->GetImageView().size() != nbTexture)
			std::cout << "Attention : le nombre de texture utilisés n'est pas égale au nombre de textures du mesh" << std::endl;
#endif // DEBUG

		VkDescriptorSet descriptorSet = createDescriptorSet(vk->GetDevice(), descriptorSetLayout,
			meshes[i]->GetImageView(), meshes[i]->GetSampler(), std::vector<VkBuffer>(1, m_ubosMatrices[m_ubosMatrices.size() -1].uniformBuffer), nbUbo, nbTexture);
		meshesPipeline.descriptorSet.push_back(descriptorSet);
	}
	
	m_meshesPipeline.push_back(meshesPipeline);

	return (int)m_meshesPipeline.size() - 1;
}

int RenderPass::addText(Vulkan * vk, Text * text)
{
	m_text = text;

	int nbText = text->GetNbTexts();
	for (int i = 0; i < nbText; ++i)
	{
		m_textID.push_back(m_meshesPipeline.size());
		MeshPipeline meshPipeline;
		meshPipeline.pipeline = m_textPipeline.GetGraphicsPipeline();
		meshPipeline.pipelineLayout = m_textPipeline.GetPipelineLayout();

		for(int j = 0; j < text->GetNbCharacters(i); ++j)
		{
			meshPipeline.vertexBuffer.push_back(text->GetVertexBuffer(i, j));
			meshPipeline.indexBuffer.push_back(text->GetIndexBuffer());
			meshPipeline.nbIndices.push_back(6);

			VkDescriptorSet descriptorSet = createDescriptorSet(vk->GetDevice(), m_textDescriptorSetLayout,
				std::vector<VkImageView>(1, text->GetImageView(i, j)), text->GetSampler(), std::vector<VkBuffer>(), 
				0, 1, false);
			meshPipeline.descriptorSet.push_back(descriptorSet);
		}

		m_meshesPipeline.push_back(meshPipeline);
	}

	return 0;
}

int RenderPass::addPointLight(Vulkan * vk, glm::vec3 position, glm::vec3 color)
{
	m_uboLights.pointLightsPositions[m_uboLights.nbPointLights] = glm::vec4(position, 1.0f);
	m_uboLights.pointLightsColors[m_uboLights.nbPointLights] = glm::vec4(color, 1.0f);
	m_uboLights.nbPointLights++;

	m_uboLights.camPos = glm::vec4(0.0f, 2.0f, -2.0f, 1.0f);

	void* data;
	vkMapMemory(vk->GetDevice(), m_uboLights.uniformBufferMemory, 0, sizeof(m_uboLights), 0, &data);
		memcpy(data, &m_uboLights, sizeof(m_uboLights));
	vkUnmapMemory(vk->GetDevice(), m_uboLights.uniformBufferMemory);

	return m_uboLights.nbPointLights - 1;
}

int RenderPass::addDirLight(Vulkan* vk, glm::vec3 direction, glm::vec3 color)
{
	m_uboLights.dirLightsDirections[m_uboLights.nbDirLights] = glm::vec4(direction, 1.0f);
	m_uboLights.dirLightsColors[m_uboLights.nbDirLights] = glm::vec4(color, 1.0f);
	m_uboLights.nbDirLights++;

	void* data;
	vkMapMemory(vk->GetDevice(), m_uboLights.uniformBufferMemory, 0, sizeof(m_uboLights), 0, &data);
	memcpy(data, &m_uboLights, sizeof(m_uboLights));
	vkUnmapMemory(vk->GetDevice(), m_uboLights.uniformBufferMemory);

	return m_uboLights.nbDirLights - 1;
}

void RenderPass::recordDraw(Vulkan * vk)
{
	if(m_useSwapChain) vk->fillCommandBuffer(m_renderPass, m_meshesPipeline);
	else fillCommandBuffer(vk);
}

void RenderPass::updateUniformBuffer(Vulkan * vk)
{
	for (int i = 0; i < m_meshes.size(); ++i)
	{
		UniformBufferObjectMatrices ubo = {};
		ubo.model = m_meshes[i]->GetModelMatrix();
		ubo.view = glm::lookAt(glm::vec3(0.0f, 2.0f, -2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		ubo.proj = glm::perspective(glm::radians(45.0f), vk->GetSwapChainExtend().width / (float)vk->GetSwapChainExtend().height, 0.1f, 10.0f);
		ubo.proj[1][1] *= -1;

		void* data;
		vkMapMemory(vk->GetDevice(), m_ubosMatrices[i].uniformBufferMemory, 0, sizeof(ubo), 0, &data);
			memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(vk->GetDevice(), m_ubosMatrices[i].uniformBufferMemory);
	}
}

void RenderPass::drawCall(Vulkan * vk)
{
	if (m_text && m_text->NeedUpdate() != -1)
	{
		m_meshesPipeline[m_textID[m_text->NeedUpdate()]].free(vk->GetDevice(), m_descriptorPool);
		//m_meshesPipeline.erase(m_meshesPipeline.begin() + m_textID[m_text->NeedUpdate()]);

		//m_textID[m_text->NeedUpdate()] = m_meshesPipeline.size();
		MeshPipeline meshPipeline;
		meshPipeline.pipeline = m_textPipeline.GetGraphicsPipeline();
		meshPipeline.pipelineLayout = m_textPipeline.GetPipelineLayout();

		for (int j = 0; j < m_text->GetNbCharacters(m_text->NeedUpdate()); ++j)
		{
			meshPipeline.vertexBuffer.push_back(m_text->GetVertexBuffer(m_text->NeedUpdate(), j));
			meshPipeline.indexBuffer.push_back(m_text->GetIndexBuffer());
			meshPipeline.nbIndices.push_back(6);

			VkDescriptorSet descriptorSet = createDescriptorSet(vk->GetDevice(), m_textDescriptorSetLayout,
				std::vector<VkImageView>(1, m_text->GetImageView(m_text->NeedUpdate(), j)), m_text->GetSampler(),
				std::vector<VkBuffer>(), 0, 1, false);
			meshPipeline.descriptorSet.push_back(descriptorSet);
		}

		m_meshesPipeline[m_textID[m_text->NeedUpdate()]] = meshPipeline;

		recordDraw(vk);

		m_text->UpdateDone();
	}
	
	if(m_useSwapChain)
		vk->drawFrame();
	else 
		drawFrame(vk);
}

void RenderPass::cleanup(Vulkan * vk)
{
	vkDestroyImageView(vk->GetDevice(), m_colorImageView, nullptr);
	vkDestroyImage(vk->GetDevice(), m_colorImage, nullptr);
	vkFreeMemory(vk->GetDevice(), m_colorImageMemory, nullptr);

	for (int i(0); i < m_meshesPipeline.size(); ++i)
		m_meshesPipeline[i].free(vk->GetDevice(), m_descriptorPool, true); // ne détruit pas les ressources

	for (int i(0); i < m_ubosMatrices.size(); ++i)
		m_ubosMatrices[i].cleanup(vk->GetDevice());
	m_ubosMatrices.clear();
	m_meshes.clear();

	m_uboLights.cleanup(vk->GetDevice());

	m_meshesPipeline.clear();

	vkDestroyRenderPass(vk->GetDevice(), m_renderPass, nullptr);
}

void RenderPass::createRenderPass(VkDevice device, VkImageLayout finalLayout)
{
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = m_format;
	colorAttachment.samples = m_msaaSamples;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = finalLayout;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = m_depthFormat;
	depthAttachment.samples = m_msaaSamples;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription colorAttachmentResolve = {};
	colorAttachmentResolve.format = m_format;
	colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentResolveRef = {};
	colorAttachmentResolveRef.attachment = 2;
	colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;
	subpass.pResolveAttachments = &colorAttachmentResolveRef;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());;
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS)
		throw std::runtime_error("Erreur : render pass");
}

void RenderPass::createColorResources(Vulkan* vk)
{
	VkFormat colorFormat = m_format;

	vk->createImage(vk->GetSwapChainExtend().width, vk->GetSwapChainExtend().height, 1, m_msaaSamples, colorFormat, VK_IMAGE_TILING_OPTIMAL, 
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_colorImage, m_colorImageMemory);
	m_colorImageView = vk->createImageView(m_colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

	vk->transitionImageLayout(m_colorImage, colorFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
}

VkDescriptorSetLayout RenderPass::createDescriptorSetLayout(VkDevice device, int nbUbo, int nbTexture)
{
	int i = 0;
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	for (; i < nbUbo; ++i)
	{
		VkDescriptorSetLayoutBinding uboLayoutBinding = {};
		uboLayoutBinding.binding = i;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uboLayoutBinding.pImmutableSamplers = nullptr;

		bindings.push_back(uboLayoutBinding);
	}

	for (; i < nbTexture + nbUbo; ++i)
	{
		VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
		samplerLayoutBinding.binding = i;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		samplerLayoutBinding.pImmutableSamplers = nullptr;

		bindings.push_back(samplerLayoutBinding);
	}

	// UBO Lights
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = i;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr;

	bindings.push_back(uboLayoutBinding);

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	VkDescriptorSetLayout descriptorSetLayout;
	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
		throw std::runtime_error("Erreur : descriptor set layout");

	return descriptorSetLayout;
}

void RenderPass::createDescriptorPool(VkDevice device)
{
	std::array<VkDescriptorPoolSize, 2> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = 1024;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = 1024;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = 1024;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

	if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
		throw std::runtime_error("Erreur : création du descriptor pool");
}

VkDescriptorSet RenderPass::createDescriptorSet(VkDevice device, VkDescriptorSetLayout decriptorSetLayout, std::vector<VkImageView> imageView,
	VkSampler sampler, std::vector<VkBuffer> uniformBuffer, int nbUbo, int nbTexture, bool useUboLights)
{
	VkDescriptorSetLayout layouts[] = { decriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = m_descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	VkDescriptorSet descriptorSet;
	VkResult res = vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet);
	if (res != VK_SUCCESS)
		throw std::runtime_error("Erreur : allocation descriptor set");

	std::vector<VkWriteDescriptorSet> descriptorWrites;

	int i = 0;
	std::vector<VkDescriptorBufferInfo> bufferInfo(nbUbo); // ne doit pas être détruit avant l'appel de vkUpdateDescriptorSets
	for(; i < nbUbo; ++i)
	{
		bufferInfo[i].buffer = uniformBuffer[i];
		bufferInfo[i].offset = 0;
		bufferInfo[i].range = sizeof( UniformBufferObjectMatrices);

		VkWriteDescriptorSet descriptorWrite;
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSet;
		descriptorWrite.dstBinding = i;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo[i];
		descriptorWrite.pNext = NULL;

		descriptorWrites.push_back(descriptorWrite);
	}

	std::vector<VkDescriptorImageInfo> imageInfo(nbTexture);
	for(; i < nbUbo + nbTexture; ++i)
	{
		imageInfo[i - nbUbo].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo[i - nbUbo].imageView = imageView[i - nbUbo];
		imageInfo[i - nbUbo].sampler = sampler;

		VkWriteDescriptorSet descriptorWrite;
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSet;
		descriptorWrite.dstBinding = i;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pImageInfo = &imageInfo[i - nbUbo];
		descriptorWrite.pNext = NULL;

		descriptorWrites.push_back(descriptorWrite);
	}

	if (useUboLights)
	{
		VkDescriptorBufferInfo uboLightBufferInfo;

		uboLightBufferInfo.buffer = m_uboLights.uniformBuffer;
		uboLightBufferInfo.offset = 0;
		uboLightBufferInfo.range = sizeof(UniformBufferObjectLights);

		VkWriteDescriptorSet descriptorWrite;
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSet;
		descriptorWrite.dstBinding = i;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &uboLightBufferInfo;
		descriptorWrite.pNext = NULL;

#ifndef NDEBUG
		std::cout << "L'UBO Lights se situe au binding " << i << std::endl;
#endif // !NDEBUG

		descriptorWrites.push_back(descriptorWrite);
	}
	

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

	return descriptorSet;
}

void RenderPass::fillCommandBuffer(Vulkan * vk)
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	if (vkAllocateCommandBuffers(vk->GetDevice(), &allocInfo, &m_commandBuffer) != VK_SUCCESS)
		throw std::runtime_error("Erreur : allocation des command buffers");


	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	vkBeginCommandBuffer(m_commandBuffer, &beginInfo);

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = m_renderPass;
	renderPassInfo.framebuffer = m_frameBuffer.framebuffer;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = m_extent;

	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = { 0.0f, 0.0f, 1.0f, 1.0f };
	clearValues[1].depthStencil = { 1.0f };

	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(m_commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	for (int j = 0; j < m_meshesPipeline.size(); ++j)
	{
		for (int k(0); k < m_meshesPipeline[j].vertexBuffer.size(); ++k)
		{
			vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_meshesPipeline[j].pipeline);

			VkBuffer vertexBuffers[] = { m_meshesPipeline[j].vertexBuffer[k] };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(m_commandBuffer, 0, 1, vertexBuffers, offsets);

			vkCmdBindIndexBuffer(m_commandBuffer, m_meshesPipeline[j].indexBuffer[k], 0, VK_INDEX_TYPE_UINT32);

			vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				m_meshesPipeline[j].pipelineLayout, 0, 1, &m_meshesPipeline[j].descriptorSet[k], 0, nullptr);

			vkCmdDrawIndexed(m_commandBuffer, m_meshesPipeline[j].nbIndices[k], 1, 0, 0, 0);
		}
	}

	vkCmdEndRenderPass(m_commandBuffer);

	if (vkEndCommandBuffer(m_commandBuffer) != VK_SUCCESS)
		throw std::runtime_error("Erreur : record command buffer");
}

void RenderPass::drawFrame(Vulkan * vk)
{
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	VkSemaphore renderFinishedSemaphore[] = { vk->GetRenderFinishedSemaphore() };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &m_renderCompleteSemaphore; // renderPass
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_commandBuffer;

	if (m_firstDraw)
	{
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.pWaitSemaphores = NULL;

		m_firstDraw = false;
	}

	if (vkQueueSubmit(vk->GetGraphicalQueue(), 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS)
		throw std::runtime_error("Erreur : draw command");
}
