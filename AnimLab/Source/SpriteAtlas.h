// SpriteAtlas.h

#ifndef SPRITE_ATLAS_H_INCLUDED
#define SPRITE_ATLAS_H_INCLUDED

#include <spinach.h>
#include <vector>

struct SpriteAtlas
{
   SpriteAtlas();

   void add(const spinach_rect_t &rectangle);
   bool get(const uint32_t id, spinach_rect_t &rectangle);

   bool load(const char *filename);
   void clear();

private:
   std::vector<spinach_rect_t> sprites_;
};

#endif // SPRITE_ATLAS_H_INCLUDED
