// Sprite.cc

#include "Sprite.h"

Sprite::Sprite()
   : texture_(nullptr)
   , position_({})
   , rectangle_({})
{
}

void Sprite::draw()
{
   if (!texture_)
      return;

   spinach_texture_bind(texture_);
   spinach_render(4, vertices_);
}

void Sprite::set_position(const spinach_vector2_t &position)
{
   position_ = position;

   update_positions();
}

void Sprite::set_texture(const spinach_texture_t* texture)
{
   texture_ = texture;

   update_texcoords();
}

void Sprite::set_rectangle(const spinach_rect_t &rectangle)
{
   rectangle_ = rectangle;
   update_positions();
   update_texcoords();
}

void Sprite::update_positions()
{
   vertices_[0].position_ = { position_.x_                    , position_.y_                      };
   vertices_[1].position_ = { position_.x_ + rectangle_.width_, position_.y_                      };
   vertices_[2].position_ = { position_.x_ + rectangle_.width_, position_.y_ + rectangle_.height_ };
   vertices_[3].position_ = { position_.x_                    , position_.y_ + rectangle_.height_ };
}

void Sprite::update_texcoords()
{
   if (!texture_)
      return;

   const float u_scale_factor = 1.0f / float(texture_->width_);
   const float v_scale_factor = 1.0f / float(texture_->height_);

   vertices_[0].texcoord_ = { rectangle_.x_ * u_scale_factor                      , rectangle_.y_ * v_scale_factor                        };
   vertices_[1].texcoord_ = { (rectangle_.x_ + rectangle_.width_) * u_scale_factor, rectangle_.y_ * v_scale_factor                        };
   vertices_[2].texcoord_ = { (rectangle_.x_ + rectangle_.width_) * u_scale_factor, (rectangle_.y_ + rectangle_.height_) * v_scale_factor };
   vertices_[3].texcoord_ = { rectangle_.x_ * u_scale_factor                      , (rectangle_.y_ + rectangle_.height_) * v_scale_factor };
}
