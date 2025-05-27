#pragma once

class Texture {
      public:
	Texture();
	~Texture();
	void Load(const unsigned int *data, int width, int height);
	void Load(const char *filename);
	void GetData(unsigned int *data);
	void Bind();
	void Unbind();
	unsigned int GetID() const { return m_id; }
	int GetWidth() const { return m_width; }
	int GetHeight() const { return m_height; }

      private:
	unsigned int m_id;
	bool m_loaded = false;
	int m_width = 0, m_height = 0, m_channels = 0;
	unsigned char *m_data;
};
