// GameViewer.cpp
//
#include <math.h>

#include "GameViewer.h"

gameView_t::gameView_t(QWidget *parent)
	: QOpenGLWidget( parent )
{

}

gameView_t::~gameView_t(void)
{
	// Make sure the context is current and then explicitly
    // destroy all underlying OpenGL resources.
    makeCurrent();

	 // Free GL texture

	 doneCurrent();
}

void gameView_t::initializeGL(void)
{
	 initializeOpenGLFunctions();
    // Set up the rendering context, load shaders and other resources, etc.:
    //QOpenGLFunctions *gl = QOpenGLContext::currentContext()->functions();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	 printf("GL Init!\n");
}

void gameView_t::resizeGL(int w, int h)
{
	printf("GL Resize: %i x %i \n", w, h );
	glViewport(0, 0, w, h);

}

void gameView_t::paintGL(void)
{
	float x1, y1, x2, y2;

	angle += (2.0 * 3.14 * 0.01); 
	 
   glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	x1 = cos( angle );
	y1 = sin( angle );

	x2 = -x1;
	y2 = -y1;

	glColor4f( 1.0, 1.0, 1.0, 1.0 );
	glLineWidth(5.0);

	glBegin(GL_LINES);
	    glVertex2f( x1, y1 );
	    glVertex2f( x2, y2 );
	glEnd();

	 //printf("Paint GL!\n");
}
