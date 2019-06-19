// SpriteAnimation.h

#ifndef SPRITE_ANIMATION_H_INCLUDED
#define SPRITE_ANIMATION_H_INCLUDED

#include <spinach.h>
#include <vector>

struct SpriteAtlas;

struct SpriteAnimation
{
   enum Type
   {
      LOOPING,
      PINGPONG,
      PLAYONCE,
   };

   SpriteAnimation(SpriteAtlas &atlas);

private:
   SpriteAtlas &atlas_;

   // note: this is just an example of
   //       how a "keyframe" might look like
   struct Keyframe
   {
      uint32_t id_;
      float duration_;
   };

   std::vector<Keyframe> keyframes_;
};

#endif // SPRITE_ANIMATION_H_INCLUDED
