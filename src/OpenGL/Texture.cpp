#include <OpenGL/Texture.h>

#include <utils.h>

#include <glad/glad.h>

Texture::Texture() { glGenTextures(1, &m_id); }

Texture::~Texture() { glDeleteTextures(1, &m_id); }

void Texture::Load(const uint32_t *data, int width, int height) {
	if (m_loaded && m_width == width && m_height == height) {
		Bind();
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_BGRA, GL_UNSIGNED_BYTE, data);
		Unbind();
		return;
	}

	PROFILE_SCOPE(LoadNonLoaded);

	m_width = width;
	m_height = height;

	Bind();

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA, m_width, m_height, 0, GL_BGRA, GL_UNSIGNED_BYTE, data);

	Unbind();

	m_loaded = true;
}

// assumes we want to use bytes and not floats
void Texture::Load(const char *filename) {
	PROFILE_FUNCTION();

	int width, height;
	unsigned int *temp = io::LoadTiff(filename, width, height);
	Bind();

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA, m_width, m_height, 0, GL_BGRA, GL_UNSIGNED_BYTE, temp);

	Unbind();

	free(temp);
	m_loaded = true;
}

void Texture::GetData(uint32_t *data) {
	PROFILE_FUNCTION();

	Bind();
	{
		PROFILE_SCOPE(OpenGLGetData);
		glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, data);
	}
	Unbind();
}

void Texture::Bind() { glBindTexture(GL_TEXTURE_2D, m_id); }

void Texture::Unbind() { glBindTexture(GL_TEXTURE_2D, 0); }
