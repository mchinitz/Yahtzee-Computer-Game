//TODO cite http://www.programming-techniques.com/2012/05/font-rendering-in-glut-using-bitmap.html

/*This code is taken from the following source:
B. Subedi. (2012, May 11). Font rendering in GLUT using bitmap fonts with Sample example [Online]. Available: 
http://www.programming-techniques.com/2012/05/font-rendering-in-glut-using-bitmap.html.

*/


#include "Aux_GL.hpp"
#include <stdio.h>  
int w = 0, h = 0;

void setOrthographicProjection() {
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0, w, 0, h);
	glScalef(1, -1, 1);
	glTranslatef(0, -h, 0);
	glMatrixMode(GL_MODELVIEW);
}
void resetPerspectiveProjection() {
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
}
void renderBitmapString(float x, float y, void *font, const char *string){
	setOrthographicProjection();
	const char *c;
	glRasterPos2f(x, y);
	for (c = string; *c != '\0'; c++) {
		glutBitmapCharacter(font, *c);
	}
	resetPerspectiveProjection();
}
