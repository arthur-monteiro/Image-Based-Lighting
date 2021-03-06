#include "Camera.h"

void Camera::initialize(glm::vec3 position, glm::vec3 target, glm::vec3 verticalAxis, float sensibility, float speed)
{
	m_position = position;
	m_target = target;
	m_verticalAxis = verticalAxis;
	m_sensibility = sensibility;
	m_speed = speed;

	setTarget(m_target);
}

void Camera::update(GLFWwindow* window)
{
	if (m_oldMousePosX < 0)
	{
		glfwGetCursorPos(window, &m_oldMousePosX, &m_oldMousePosY);
		return;
	}

	auto currentTime = std::chrono::steady_clock::now();
	long long millisecondOffset = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - m_startTime).count();
	float secondOffset = millisecondOffset / 1000.0;
	m_startTime = currentTime;

	double currentMousePosX, currentMousePosY;
	glfwGetCursorPos(window, &currentMousePosX, &currentMousePosY);

	updateOrientation(currentMousePosX - m_oldMousePosX, currentMousePosY - m_oldMousePosY);
	m_oldMousePosX = currentMousePosX;
	m_oldMousePosY = currentMousePosY;

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
	{
		m_position = m_position + m_orientation * (secondOffset * m_speed);
		m_target = m_position + m_orientation;
	}
	else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
	{
		m_position = m_position - m_orientation * (secondOffset * m_speed);
		m_target = m_position + m_orientation;
	}

	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
	{
		m_position = m_position + m_lateralDirection * (secondOffset * m_speed);
		m_target = m_position + m_orientation;
	}

	else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
	{
		m_position = m_position - m_lateralDirection * (secondOffset * m_speed);
		m_target = m_position + m_orientation;
	}
}

glm::mat4 Camera::getViewMatrix()
{
	return glm::lookAt(m_position, m_target, m_verticalAxis);
}

glm::vec3 Camera::getPosition()
{
	return m_position;
}

void Camera::setPosition(glm::vec3 position)
{
}

void Camera::setTarget(glm::vec3 target)
{
	m_orientation = m_target - m_position;
	m_orientation = glm::normalize(m_orientation);

	if (m_verticalAxis.x == 1.0)
	{
		m_phi = glm::asin(m_orientation.x);
		m_theta = glm::acos(m_orientation.y / glm::cos(m_phi));

		if (m_orientation.y < 0)
			m_theta *= -1;
	}

	else if (m_verticalAxis.y == 1.0)
	{
		m_phi = glm::asin(m_orientation.y);
		m_theta = glm::acos(m_orientation.z / glm::cos(m_phi));

		if (m_orientation.z < 0)
			m_theta *= -1;
	}

	else
	{
		m_phi = glm::asin(m_orientation.x);
		m_theta = glm::acos(m_orientation.z / glm::cos(m_phi));

		if (m_orientation.z < 0)
			m_theta *= -1;
	}
}

void Camera::updateOrientation(int xOffset, int yOffset)
{
	m_phi -= yOffset * m_sensibility;
	m_theta -= xOffset * m_sensibility;

	if (m_phi > glm::half_pi<float>() - OFFSET_ANGLES)
		m_phi = glm::half_pi<float>() - OFFSET_ANGLES;
	else if (m_phi < - glm::half_pi<float>() + OFFSET_ANGLES)
		m_phi = - glm::half_pi<float>() + OFFSET_ANGLES;

	if (m_verticalAxis.x == 1.0)
	{
		m_orientation.x = glm::sin(m_phi);
		m_orientation.y = glm::cos(m_phi) * glm::cos(m_theta);
		m_orientation.z = glm::cos(m_phi) * glm::sin(m_theta);
	}

	else if (m_verticalAxis.y == 1.0)
	{
		m_orientation.x = glm::cos(m_phi) * glm::sin(m_theta);
		m_orientation.y = glm::sin(m_phi);
		m_orientation.z = glm::cos(m_phi) * glm::cos(m_theta);
	}

	else
	{
		m_orientation.x = glm::cos(m_phi) * glm::cos(m_theta);
		m_orientation.y = glm::cos(m_phi) * glm::sin(m_theta);
		m_orientation.z = glm::sin(m_phi);
	}

	m_lateralDirection = glm::normalize(glm::cross(m_verticalAxis, m_orientation));
	m_target = m_position + m_orientation;
}
