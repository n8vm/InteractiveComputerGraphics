#pragma once
#include <glm/glm.hpp>

/* A light block is a struct which contains information about a particular light to use. */
struct LightObject { /* TODO: maybe change this name */
	glm::vec4 position;
	glm::vec4 ambient;
	glm::vec4 diffuse;
	glm::vec4 specular;

	/* For shadow mapping */
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 fproj;
	glm::mat4 bproj;
	glm::mat4 lproj;
	glm::mat4 rproj;
	glm::mat4 uproj;
	glm::mat4 dproj;

	//float ambientContribution, constantAttenuation, linearAttenuation, quadraticAttenuation;
	//float spotCutoff, spotExponent;
	//glm::vec3 spotDirection;
};

#define MAXLIGHTS 3
struct LightBufferObject {
	LightObject lights[MAXLIGHTS] = {};
	int numLights = 0;
};