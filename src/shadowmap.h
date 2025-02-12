#ifndef SHADOWMAP_H
#define SHADOWMAP_H

#include <stdio.h>
#include "camera.h"
#include "shader.h"

#include <glm/glm.hpp>
#include "../glad/glad.h"

class ShadowMap {
private:
	GLuint shadowMap;
	GLuint shadowMapFBO;
	const unsigned int shadowMapWidth = 2048;
	const unsigned int shadowMapHeight = 2048;
public:
	ShadowMap();
	void BindShadowMap();
	void UnbindShadowMap(Camera& camera);
	void PushShadows(Shader& shader, mat4 lightProjection);
	~ShadowMap();
};

#endif
