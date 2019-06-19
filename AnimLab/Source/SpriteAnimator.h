// SpriteAnimator.h

#ifndef ANIMATOR_H_INCLUDED
#define ANIMATOR_H_INCLUDED

struct Sprite;
struct SpriteAnimation;

struct SpriteAnimator
{
   SpriteAnimator(Sprite &sprite, SpriteAnimation &animation);

   void update(float deltatime);
   void draw();

private:
   Sprite &sprite_;
   SpriteAnimation &animation_;
};

#endif // ANIMATOR_H_INCLUDED
