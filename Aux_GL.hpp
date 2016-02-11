#ifdef _WINDOWS_
#include <Windows.h>
#include <freeglut\include\GL\freeglut.h>
#endif

#ifdef WIN32
#include <Windows.h>
#include <freeglut\include\GL\freeglut.h>
#endif



#ifdef unix
#include <GL/freeglut.h>
#endif



#ifndef AUX_GL_HPP
#define AUX_GL_HPP

//Creates a circle centered at (x,y) with a specified radius, with a specified color array, rotated theta degrees
void create_circle(float x, float y, float radius, float colors[3], int theta = 0);

//Creates a rectangle [left, left + len] x [top, top + width]
void create_rectangle(float left, float top, float length, float width, float colors[3]);

////Creates a rectangle, and adds a white border
void create_rectangle_with_border(float left, float top, float length, float width, float colors[3]);

//Creates either a GlutSolidCube or a GlutWiredCube centered at (x,y,0) with length side_len.
void create_cube(float x, float y, float side_len, float colors[3], bool is_solid = true);

//Creates a cube and adds a black border
void create_cube_with_border(float x, float y, float side_len, float colors[3]);

//displays given text line by line, starting at the specified coordinates, in the specified color. The size of the text and the font are determined
//elsewhere by the size of the window
void show_text(float starting_x, float starting_y, const char *text, float colors[3], int num_lines_before_sleep, bool are_refreshing);


//Contains the width and height of the window. In this application, both dimensions are the same, the minimum of the actual window's length and height, although the window manager might expand the objects to full size.
struct dimensions
{
	float window_width;
	float window_height;
};

static struct dimensions Dimensions;

//Refreshes the view
void force_view_redisplay();

#endif
