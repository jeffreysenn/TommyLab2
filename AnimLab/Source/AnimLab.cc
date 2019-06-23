// AnimLab.cc

#include <spinach.h>
#include <array>

#include "Sprite.h"
#include "SpriteAtlas.h"
#include "SpriteAnimation.h"
#include "SpriteAnimator.h"

#include <string>
#include <iostream>
#include <chrono>

int main(int argc, char **argv)
{
   if (!spinach_window_init("AnimLab", 1280, 720))
      return 0;

   Sprite mySprite;
   SpriteAtlas atlas;
   SpriteAnimation animation(atlas);
   animation.load("Assets/sunny-land/Animations/player-idle.txt");

   SpriteAnimator animator(mySprite, animation);

   auto last_frame_time = std::chrono::high_resolution_clock::now();
   auto this_frame_time = last_frame_time;

   while (spinach_window_process())
   { 
      if (spinach_key_released(KEY_ESCAPE))
         break;

	  this_frame_time = std::chrono::high_resolution_clock::now();
	  std::chrono::duration<double> diff = this_frame_time - last_frame_time;
	  last_frame_time = this_frame_time;
	  float delta_time = diff.count();

      auto mouse = spinach_mouse_position();

	  mySprite.set_position({ (float)mouse.x_, (float)mouse.y_ });
	  animator.update(delta_time);
	  animator.draw();

   }

   return 0;
}
