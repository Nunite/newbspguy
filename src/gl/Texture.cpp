#include "lang.h"
#include <GL/glew.h>
#include "Wad.h"
#include "Texture.h"
#include "lodepng.h"
#include "log.h"
#include "Settings.h"
#include "Renderer.h"

std::vector<Texture*> g_all_Textures;

Texture::Texture(GLsizei _width, GLsizei _height, unsigned char* data, const std::string& name, bool rgba, bool _owndata)
{
	tex_owndata = _owndata;
	wad_name = "";
	this->width = _width;
	this->height = _height;

	if (!(g_render_flags & RENDER_TEXTURES_NOFILTER))
	{
		nearFilter = GL_LINEAR;
		farFilter = GL_LINEAR;
	}
	else
	{
		nearFilter = GL_NEAREST;
		farFilter = GL_NEAREST;
	}
	this->data = data;
	dataLen = (unsigned int)(width * height) * (rgba ? sizeof(COLOR4) : sizeof(COLOR3));
	id = 0xFFFFFFFF;
	format = rgba ? GL_RGBA : GL_RGB;
	this->texName = name;
	uploaded = false;

	type = -1;

	if (g_settings.verboseLogs)
		print_log(get_localized_string(LANG_0970), name, width, height);

	this->transparentMode = IsTextureTransparent(name) ? 1 : 0;

	if (name.size() && name[0] == '{')
	{
		this->transparentMode = 2;
	}
	g_all_Textures.push_back(this);
}

Texture::~Texture()
{
	this->wad_name = "";

	if (id != 0xFFFFFFFF)
		glDeleteTextures(1, &id);

	if (this->tex_owndata && data != NULL)
		delete[] data;

	std::vector<Texture*>::iterator it = std::remove(g_all_Textures.begin(), g_all_Textures.end(), this);
	if (it != g_all_Textures.end())
	{
		g_all_Textures.erase(it);
	}
	else
	{
		if (g_verbose)
		{
			print_log(PRINT_RED, "MISSING TEX BUFF IN TOTAL TEXTURES BUFF!\n");
		}
	}
}

unsigned char* Texture::get_data()
{
	if (data == NULL)
	{
		tex_owndata = true;
		data = new unsigned char[dataLen];
		if (id != 0xFFFFFFFF)
		{
			memset(binded_tex, 0, sizeof(binded_tex));
			glBindTexture(GL_TEXTURE_2D, id);
			glPixelStorei(GL_PACK_ALIGNMENT, 1);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glGetTexImage(GL_TEXTURE_2D, 0, format, GL_UNSIGNED_BYTE, data);
			//glReadPixels(0,0, width,height,format, GL_UNSIGNED_BYTE, data);
		}
		else
		{
			memset(data, 255, dataLen);
		}
	}
	return data;
}

void Texture::upload(int _type)
{
	this->type = _type;
	g_mutex_list[3].lock();
	get_data();

	if (id != 0xFFFFFFFF)
	{
		glDeleteTextures(1, &id);
	}

	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id); // Binds this texture handle so we can load the data into it

	// Set up filters and wrap mode
	if (type == TYPE_LIGHTMAP)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}
	else if (type == TYPE_TEXTURE)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, this->nearFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, this->farFilter);
	}
	else if (type == TYPE_DECAL)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, this->nearFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, this->farFilter);
	}

	if (texName[0] == '{')
	{
		if (format == GL_RGB)
		{
			format = GL_RGBA;
			COLOR3* rgbData = (COLOR3*)data;
			int pixelCount = width * height;
			COLOR4* rgbaData = new COLOR4[pixelCount];
			for (int i = 0; i < pixelCount; i++)
			{
				rgbaData[i] = rgbData[i];
				if (rgbaData[i].r == 0 && rgbaData[i].g == 0 && rgbaData[i].b > 250)
				{
					rgbaData[i] = COLOR4(0, 0, 0, 0);
				}
			}
			delete[] data;
			data = (unsigned char*)rgbaData;
			dataLen = (unsigned int)(width * height) * sizeof(COLOR4);
		}
	}

	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

	if (g_settings.verboseLogs)
		print_log(get_localized_string(LANG_0971), texName, width, height);

	if (tex_owndata)
	{
		delete[] data;
		data = NULL;
	}
	g_mutex_list[3].unlock();
}

Texture* binded_tex[64];

void Texture::bind(GLuint texnum)
{
	if (binded_tex[texnum] != this)
	{
		glActiveTexture(GL_TEXTURE0 + texnum);
		glBindTexture(GL_TEXTURE_2D, id);
		binded_tex[texnum] = this;
	}
}

bool IsTextureTransparent(const std::string& texname)
{
	if (!texname.size())
		return false;
	for (auto const& s : g_settings.transparentTextures)
	{
		if (s == texname)
			return true;
	}
	return false;
}