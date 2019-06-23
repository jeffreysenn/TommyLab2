// SpriteAnimator.cc

#include "SpriteAnimator.h"

#include "Sprite.h"
#include "SpriteAtlas.h"
#include <vector>


SpriteAnimator::SpriteAnimator(Sprite &sprite, SpriteAnimation &animation)
	: sprite_(sprite),
	animation_(animation)
{
	sprite.set_texture(animation.get_atlas().get_texture());
	update_frame();
}

void SpriteAnimator::update(float deltatime)
{
	time_since_current_frame_ += deltatime;

	if (time_since_current_frame_ >= current_key_frame_.duration_)
	{
		time_since_current_frame_ = 0;

		int32_t next_index = (signed)current_index_ + delta_index_;
		if (next_index < 0 || next_index > (signed) animation_.get_total_frames() - 1)
		{
			switch (animation_.get_type())
			{
			case SpriteAnimation::LOOPING:
				next_index = 0;
				break;
			case SpriteAnimation::PINGPONG:
				delta_index_ *= -1;
				next_index = (signed)current_index_ + delta_index_;
				break;
			case SpriteAnimation::PLAYONCE:
				return;
			default:
				break;
			}
		}

		current_index_ = (unsigned) next_index;

		update_frame();
	}
}

void SpriteAnimator::update_frame()
{
	if (animation_.get_keyframe(current_index_, current_key_frame_))
	{
		spinach_rect_t out_rect;
		if (animation_.get_atlas().get_rect(current_key_frame_.id_, out_rect))
		{
			sprite_.set_rectangle(out_rect);
		}
	}
}

void SpriteAnimator::draw()
{
	sprite_.draw();
}