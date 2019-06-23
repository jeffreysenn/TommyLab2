// SpriteAnimator.h

#ifndef ANIMATOR_H_INCLUDED
#define ANIMATOR_H_INCLUDED

#include <cstdint>

#include "SpriteAnimation.h"


struct SpriteAnimator
{
	SpriteAnimator(struct Sprite &sprite, SpriteAnimation &animation);

	void update(float deltatime);

	void draw();

private:
	void update_frame();

	struct Sprite& sprite_;

	SpriteAnimation& animation_;

	int32_t current_index_ = 0;
	int32_t delta_index_ = 1;

	float time_since_current_frame_ = 0;

	struct SpriteAnimation::Keyframe current_key_frame_;
};

#endif // ANIMATOR_H_INCLUDED
