#include "texture.h"
#include "color.h"

#include <assert.h>
#include <iostream>
#include <algorithm>

using namespace std;

namespace CS248 {

inline void uint8_to_float( float dst[4], unsigned char* src ) {
  uint8_t* src_uint8 = (uint8_t *)src;
  dst[0] = src_uint8[0] / 255.f;
  dst[1] = src_uint8[1] / 255.f;
  dst[2] = src_uint8[2] / 255.f;
  dst[3] = src_uint8[3] / 255.f;
}

inline void float_to_uint8( unsigned char* dst, float src[4] ) {
  uint8_t* dst_uint8 = (uint8_t *)dst;
  dst_uint8[0] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[0])));
  dst_uint8[1] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[1])));
  dst_uint8[2] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[2])));
  dst_uint8[3] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[3])));
}

void Sampler2DImp::generate_mips(Texture& tex, int startLevel) {

  // NOTE: 
  // This starter code allocates the mip levels and generates a level 
  // map by filling each level with placeholder data in the form of a 
  // color that differs from its neighbours'. You should instead fill
  // with the correct data!

  // Extra credit: Implement this

  // check start level
  if ( startLevel >= tex.mipmap.size() ) {
    std::cerr << "Invalid start level"; 
  }

  // allocate sublevels
  int baseWidth  = tex.mipmap[startLevel].width;
  int baseHeight = tex.mipmap[startLevel].height;
  int numSubLevels = (int)(log2f( (float)max(baseWidth, baseHeight)));

  numSubLevels = min(numSubLevels, kMaxMipLevels - startLevel - 1);
  tex.mipmap.resize(startLevel + numSubLevels + 1);

  int width  = baseWidth;
  int height = baseHeight;
  for (int i = 1; i <= numSubLevels; i++) {

    MipLevel& level = tex.mipmap[startLevel + i];

    // handle odd size texture by rounding down
    width  = max( 1, width  / 2); assert(width  > 0);
    height = max( 1, height / 2); assert(height > 0);

    level.width = width;
    level.height = height;
    level.texels = vector<unsigned char>(4 * width * height);

  }

  // fill all 0 sub levels with interchanging colors
  Color colors[3] = { Color(1,0,0,1), Color(0,1,0,1), Color(0,0,1,1) };
  for(size_t i = 1; i < tex.mipmap.size(); ++i) {

    Color c = colors[i % 3];
    MipLevel& mip = tex.mipmap[i];

    for(size_t i = 0; i < 4 * mip.width * mip.height; i += 4) {
      float_to_uint8( &mip.texels[i], &c.r );
    }
  }

}

Color Sampler2DImp::sample_nearest(Texture& tex, 
                                   float u, float v, 
                                   int level) {

  // Task 4: Implement nearest neighbour interpolation
  
  Color sample_color;
  auto mip_level = tex.mipmap[level];
  auto texels = mip_level.texels;
  float inv255 = 1.0 / 255.0;

  int x = int(u * mip_level.width);
  int y = int(v * mip_level.height);

  size_t index = x + y * mip_level.width;
   
  sample_color.r = float(texels[4 * index]) * inv255;
  sample_color.g = float(texels[4 * (index) + 1]) * inv255;
  sample_color.b = float(texels[4 * (index) + 2]) * inv255;
  sample_color.a = float(texels[4 * (index) + 3]) * inv255;
  return sample_color;
}

float lerp(float x, float v0, float v1) {
  return v0 + x * (v1 - v0);
}

Color Sampler2DImp::sample_bilinear(Texture& tex, 
                                    float u, float v, 
                                    int level) {
  
  // Task 4: Implement bilinear filtering

  auto mip_level = tex.mipmap[level];
  auto texels = mip_level.texels;
  float inv255 = 1.0 / 255.0;

  float x = u * mip_level.width;
  float y = v * mip_level.height;

  int nearest_x = int(floor(x - 1));
  int nearest_y = int(floor(y - 1));

  Color white;
  white.r = 1.0;
  white.g = 1.0;
  white.b = 1.0;
  white.a = 1.0;

  // u00
  size_t index = nearest_x + nearest_y * mip_level.width;

  Color u00;
  u00.r = float(texels[4 * index]) * inv255;
  u00.g = float(texels[4 * (index)+1]) * inv255;
  u00.b = float(texels[4 * (index)+2]) * inv255;
  u00.a = float(texels[4 * (index)+3]) * inv255;

  // u01
  index = nearest_x + (nearest_y+1) * mip_level.width;

  Color u01;
  u01.r = float(texels[4 * index]) * inv255;
  u01.g = float(texels[4 * (index)+1]) * inv255;
  u01.b = float(texels[4 * (index)+2]) * inv255;
  u01.a = float(texels[4 * (index)+3]) * inv255;
  
  // u10
  index = (nearest_x+1) + nearest_y * mip_level.width;

  Color u10;
  u10.r = float(texels[4 * index]) * inv255;
  u10.g = float(texels[4 * (index)+1]) * inv255;
  u10.b = float(texels[4 * (index)+2]) * inv255;
  u10.a = float(texels[4 * (index)+3]) * inv255;

  // u11
  index = (nearest_x + 1) + (nearest_y + 1) * mip_level.width;

  Color u11;
  u11.r = float(texels[4 * index]) * inv255;
  u11.g = float(texels[4 * (index)+1]) * inv255;
  u11.b = float(texels[4 * (index)+2]) * inv255;
  u11.a = float(texels[4 * (index)+3]) * inv255;

  float s = (x - (float(nearest_x)));
  float t = (y - (float(nearest_y)));

  // two helper lerps
  Color u0;
  u0.r = lerp(s, u00.r, u10.r);
  u0.g = lerp(s, u00.g, u10.g);
  u0.b = lerp(s, u00.b, u10.b);
  u0.a = lerp(s, u00.a, u10.a);

  Color u1;
  u1.r = lerp(s, u01.r, u11.r);
  u1.g = lerp(s, u01.g, u11.g);
  u1.b = lerp(s, u01.b, u11.b);
  u1.a = lerp(s, u01.a, u11.a);

  // final helper lerp

  Color final_color;
  final_color.r = lerp(t, u0.r, u1.r);
  final_color.g = lerp(t, u0.g, u1.g);
  final_color.b = lerp(t, u0.b, u1.b);
  final_color.a = lerp(t, u0.a, u1.a);

  return final_color;

}

Color Sampler2DImp::sample_trilinear(Texture& tex, 
                                     float u, float v, 
                                     float u_scale, float v_scale) {

  // Extra credit: Implement trilinear filtering

  // return magenta for invalid level
  return Color(1,0,1,1);

}

} // namespace CS248
