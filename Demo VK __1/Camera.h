#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <chrono>
#include <iostream>

class Camera
{
public:
	void initialize(glm::vec3 position, glm::vec3 target, glm::vec3 verticalAxis, float sensibility, float speed);

	void update(GLFWwindow* window);

	glm::mat4 getViewMatrix();
	glm::vec3 getPosition();
	void setPosition(glm::vec3 position);
	void setTarget(glm::vec3 target);

private:
	void updateOrientation(int xOffset, int yOffset);

private:
	float m_phi = 0.0f;
	float m_theta = 0.0f;
	glm::vec3 m_orientation = glm::vec3(1.0f, 0.0f, 0.0f);
	glm::vec3 m_lateralDirection = glm::vec3(-1.0f, 0.0f, 0.0f);

	glm::vec3 m_position = glm::vec3(0.0f);
	glm::vec3 m_target = glm::vec3(1.0f, 0.0f, 0.0f);

	glm::vec3 m_verticalAxis = glm::vec3(0.0f, 1.0f, 0.0f);
	float m_sensibility = 0.5f;
	float m_speed = 0.5f;

	std::chrono::steady_clock::time_point m_startTime = std::chrono::steady_clock::now();
	double m_oldMousePosX = -1;
	double m_oldMousePosY = -1;

	const float OFFSET_ANGLES = 0.01f;
};

