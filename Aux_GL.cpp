#include "Aux_GL.hpp"
#include <assert.h>
#include "Text.cpp"
#include <vector>
#include "TextInfo.cpp"
 
//returns the distance between two points
float dist(pair<float, float> p1, pair<float, float> p2)
{
	return sqrt(pow(p2.first - p1.first, 2) + pow(p2.second - p1.second, 2));
}

//Moves the current position of objects being drawn to the window coordinate (x,y,z)
float *translate(float x, float y, bool get_result_only = false, float z = 0)
{
	static float translate[3] = { 0 };
	glLoadIdentity();
	translate[0] = 2 * x / Dimensions.window_width - 1;
	translate[1] = -1 * (2 * y / Dimensions.window_height - 1);
	translate[2] = z;
	if (!get_result_only)
		glTranslatef(translate[0], translate[1], translate[2]);
	return translate;
}

//Creates a circle centered at (x,y) with a specified radius, with a specified color array, rotated theta degrees
void create_circle(float x, float y, float radius, float colors[3], int theta)
{
	static float larger_dim = max(Dimensions.window_width, Dimensions.window_height); //accounts for diff types of parameters for gluDisk, where start at (0,0) and +-1 denotes the edges 
	glColor3f(colors[0], colors[1], colors[2]);
	GLUquadricObj * obj = gluNewQuadric();
	translate(x, y, false);
	glRotatef(theta, 1, 1, 1);
	gluDisk(obj, 0, 2 * radius / larger_dim, 60, 1);
	glRotatef(-1 * theta, 1, 1, 1);
	gluDeleteQuadric(obj);
}

//Creates the index'th vertex on a rectangle [left, left + len] x [top, top + width]
void create_vertex_on_rectangle(int index, float left, float top, float length, float width)
{
	float x = left + ((index == 0 || index == 1) ? width : 0);
	float y = top + ((index == 0 || index == 3) ? length : 0);
	float *new_coords = translate(x, y);
	glVertex3f(new_coords[0], new_coords[1], 0);
}

//Creates a rectangle [left, left + len] x [top, top + width]
void create_rectangle(float left, float top, float length, float width, float colors[3])
{
	glBegin(GL_POLYGON);
	glColor3f(colors[0], colors[1], colors[2]);
	for (int i = 0; i < 4; i++)
	{
		create_vertex_on_rectangle(i, left, top, length, width);
		
	}
	glEnd();
}

//Creates a rectangle, and adds a white border
void create_rectangle_with_border(float left, float top, float length, float width, float colors[3])
{
	create_rectangle(left, top, length, width, colors); //the border will overwrite the "colors" if we create the interior first
	glLineWidth(2.0);
	vector<int> vertex_indices = { 0, 1, 1, 2, 2, 3, 3, 0 }; //lines are formed between adjacent vertices, hence the repetitions
	glBegin(GL_LINES);
	glColor3f(255, 255, 255); //white
	for (int vertex : vertex_indices)
	{
		create_vertex_on_rectangle(vertex, left, top, length, width);
	}
	glEnd();
}

//Creates either a GlutSolidCube or a GlutWiredCube centered at (x,y,0) with length side_len.
void create_cube(float x, float y, float side_len, float colors[3], bool is_solid)
{
	static float larger_dim = max(Dimensions.window_width, Dimensions.window_height); 
	//accounts for diff types of parameters for gluDisk, where start at (0,0) and +-1 denotes the edges 

	glPushMatrix(); //important because creating the cube occurs outside of glBegin
	translate(x, y);
	glColor3f(colors[0], colors[1], colors[2]);
	glRotatef(45, 1, 1, 1);
	if (is_solid)
		glutSolidCube(2 * side_len / larger_dim);
	else
		glutWireCube(2 * side_len / larger_dim);
	glRotatef(-45, 1, 1, 1); //undos the rotation
	glPopMatrix();

}

//Creates a cube and adds a black border
void create_cube_with_border(float x, float y, float side_len, float colors[3])
{
	create_cube(x, y, side_len, colors);
	float black[3] = { 0 };
	create_cube(x, y, side_len, black, false);

}

//displays given text line by line, starting at the specified coordinates, in the specified color.
void show_text(float starting_x, float starting_y, const char *text, float colors[3], int num_lines_before_sleep = 0, bool are_refreshing = true)
{
	glColor3f(colors[0], colors[1], colors[2]);
	vector<char *> lines = get_lines(text);
	
	float y = starting_y;

	static TextInfo textInfo = TextInfo(Dimensions.window_width);

	for (int line_number = 0; line_number < lines.size(); line_number ++)
	{
		char *line = lines[line_number];
		float *screen_coords = translate(starting_x, Dimensions.window_height - y, true);
		renderBitmapString(screen_coords[0], screen_coords[1], textInfo.Font, line);
		y += textInfo.gap;
		if (are_refreshing && line != lines[lines.size() - 1])
		{
			if (line_number >= num_lines_before_sleep)
				this_thread::sleep_for(chrono::milliseconds(300));
			glFlush(); //forces text to appear gradually!
		}
	}

	
	
}

void force_view_redisplay()
{
	glutPostRedisplay();
}
