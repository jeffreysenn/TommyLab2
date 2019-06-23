// SpriteAnimation.h

#ifndef SPRITE_ANIMATION_H_INCLUDED
#define SPRITE_ANIMATION_H_INCLUDED

#include <spinach.h>
#include <vector>

struct SpriteAnimation
{
	enum Type
	{
		LOOPING,
		PINGPONG,
		PLAYONCE,
	};

	struct Keyframe
	{
		uint32_t id_;
		float duration_;
	};

	SpriteAnimation(struct SpriteAtlas &atlas);

	Type get_type() const { return type_; }

	const struct SpriteAtlas& get_atlas() const { return atlas_; }

	uint32_t get_total_frames() const { return (uint32_t) keyframes_.size(); }

	bool get_keyframe(const uint32_t index, Keyframe &keyframe) const;

	void set_type(const Type &type) { type_ = type; }

	bool load(const char* filename);

private:
	struct SpriteAtlas &atlas_;

	Type type_ = LOOPING;

	std::vector<Keyframe> keyframes_;

	void add_keyframe(const Keyframe& keyframe) { keyframes_.push_back(keyframe); }
};

#endif // SPRITE_ANIMATION_H_INCLUDED
