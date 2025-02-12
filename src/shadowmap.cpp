#include <glm/gtc/type_ptr.hpp>
#include "shadowmap.h"

ShadowMap::ShadowMap() {
	glGenTextures(1, &shadowMap);
	glBindTexture(GL_TEXTURE_2D, shadowMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowMapWidth, shadowMapHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float clampColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, clampColor);

	glGenFramebuffers(1, &shadowMapFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	GLenum fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (fboStatus != GL_FRAMEBUFFER_COMPLETE)
		printf("[ERROR] Shadow Map framebuffer failed with status %d\n", fboStatus);
}

void ShadowMap::BindShadowMap() {
	glViewport(0, 0, shadowMapWidth, shadowMapHeight);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
}

void ShadowMap::UnbindShadowMap(Camera& camera) {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, camera.screen_width, camera.screen_height);
}

void ShadowMap::PushShadows(Shader& shader, mat4 lightProjection) {
	shader.Use();
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, shadowMap);
	glUniform1i(glGetUniformLocation(shader.id, "shadowMap"), 1);
	glUniformMatrix4fv(glGetUniformLocation(shader.id, "lightProjection"), 1, GL_FALSE, value_ptr(lightProjection));
}

ShadowMap::~ShadowMap() {
	glDeleteTextures(1, &shadowMap);
	glDeleteFramebuffers(1, &shadowMapFBO);
}
