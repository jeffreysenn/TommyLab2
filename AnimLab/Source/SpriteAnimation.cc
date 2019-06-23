// SpriteAnimation.cc

#include "SpriteAnimation.h"

#include "SpriteAtlas.h"
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>

SpriteAnimation::SpriteAnimation(SpriteAtlas &atlas)
	: atlas_(atlas)
{
}

bool SpriteAnimation::get_keyframe(const uint32_t index, Keyframe &keyframe) const
{
	if (index >= keyframes_.size()) { return false; }

	keyframe = keyframes_[index];
	return true;
}

bool SpriteAnimation::load(const char* filename)
{
	uint32_t lineCount = 0;
	std::string line;
	std::ifstream file(filename);
	if (file.is_open())
	{
		while (getline(file, line))
		{
			// Load for atlas
			if (lineCount == 0)
			{
				atlas_.load(line.c_str());
			}
			// Set type
			else if (lineCount == 1)
			{
				std::for_each(line.begin(), line.end(), [](char &c){
					c = ::toupper(c);
				});

				if (line == "LOOPING")
				{
					type_ = LOOPING;
				}
				else if (line == "PINGPONG")
				{
					type_ = PINGPONG;
				}
				else if (line == "PLAYONCE")
				{
					type_ = PLAYONCE;
				}
				else
				{
					spinach_log("[error] animation looping type has wrong format (%s)\n", filename);
					return false;
				}

			}
			// Add keyframes
			else
			{
				std::string space = " ";
				std::string splitter = "/";

				size_t space_pose = 0;

				do
				{
					space_pose = line.find(space);
					size_t end_pose = (space_pose != std::string::npos) ? space_pose : line.length();
					std::string frame_info = line.substr(0, end_pose);

					size_t splitter_pose = 0;
					if ((splitter_pose = frame_info.find(splitter)) != std::string::npos)
					{
						uint32_t frame_id = std::stoi(frame_info.substr(0, splitter_pose));
						float frame_duration = std::stof(frame_info.substr(splitter_pose + splitter.length(), frame_info.length()));
						Keyframe keyframe { frame_id, frame_duration };
						add_keyframe(keyframe);
					}
					else
					{
						spinach_log("[error] animation has wrong format (%s)\n", filename);
						return false;
					}

					line.erase(0, end_pose + space.length());
				} while (space_pose != std::string::npos);
			}
			lineCount++;
		}
		file.close();
		return true;
	}
	return false;
}
