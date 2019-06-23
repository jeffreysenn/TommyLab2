// SpriteAtlas.cc


#include "SpriteAtlas.h"

#include "Sprite.h"
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>

SpriteAtlas::SpriteAtlas()
{
}

SpriteAtlas::~SpriteAtlas()
{
	delete texture_;
}


bool SpriteAtlas::get_rect(const uint32_t id, spinach_rect_t &rectangle) const
{
	if (id >= rects_.size()) { return false; }

	rectangle = rects_[id];
	return true;
}

bool SpriteAtlas::load(const char *filename)
{
	uint32_t lineCount = 0;
	std::string line;
	std::ifstream file(filename);
	if (file.is_open())
	{
		while (getline(file, line))
		{
			if (lineCount == 0)
			{
				texture_ = new spinach_texture_t();
				*texture_ = spinach_texture_load(line.c_str());
			}
			else
			{
				std::istringstream iss(line);
				std::vector<std::string> results((std::istream_iterator<std::string>(iss)), std::istream_iterator<std::string>());
				if (results.size() != 4)
				{
					spinach_log("[error] atlas has wrong format (%s)\n", filename);
					return false;
				}

				spinach_rect_t rect { std::stoi(results[0]), std::stoi(results[1]), std::stoi(results[2]), std::stoi(results[3]) };
				add_rect(rect);
			}
			lineCount++;
		}
		file.close();
	}

	return true;
}

void SpriteAtlas::clear()
{
	delete texture_;
	rects_.clear();
}
