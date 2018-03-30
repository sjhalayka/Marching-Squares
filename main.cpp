#include "main.h"


// Cat image from: http://www.iacuc.arizona.edu/training/cats/index.html
int main(int argc, char **argv)
{
	// Example command for standard binary image: ms figure1.tga 1e-3 0.5
	// Example command for blurred binary image: ms figure3.tga 1e-3 0.5
	// Example command for noise binary image: ms figure5.tga 1e-3 0.5
	if(4 != argc)
	{
		cout << "Usage: " << argv[0] << " file.tga template_width_in_metres isovalue" << endl;
		return 0;
	}

	// Read a 24-bit uncompressed/non-RLE Targa file, and then convert it to a floating point grayscale image.
	cout << "Reading luma..." << endl;
	cout << endl;

	if(false == convert_tga_to_float_grayscale(argv[1], tga_texture, luma, true, true, true))
		return 0;

	// Too small.
	if(luma.px < 3 || luma.py < 3)
	{
		cout << "Template must be at least 3x3 pixels in size (minimum 2x2 marching cubes)." << endl;
		return 0;
	}

	// If rendering problems occur, try using images of equal width and height (e.g. px = py).
	// Also try sizes that are powers of two (e.g. px = py = 2^x, x = 0, 1, 2, 3, ...).

	// Get amplitude mask width.
	istringstream iss(argv[2]);
	iss >> template_width;
	inverse_width = 1.0/template_width;
	step_size = template_width/static_cast<double>(luma.px - 1);
	template_height = step_size*(luma.py - 1); // Assumes square pixels.

	// Get marching squares isovalue.
	iss.clear();
	iss.str(argv[3]);
	iss >> isovalue;

	if(0 >= isovalue || 1 <= isovalue)
	{
		cout << "Isovalue must be 0 < i < 1." << endl;
		return 0;
	}

	// Print basic information.
	cout << "Template info: " << endl;
	cout << luma.px << " x " << luma.py << " pixels" << endl;
	cout << template_width << " x " << template_height << " metres" << endl;
	cout << endl;

	grid_x_min = -template_width/2.0;
	grid_y_max = template_height/2.0;
	
	cout << "Grid info: " << endl;
	cout << luma.px - 1 << " x " << luma.py - 1 << " grid squares" << endl;
	cout << "x min (-x max): " << grid_x_min << endl;
	cout << "y min (-y max): " << -grid_y_max << endl;
	cout << "Isovalue: " << isovalue << endl;
	cout << endl;


	// Generate geometric primitives using marching squares.
	grid_square g;

	cout << "Generating geometric primitives..." << endl;
	cout << endl;

	double grid_x_pos = grid_x_min; // Start at minimum x.
	double grid_y_pos = grid_y_max; // Start at maximum y.

	// Begin march.
	for(short unsigned int y = 0; y < luma.py - 1; y++, grid_y_pos -= step_size, grid_x_pos = grid_x_min)
	{
		for(short unsigned int x = 0; x < luma.px - 1; x++, grid_x_pos += step_size)
		{
			// Corner vertex order: 03
			//                      12
			// e.g.: clockwise, as in OpenGL
			g.vertex[0] = vertex_2(grid_x_pos, grid_y_pos);
			g.vertex[1] = vertex_2(grid_x_pos, grid_y_pos - step_size);
			g.vertex[2] = vertex_2(grid_x_pos + step_size, grid_y_pos - step_size);
			g.vertex[3] = vertex_2(grid_x_pos + step_size, grid_y_pos);

			g.value[0] = luma.pixel_data[y*luma.px + x];
			g.value[1] = luma.pixel_data[(y + 1)*luma.px + x];
			g.value[2] = luma.pixel_data[(y + 1)*luma.px + (x + 1)];
			g.value[3] = luma.pixel_data[y*luma.px + (x + 1)];

			g.generate_primitives(line_segments, triangles, isovalue);
		}
	}


	// Gather and print final information
	double length = 0;
	double area = 0;
	double sa = 0;

	double x_max = grid_x_min;
	double x_min = -grid_x_min;
	double y_max = -grid_y_max;
	double y_min = grid_y_max;

	for(size_t i = 0; i < line_segments.size(); i++)
	{
		length += line_segments[i].length();

		if(line_segments[i].vertex[0].x > x_max)
			x_max = line_segments[i].vertex[0].x; 

		if(line_segments[i].vertex[1].x > x_max)
			x_max = line_segments[i].vertex[1].x;

		if(line_segments[i].vertex[0].x < x_min)
			x_min = line_segments[i].vertex[0].x;

		if(line_segments[i].vertex[1].x < x_min)
			x_min = line_segments[i].vertex[1].x;

		if(line_segments[i].vertex[0].y > y_max)
			y_max = line_segments[i].vertex[0].y;

		if(line_segments[i].vertex[1].y > y_max)
			y_max = line_segments[i].vertex[1].y;

		if(line_segments[i].vertex[0].y < y_min)
			y_min = line_segments[i].vertex[0].y;

		if(line_segments[i].vertex[1].y < y_min)
			y_min = line_segments[i].vertex[1].y;
	}

	for(size_t i = 0; i < triangles.size(); i++)
		area += triangles[i].area();

	if(0 != area)
		sa = length/area;

	cout << "Geometric primitive info: " << endl;
	cout << "Vertex x min, max: " << x_min << ", " << x_max << endl;
	cout << "Vertex y min, max: " << y_min << ", " << y_max << endl;
	cout << "Line segments:     " << line_segments.size() << endl;
	cout << "Length:            " << length << endl;
	cout << "Triangles:         " << triangles.size() << endl;
	cout << "Area:              " << area << endl;
	cout << "Length/Area:       " << sa << endl;
	cout << "Photon wavelength: " << 6.0/sa << endl;


	// Render the Targa image underneath the associated geometric primitives,
	// using OpenGL fixed-pipeline functionality.
	render_image(argc, argv);

	return 0;
}

