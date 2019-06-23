// SpriteAtlas.h

#ifndef SPRITE_ATLAS_H_INCLUDED
#define SPRITE_ATLAS_H_INCLUDED

#include <spinach.h>
#include <vector>

struct SpriteAtlas
{
	SpriteAtlas();
	~SpriteAtlas();

	void add_rect(const spinach_rect_t &rectangle) { rects_.push_back(rectangle); }

	bool get_rect(const uint32_t id, spinach_rect_t &rectangle) const;

	bool load(const char *filename);

	void clear();

	const spinach_texture_t* get_texture() const { return texture_; }

private:
	std::vector<spinach_rect_t> rects_;
	spinach_texture_t* texture_ = nullptr;
};

#endif // SPRITE_ATLAS_H_INCLUDED
