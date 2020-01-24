#include "viewport.h"

#include "CS248.h"

namespace CS248 {

void ViewportImp::set_viewbox( float x, float y, float span ) {

  // Task 3 (part 2): 
  // Set svg to normalized device coordinate transformation. Your input
  // arguments are defined as SVG canvans coordinates.

  this->x = x;
  this->y = y;
  this->span = span;

  Matrix3x3 m;

  // translate over by bottom left
  float left = x - span;
  float bottom = y - span;

  m(0, 0) = 1; m(0, 1) = 0; m(0, 2) = -left;
  m(1, 0) = 0; m(1, 1) = 1; m(1, 2) = -bottom;
  m(2, 0) = 0; m(2, 1) = 0; m(2, 2) = 1;
  Matrix3x3 translate_matrix(m);

  // scale down by 2 * span each way
  double scale = 1 / (2 * span);

  m(0, 0) = scale; m(0, 1) = 0; m(0, 2) = 0;
  m(1, 0) = 0; m(1, 1) = scale; m(1, 2) = 0;
  m(2, 0) = 0; m(2, 1) = 0; m(2, 2) = 1;
  Matrix3x3 scale_matrix(m);


  Matrix3x3 canvas_to_norm(scale_matrix * translate_matrix);

  set_canvas_to_norm(canvas_to_norm);
}

void ViewportImp::update_viewbox( float dx, float dy, float scale ) { 
  
  this->x -= dx;
  this->y -= dy;
  this->span *= scale;
  set_viewbox( x, y, span );
}

} // namespace CS248
