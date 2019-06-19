// AnimLab.cc

#include <spinach.h>

#include "Sprite.h"

int main(int argc, char **argv)
{
   if (!spinach_window_init("AnimLab", 1280, 720))
      return 0;

   spinach_texture_t texture = spinach_texture_load("Assets/sunny-land/player-idle.png");
   Sprite sprite;
   sprite.set_position({ 100, 100 });
   sprite.set_texture(texture);
   sprite.set_rectangle({ 0, 0, 32, 32 });

   int frames = 0;
   int index = 0;

   while (spinach_window_process())
   { 
      if (spinach_key_released(KEY_ESCAPE))
         break;

      auto mouse = spinach_mouse_position();
      sprite.set_position({ (float)mouse.x_, (float)mouse.y_ });

      frames++;
      if (frames > 5)
      { 
         frames = 0;
         index = (index + 1) % 4;
         sprite.set_rectangle({ index * 32, 0, 32, 32 });
      }

      sprite.draw();
   }

   return 0;
}
