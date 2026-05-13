#pragma once 
#include <glm/glm.hpp>

struct Camera {
	float fov;
    float aspect;

    float nearPlane;
    float farPlane;

	glm::mat4 view;
	glm::mat4 projection;

	void setPosition(glm::vec3 position_); 
	void moveBy();
};
