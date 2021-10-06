#ifndef MS_H
#define MS_H

#include "image.h"
#include "primitives.h"
#include "marching_squares.h"

#include <vector>
using std::vector;

#include <iostream>
using std::cout;
using std::endl;

#include <sstream>
using std::istringstream;

// Image objects and parameters.
tga tga_texture;
float_grayscale luma;

double template_width = 0;
double template_height = 0;
double step_size = 0;
double isovalue = 0;
double inverse_width = 0;
double grid_x_min = 0;
double grid_y_max = 0;

// Marching squares-generated geometric primitives.
vector<line_segment> line_segments;
vector<triangle> triangles;


#endif
