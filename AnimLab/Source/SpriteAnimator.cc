// SpriteAnimator.cc

#include "SpriteAnimator.h"
#include "Sprite.h"
#include "SpriteAnimation.h"

SpriteAnimator::SpriteAnimator(Sprite &sprite, SpriteAnimation &animation)
   : sprite_(sprite)
   , animation_(animation)
{
}

void SpriteAnimator::update(float deltatime)
{
   // ...
}

void SpriteAnimator::draw()
{
   sprite_.draw();
}