void render_image(int &argc, char ** &argv)
{
	// Initialize OpenGL.
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB);
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(win_x, win_y);
	win_id = glutCreateWindow("Marching Squares");
	glutIdleFunc(idle_func);
	glutReshapeFunc(reshape_func);
	glutDisplayFunc(display_func);

	glShadeModel(GL_FLAT);
	glClearColor(background_colour, background_colour, background_colour, 1);

	// Begin rendering.
	glutMainLoop();

	// Cleanup OpenGL.
	glutDestroyWindow(win_id);
}

void idle_func(void)
{
	glutPostRedisplay();
}

// Resize window.
void reshape_func(int width, int height)
{
	if(width < 1)
		width = 1;

	if(height < 1)
		height = 1;

	win_x = width;
	win_y = height;

	// Viewport setup.
	glutSetWindow(win_id);
	glutReshapeWindow(win_x, win_y);
	glViewport(0, 0, win_x, win_y);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0f, static_cast<GLfloat>(win_x)/static_cast<GLfloat>(win_y), 0.1, 10);
	gluLookAt(0, 0, camera_z, 0, 0, 0, 0, 1, 0);
}

// Visualization.
void display_func(void)
{
	glClear(GL_COLOR_BUFFER_BIT);

	// Scale all geometric primitives so that template width == 1.
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glScalef(inverse_width, inverse_width, inverse_width);

	// Render a dark background.
	glColor3f(0, 0, 0);
	glBegin(GL_QUADS);
		glVertex2f(-template_width/2.0, template_height/2.0);
		glVertex2f(-template_width/2.0, -template_height/2.0);
		glVertex2f(template_width/2.0, -template_height/2.0);
		glVertex2f(template_width/2.0, template_height/2.0);
	glEnd();

	// Render image area.
	glColor3f(1, 1, 1);
	glBegin(GL_TRIANGLES);
	for(size_t i = 0; i < triangles.size(); i++)
	{
		glVertex2f(triangles[i].vertex[0].x, triangles[i].vertex[0].y);
		glVertex2f(triangles[i].vertex[1].x, triangles[i].vertex[1].y);
		glVertex2f(triangles[i].vertex[2].x, triangles[i].vertex[2].y);
	}
	glEnd();

	//// Render image outline edge length.
	//glColor3f(0, 0, 1);
	//glLineWidth(2);
	//glBegin(GL_LINES);
	//for(size_t i = 0; i < line_segments.size(); i++)
	//{
	//	glVertex2f(line_segments[i].vertex[0].x, line_segments[i].vertex[0].y);
	//	glVertex2f(line_segments[i].vertex[1].x, line_segments[i].vertex[1].y);
	//}
	//glEnd();
	//glLineWidth(1);

	glFlush();
}
