// SpriteAtlas.cc

#include "SpriteAtlas.h"

SpriteAtlas::SpriteAtlas()
{
}

void SpriteAtlas::add(const spinach_rect_t &rectangle)
{
   sprites_.push_back(rectangle);
}

bool SpriteAtlas::get(const uint32_t id, spinach_rect_t &rectangle)
{
   if (sprites_.size() >= id)
      return false;

   rectangle = sprites_[id];

   return true;
}

bool SpriteAtlas::load(const char *filename)
{
   return true;
}

void SpriteAtlas::clear()
{
   sprites_.clear();
}
