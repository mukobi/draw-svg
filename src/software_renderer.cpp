#include "software_renderer.h"

#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>

#include "triangulation.h"

using namespace std;

namespace CS248 {


// Implements SoftwareRenderer //

// fill a sample location with color
void SoftwareRendererImp::fill_sample(int sx, int sy, const Color &color) {
  // check bounds
  if (sx < 0 || sx >= sample_w) return;
  if (sy < 0 || sy >= sample_h) return;

  Color sample_color;
  float inv255 = 1.0 / 255.0;
  sample_color.r = sample_buffer[4 * (sx + sy * sample_w)] * inv255;
  sample_color.g = sample_buffer[4 * (sx + sy * sample_w) + 1] * inv255;
  sample_color.b = sample_buffer[4 * (sx + sy * sample_w) + 2] * inv255;
  sample_color.a = sample_buffer[4 * (sx + sy * sample_w) + 3] * inv255;

  sample_color = ref->alpha_blending_helper(sample_color, color);

  sample_buffer[4 * (sx + sy * sample_w)] = (uint8_t)(sample_color.r * 255);
  sample_buffer[4 * (sx + sy * sample_w) + 1] = (uint8_t)(sample_color.g * 255);
  sample_buffer[4 * (sx + sy * sample_w) + 2] = (uint8_t)(sample_color.b * 255);
  sample_buffer[4 * (sx + sy * sample_w) + 3] = (uint8_t)(sample_color.a * 255);
}

// fill samples in the entire pixel specified by pixel coordinates
void SoftwareRendererImp::fill_pixel(int x, int y, const Color &color) {

	// Task 2: Re-implement this function

  // fill a sample for each sample in the pixel
  for (int sx = 0; sx < sample_rate; sx++) {
    for (int sy = 0; sy < sample_rate; sy++) {
      fill_sample(x * sample_rate + sx, y * sample_rate + sy, color);
    }
  }
}

void SoftwareRendererImp::draw_svg( SVG& svg ) {

  // set top level transformation
  transformation = canvas_to_screen;

  // draw all elements
  for ( size_t i = 0; i < svg.elements.size(); ++i ) {
    draw_element(svg.elements[i]);
  }

  // draw canvas outline
  Vector2D a = transform(Vector2D(    0    ,     0    )); a.x--; a.y--;
  Vector2D b = transform(Vector2D(svg.width,     0    )); b.x++; b.y--;
  Vector2D c = transform(Vector2D(    0    ,svg.height)); c.x--; c.y++;
  Vector2D d = transform(Vector2D(svg.width,svg.height)); d.x++; d.y++;

  rasterize_line(a.x, a.y, b.x, b.y, Color::Black);
  rasterize_line(a.x, a.y, c.x, c.y, Color::Black);
  rasterize_line(d.x, d.y, b.x, b.y, Color::Black);
  rasterize_line(d.x, d.y, c.x, c.y, Color::Black);

  // resolve and send to render target
  resolve();

}

void SoftwareRendererImp::set_sample_rate( size_t sample_rate ) {

  // Task 2: 
  // You may want to modify this for supersampling support
  this->sample_rate = sample_rate;
  this->sample_w = target_w * sample_rate;
  this->sample_h = target_h * sample_rate;
}

void SoftwareRendererImp::set_render_target( unsigned char* render_target,
                                             size_t width, size_t height ) {

  // Task 2: 
  // You may want to modify this for supersampling support
  this->render_target = render_target;
  this->target_w = width;
  this->target_h = height;

  this->sample_w = target_w * sample_rate;
  this->sample_h = target_h * sample_rate;
}

void SoftwareRendererImp::draw_element( SVGElement* element ) {

	// Task 3 (part 1):
	// Modify this to implement the transformation stack

	switch (element->type) {
	case POINT:
		draw_point(static_cast<Point&>(*element));
		break;
	case LINE:
		draw_line(static_cast<Line&>(*element));
		break;
	case POLYLINE:
		draw_polyline(static_cast<Polyline&>(*element));
		break;
	case RECT:
		draw_rect(static_cast<Rect&>(*element));
		break;
	case POLYGON:
		draw_polygon(static_cast<Polygon&>(*element));
		break;
	case ELLIPSE:
		draw_ellipse(static_cast<Ellipse&>(*element));
		break;
	case IMAGE:
		draw_image(static_cast<Image&>(*element));
		break;
	case GROUP:
		draw_group(static_cast<Group&>(*element));
		break;
	default:
		break;
	}

}


// Primitive Drawing //

void SoftwareRendererImp::draw_point( Point& point ) {

  Vector2D p = transform(point.position);
  rasterize_point( p.x, p.y, point.style.fillColor );

}

void SoftwareRendererImp::draw_line( Line& line ) { 

  Vector2D p0 = transform(line.from);
  Vector2D p1 = transform(line.to);
  rasterize_line( p0.x, p0.y, p1.x, p1.y, line.style.strokeColor );

}

void SoftwareRendererImp::draw_polyline( Polyline& polyline ) {

  Color c = polyline.style.strokeColor;

  if( c.a != 0 ) {
    int nPoints = polyline.points.size();
    for( int i = 0; i < nPoints - 1; i++ ) {
      Vector2D p0 = transform(polyline.points[(i+0) % nPoints]);
      Vector2D p1 = transform(polyline.points[(i+1) % nPoints]);
      rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    }
  }
}

void SoftwareRendererImp::draw_rect( Rect& rect ) {

  Color c;
  
  // draw as two triangles
  float x = rect.position.x;
  float y = rect.position.y;
  float w = rect.dimension.x;
  float h = rect.dimension.y;

  Vector2D p0 = transform(Vector2D(   x   ,   y   ));
  Vector2D p1 = transform(Vector2D( x + w ,   y   ));
  Vector2D p2 = transform(Vector2D(   x   , y + h ));
  Vector2D p3 = transform(Vector2D( x + w , y + h ));
  
  // draw fill
  c = rect.style.fillColor;
  if (c.a != 0 ) {
    rasterize_triangle( p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c );
    rasterize_triangle( p2.x, p2.y, p1.x, p1.y, p3.x, p3.y, c );
  }

  // draw outline
  c = rect.style.strokeColor;
  if( c.a != 0 ) {
    rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    rasterize_line( p1.x, p1.y, p3.x, p3.y, c );
    rasterize_line( p3.x, p3.y, p2.x, p2.y, c );
    rasterize_line( p2.x, p2.y, p0.x, p0.y, c );
  }

}

void SoftwareRendererImp::draw_polygon( Polygon& polygon ) {

  Color c;

  // draw fill
  c = polygon.style.fillColor;
  if( c.a != 0 ) {

    // triangulate
    vector<Vector2D> triangles;
    triangulate( polygon, triangles );

    // draw as triangles
    for (size_t i = 0; i < triangles.size(); i += 3) {
      Vector2D p0 = transform(triangles[i + 0]);
      Vector2D p1 = transform(triangles[i + 1]);
      Vector2D p2 = transform(triangles[i + 2]);
      rasterize_triangle( p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c );
    }
  }

  // draw outline
  c = polygon.style.strokeColor;
  if( c.a != 0 ) {
    int nPoints = polygon.points.size();
    for( int i = 0; i < nPoints; i++ ) {
      Vector2D p0 = transform(polygon.points[(i+0) % nPoints]);
      Vector2D p1 = transform(polygon.points[(i+1) % nPoints]);
      rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    }
  }
}

void SoftwareRendererImp::draw_ellipse( Ellipse& ellipse ) {

  // Extra credit 

}

void SoftwareRendererImp::draw_image( Image& image ) {

  Vector2D p0 = transform(image.position);
  Vector2D p1 = transform(image.position + image.dimension);

  rasterize_image( p0.x, p0.y, p1.x, p1.y, image.tex );
}

void SoftwareRendererImp::draw_group( Group& group ) {

  for ( size_t i = 0; i < group.elements.size(); ++i ) {
    draw_element(group.elements[i]);
  }

}

// Rasterization //

// The input arguments in the rasterization functions 
// below are all defined in screen space coordinates

void SoftwareRendererImp::rasterize_point( float x, float y, Color color ) {

  // fill in the nearest pixel
  int sx = (int)floor(x);
  int sy = (int)floor(y);

  // check bounds
  if (sx < 0 || sx >= target_w) return;
  if (sy < 0 || sy >= target_h) return;

  // fill sample - NOT doing alpha blending!
  // TODO: Call fill_pixel here to run alpha blending
  /*render_target[4 * (sx + sy * target_w)] = (uint8_t)(color.r * 255);
  render_target[4 * (sx + sy * target_w) + 1] = (uint8_t)(color.g * 255);
  render_target[4 * (sx + sy * target_w) + 2] = (uint8_t)(color.b * 255);
  render_target[4 * (sx + sy * target_w) + 3] = (uint8_t)(color.a * 255);*/

  fill_pixel(sx, sy, color);

}

void SoftwareRendererImp::rasterize_line( float x0, float y0,
                                          float x1, float y1,
                                          Color color) {

  // Extra credit (delete the line below and implement your own)
  ref->rasterize_line_helper(x0, y0, x1, y1, target_w, target_h, color, this);

}

void SoftwareRendererImp::rasterize_triangle( float x0, float y0,
                                              float x1, float y1,
                                              float x2, float y2,
                                              Color color ) {
  // Task 1: 
  // Implement triangle rasterization (you may want to call fill_sample here)

  // fill in the nearest pixel
  int sx0 = (int)floor(x0);
  int sx1 = (int)floor(x1);
  int sx2 = (int)floor(x2);
  int sy0 = (int)floor(y0);
  int sy1 = (int)floor(y1);
  int sy2 = (int)floor(y2);

  // compute bounding box
  int xMin = min({ sx0, sx1, sx2});
  int xMax = max({ sx0, sx1, sx2});
  int yMin = min({ sy0, sy1, sy2});
  int yMax = max({ sy0, sy1, sy2});

  // check bounds
  xMin = max(xMin, 0);
  yMin = max(yMin, 0);
  xMax = min(xMax, int(target_w - 1));
  yMax = min(yMax, int(target_h - 1));

  // iterate over bounded pixels
  for (int sx = xMin; sx <= xMax; sx++) {
    for (int sy = yMin; sy <= yMax; sy++) {
      if (edge(sx, sy, sx0, sy0, sx1, sy1) <= 0 &&
          edge(sx, sy, sx1, sy1, sx2, sy2) <= 0 &&
          edge(sx, sy, sx2, sy2, sx0, sy0) <= 0) { // in triangle

        // fill pixel - alpha blending!
        fill_pixel(sx, sy, color);
      }
    }
  }

}

void SoftwareRendererImp::rasterize_image( float x0, float y0,
                                           float x1, float y1,
                                           Texture& tex ) {
  // Task 4: 
  // Implement image rasterization (you may want to call fill_sample here)

}

// resolve samples to render target
void SoftwareRendererImp::resolve( void ) {

  // Task 2: 
  // Implement supersampling
  // You may also need to modify other functions marked with "Task 2".


  for (int px = 0; px < target_w; px++) {
    for (int py = 0; py < target_h; py++) {
      // average samples per pixel
      Color pixel_color;
      // sum
      for (int sx = px * sample_rate; sx < px*sample_rate + sample_rate; sx++) {
        for (int sy = py * sample_rate; sy < py * sample_rate + sample_rate; sy++) {
          pixel_color.r += sample_buffer[4 * (sx + sy * sample_w)];
          pixel_color.g += sample_buffer[4 * (sx + sy * sample_w) + 1];
          pixel_color.b += sample_buffer[4 * (sx + sy * sample_w) + 2];
          pixel_color.a += sample_buffer[4 * (sx + sy * sample_w) + 3];
        }
      }
      // divide
      pixel_color.r /= sample_rate;
      pixel_color.g /= sample_rate;
      pixel_color.b /= sample_rate;
      pixel_color.a /= sample_rate;
      render_target[4 * (px + py * target_w)] = (uint8_t)(pixel_color.r * 255);
      render_target[4 * (px + py * target_w) + 1] = (uint8_t)(pixel_color.g * 255);
      render_target[4 * (px + py * target_w) + 2] = (uint8_t)(pixel_color.b * 255);
      render_target[4 * (px + py * target_w) + 3] = (uint8_t)(pixel_color.a * 255);
    }
  }
}

int SoftwareRendererImp::edge(int x, int y, int p0x, int p0y, int p1x, int p1y)
{
  // evaluates the edge equation: the side of the edge from
  // p0 to p1 that (x,y) lies on
  int dx = p1x - p0x;
  int dy = p1y - p0y;
  return (x - p0x) * dy - (y - p0y) * dx;
}


} // namespace CS248
