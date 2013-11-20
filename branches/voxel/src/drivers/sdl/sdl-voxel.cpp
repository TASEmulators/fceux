/**********************************************************
 *             *** Emulation Voxel Engine ***             *
 *         Original concept and implementation by         *
 *                   Scott Jacobi, 2013                   *
 *                                                        *
 * The purpose of this code is to take the video output   *
 * buffer of an emulator, and render it in 3D using       *
 * voxels.  To provide a greater sense of depth, one      *
 * color is designated as the background color.  That     *
 * color is used to fill the screen each frame, and any   *
 * pixels in the buffer that match that color are         *
 * ignored.                                               *
 *                                                        *
 * Various methods may be used to determine what the      *
 * background color should be.  One way is to ask the     *
 * emulated system for its notion of a background clear   *
 * color, if the system supports that.  Another method is *
 * to poll the color of a particular coordinate on the    *
 * screen.                                                *
 *                                                        *
 * Additional features have been implemented, including   *
 * the ability to render the resulting three-dimensional  *
 * image with ripples or waves.  Various aspects of the   *
 * waves are under the control of the user, including the *
 * direction, wavelength, amplitude, and frequency.       *
 * Another feature is the ability to size and space the   *
 * voxels according to the users wishes.                  *
 *                                                        *
 * Control over the camera is provided through the        *
 * manipulation of the mouse.  The camera can be moved to *
 * various locations in order to customize the angle and  *
 * perspective from which the player views the screen.    *
 * The mouse wheel may be used to zoom in and out from    *
 * the scene.                                             *
 *                                                        *
 * This code is freely distributable, and may be used for *
 * any non-commercial purpose.  Please do not incorporate *
 * this code into any product that is intended for sale   *
 * or profit.  Collaborations to improve upon this code   *
 * are highly encouraged.  Please share your knowledge    *
 * and improvements with the emulation community.         *
 **********************************************************/

#define GL_GLEXT_LEGACY

#include "sdl.h"
#include "sdl-opengl.h"
#include "gui.h"
#include "math.h"
#include "../common/vidblit.h"
#include "../../utils/memory.h"

#ifdef APPLEOPENGL
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <OpenGL/glext.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>

#endif
#include <cstring>
#include <cstdlib>

#ifndef APIENTRY
#define APIENTRY
#endif

/* Ambient Light Values ( NEW ) */
GLfloat lmodel_ambient[] = { 0.3, 0.3, 0.3, 1.0 };
/* Diffuse Light Values ( NEW ) */
GLfloat LightDiffuse[]  = { 1.0f, 1.0f, 1.0f, 1.0f };
/* Light Position ( NEW ) */
GLfloat LightPosition[] = { 0.0f, 0.0f, 0.0f, 1.0f };

float wave[] = {0.0f, 3.0f, 6.0f, 8.0f, 9.0f, 9.5f, 10.0f, 9.5f, 9.0f, 8.0f, 6.0f, 3.0f,
		0.0f, -3.0f, -6.0f, -8.0f, -9.0f, -9.5f, -10.0f, -9.5f, -9.0f, -8.0f, -6.0f, -3.0f };
	
static int left,right,top,bottom; // right and bottom are not inclusive.
			
GLfloat vWidth = 0.5f;
GLfloat vHeight = 0.5f;
GLfloat vDepth = 0.5f;
GLfloat clearColor[4];

// Settings
double xScale, yScale, zScale, transparency, xSpace, ySpace;
int bgcMethod, sampleXcoord, sampleYcoord;
double waveLength, waveAmp, waveFreq;
int halfWidth, halfHeight;
float cameraZ;

extern void *HiBuffer;
extern Config *g_config;
extern uint8 PALRAM[0x20];

enum waveTypes 
{
	wtDisabled = 0,
	wtHorizontal,
	wtVertical,
	wtDiagonal1,
	wtDiagonal2
};

waveTypes waves;

GLuint  voxel;                                    // Storage For The Display List

GLvoid voxelList()
{
	GLfloat voxW = vWidth * (float)xScale;
	GLfloat voxH = vHeight * (float)yScale;
	GLfloat voxD = vDepth * (float)zScale;
        
	voxel=glGenLists(1);
	glNewList(voxel, GL_COMPILE);
		glBegin(GL_QUADS);                // Start Drawing Quads
		// Bottom Face
		glNormal3f( 0.0f, -1.0f,  0.0f);
		glVertex3f(-voxW, -voxH, -voxD);  // Top Right Of The Quad
		glVertex3f( voxW, -voxH, -voxD);  // Top Left Of The Quad
		glVertex3f( voxW, -voxH,  voxD);  // Bottom Left Of The Quad
		glVertex3f(-voxW, -voxH,  voxD);  // Bottom Right Of The Quad
		// Front Face
		glNormal3f( 0.0f,  0.0f,  1.0f);
		glVertex3f(-voxW, -voxH,  voxD);  // Bottom Left Of The Quad
		glVertex3f( voxW, -voxH,  voxD);  // Bottom Right Of The Quad
		glVertex3f( voxW,  voxH,  voxD);  // Top Right Of The Quad
		glVertex3f(-voxW,  voxH,  voxD);  // Top Left Of The Quad
		// Back Face
//		glNormal3f( 0.0f,  0.0f, -1.0f);
//		glVertex3f(-voxW, -voxH, -voxD);  // Bottom Right Of and Quad
//		glVertex3f(-voxW,  voxH, -voxD);  // Top Right Of The Quad
//		glVertex3f( voxW,  voxH, -voxD);  // Top Left Of The Quad
//		glVertex3f( voxW, -voxH, -voxD);  // Bottom Left Of The Quad
		// Right face
		glNormal3f( 1.0f,  0.0f,  0.0f);
		glVertex3f( voxW, -voxH, -voxD);  // Bottom Right Of The Quad
		glVertex3f( voxW,  voxH, -voxD);  // Top Right Of The Quad
		glVertex3f( voxW,  voxH,  voxD);  // Top Left Of The Quad
		glVertex3f( voxW, -voxH,  voxD);  // Bottom Left Of The Quad
		// Left Face
		glNormal3f(-1.0f,  0.0f,  0.0f);
		glVertex3f(-voxW, -voxH, -voxD);  // Bottom Left Of The Quad
		glVertex3f(-voxW, -voxH,  voxD);  // Bottom Right Of The Quad
		glVertex3f(-voxW,  voxH,  voxD);  // Top Right Of The Quad
		glVertex3f(-voxW,  voxH, -voxD);  // Top Left Of The Quad
		// Top Face
		glNormal3f( 0.0f,  1.0f,  0.0f);
		glVertex3f(-voxW,  voxH, -voxD);  // Top Left Of The Quad
		glVertex3f(-voxW,  voxH,  voxD);  // Bottom Left Of The Quad
		glVertex3f( voxW,  voxH,  voxD);  // Bottom Right Of The Quad
		glVertex3f( voxW,  voxH, -voxD);  // Top Right Of The Quad
		glEnd();                          // Done Drawing Quads
	glEndList( );
}

int initVoxelGL(int l,
		int r,
		int t,
		int b,
		SDL_Surface *screen, 
		int efx)
{
	left=l;
	right=r;
	top=t;
	bottom=b;

	HiBuffer=FCEU_malloc(4*256*256);
	memset(HiBuffer,0x00,4*256*256);
  #ifndef LSB_FIRST
	InitBlitToHigh(4,0xFF000000,0xFF0000,0xFF00,efx&2,0,0);
  #else
	InitBlitToHigh(4,0xFF,0xFF00,0xFF0000,efx&2,0,0);
  #endif

	g_config->getOption("SDL.VoxelXScale", &xScale);
	g_config->getOption("SDL.VoxelYScale", &yScale);
	g_config->getOption("SDL.VoxelZScale", &zScale);
	g_config->getOption("SDL.VoxelTransparency", &transparency);
	g_config->getOption("SDL.VoxelXSpace", &xSpace);
	g_config->getOption("SDL.VoxelYSpace", &ySpace);
	g_config->getOption("SDL.VoxelBGCMethod", &bgcMethod);
	g_config->getOption("SDL.VoxelXCoordBGC", &sampleXcoord);
	g_config->getOption("SDL.VoxelYCoordBGC", &sampleYcoord);
	int waveType;
	g_config->getOption("SDL.VoxelWaves", &waveType);
	waves = (waveTypes)waveType;
	g_config->getOption("SDL.VoxelWavelength", &waveLength);
	g_config->getOption("SDL.VoxelWaveAmplitude", &waveAmp);
	g_config->getOption("SDL.VoxelWaveFrequency", &waveFreq);
	
	cameraZ = 360.0f;

	/* Build our display lists */
	voxelList();

	/* Enable Texture Mapping ( NEW ) */
	glEnable( GL_TEXTURE_2D );

	/* Enable smooth shading */
	glShadeModel( GL_SMOOTH );

	/* Set the background black */
	glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );

	/* Depth buffer setup */
	glClearDepth( 1.0f );

	/* Enables Depth Testing */
	glEnable( GL_DEPTH_TEST );

	/* The Type Of Depth Test To Do */
	glDepthFunc( GL_LEQUAL );

	/* Alpha blending for transparency */
	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	/* Really Nice Perspective Calculations */
	glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST );

	/* Material settings */
	GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat mat_shininess[] = { 50.0 };

	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);

	/* Setup The Ambient Light */
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);

	/* Setup The Diffuse Light */
	glLightfv( GL_LIGHT1, GL_DIFFUSE, LightDiffuse );

	/* Position The Light */
	glLightfv( GL_LIGHT1, GL_POSITION, LightPosition );

	/* Enable Light One */
	glEnable( GL_LIGHTING );       /* Enable Lighting */
	glEnable( GL_LIGHT1 );
	glEnable( GL_COLOR_MATERIAL ); /* Enable Material Coloring */

	/* Height / width ration */
	GLfloat ratio;
	int width = screen->w;
	int height = screen->h;
	halfWidth = width/2;
	halfHeight = height/2;

	ratio = ( GLfloat )width / ( GLfloat )height;

	/* Setup our viewport. */
	glViewport( 0, 0, ( GLint )width, ( GLint )height );

	/* change to the projection matrix and set our viewing volume. */
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity( );

	/* Set our perspective */
	gluPerspective( 45.0f, ratio, 0.1f, 1000.0f );

	/* Make sure we're chaning the model view and not the projection */
	glMatrixMode( GL_MODELVIEW );

	/* Reset The View */
	glLoadIdentity( );

	return 1;
}

uint16 arrayColors[256], maxColorCount;
uint8 maxColorIx;

/* Here goes our drawing code */
void drawVoxelScene(uint8 *Xbuf)
{
	int paletteIx;

	Blit8ToHigh(Xbuf, (uint8*)HiBuffer, 256, 240, 256*4, 1, 1);

	if (bgcMethod == 1) // Color prominence
	{
		maxColorIx = 0;
		maxColorCount = arrayColors[0];
		for (paletteIx = 1; paletteIx < 256; paletteIx++)
		{
			if (arrayColors[paletteIx] > maxColorCount)
			{
				maxColorIx = paletteIx;
				maxColorCount = arrayColors[paletteIx];
			}
		}

		memset (arrayColors, 0, 256*sizeof(short));
	}
	
	/* These are to calculate our fps */
	static GLint T0     = 0;
	static GLint Frames = 0;
	static GLint totalFrames = 0;

	GLfloat Depth = cameraZ;
	GLfloat Zoom = cameraZ/4.0f;
	GLfloat voxelDepth;

	int length, freq, waveState;

	/* Select Our Texture */
//	glBindTexture( GL_TEXTURE_2D, texture[0] );

	uint8* buf = (uint8*)HiBuffer;
	GLfloat pixcol[4];

	uint8 r, g, b;
	switch (bgcMethod)
	{
	case 0: // Get the clear color from the system pallet.
		FCEUD_GetPalette(0x80 | PALRAM[0], &r, &g, &b); 

		pixcol[0] = (GLfloat)r / 256.0f;    
		pixcol[1] = (GLfloat)g / 256.0f;    
		pixcol[2] = (GLfloat)b / 256.0f;    
		break;
	
	case 1:	// Get the clear color from the previous frame's color count
		FCEUD_GetPalette(maxColorIx, &r, &g, &b);

		pixcol[0] = (GLfloat)r / 256.0f;    
		pixcol[1] = (GLfloat)g / 256.0f;    
		pixcol[2] = (GLfloat)b / 256.0f;    
		break;
		
	case 2: // Get the clear color from the sample coordinate.
		pixcol[0] = (GLfloat)(buf[sampleYcoord*256*4 + sampleXcoord*4    ]) / 256.0f;
		pixcol[1] = (GLfloat)(buf[sampleYcoord*256*4 + sampleXcoord*4 + 1]) / 256.0f;
		pixcol[2] = (GLfloat)(buf[sampleYcoord*256*4 + sampleXcoord*4 + 2]) / 256.0f;
	}

	// Voxel transparency
	pixcol[3] = transparency;

	// Check if the clear color has changed since the previous frame.
	if (!(clearColor[0] == pixcol[0] &&
		clearColor[1] == pixcol[1] &&
		clearColor[2] == pixcol[2] ))
	{
		clearColor[0] = pixcol[0];
		clearColor[1] = pixcol[1];
		clearColor[2] = pixcol[2];
		glClearColor(clearColor[0], clearColor[1], clearColor[2], 0.0f);
	}
	
	/* Clear The Screen And The Depth Buffer */
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	GLfloat cameraPos[3];

	cameraPos[0] = (float)(-halfWidth + GtkMouseData[0]);
	cameraPos[1] = (float)(halfHeight - GtkMouseData[1]);
	cameraPos[2] = Depth - 
		((fabs(cameraPos[0])/(float)halfWidth) * Zoom) -
		((fabs(cameraPos[1])/(float)halfHeight) * Zoom);

	/* Reset the view */
	glLoadIdentity( );
	gluLookAt(cameraPos[0], cameraPos[1], cameraPos[2],
			  0.0, 0.0, 0.0,
			  0.0, 1.0, 0.0);

	GLint xloop;      /* Loop For X Axis */
	GLint yloop;      /* Loop For Y Axis */
	
	// Calculate the positional space that each voxel needs to be relative to one another.
	float xPos = (float)xScale + (float)xSpace;
	float yPos = (float)yScale + (float)ySpace;

	for ( yloop = top; yloop < bottom; yloop++ )
	{
		for ( xloop = left; xloop < right; xloop++ )
		{
			if (bgcMethod == 1)
				arrayColors[Xbuf[yloop*256 + xloop]]++;
			
			pixcol[0] = (GLfloat)(buf[yloop*256*4 + xloop*4]    ) / 256.0f;
			pixcol[1] = (GLfloat)(buf[yloop*256*4 + xloop*4 + 1]) / 256.0f;
			pixcol[2] = (GLfloat)(buf[yloop*256*4 + xloop*4 + 2]) / 256.0f;

			if (!(pixcol[0] == clearColor[0] && pixcol[1] == clearColor[1] && pixcol[2] == clearColor[2]))
			{
				// Determine the length of the waves across the buffer.
				switch (waves)
				{
				case wtHorizontal:
					length = (int)((float)xloop / waveLength);
					break;
				case wtVertical:
					length = (int)((float)yloop / waveLength);
					break;
				case wtDiagonal1:
					length = (int)((float)(yloop - xloop) / waveLength);
					break;
				case wtDiagonal2:
					length = (int)((float)(yloop + xloop) / waveLength);
					break;
				}

				if (waves)
				{
					// Frequency of waves is driven by the number of frames.
					freq = (int)((float)totalFrames * waveFreq);
					waveState = (length + freq) % 24;
					if (waveState < 0)
						waveState += 24;
					// Amplitude is determined by multiplying the result.
					voxelDepth = wave[waveState] * waveAmp;
				}
				else
				{
				    voxelDepth = 0;
				}

				glPushMatrix();

				/* Position The Cubes On The Screen */
				glTranslatef( (-128.0 + ( float )xloop) * xPos,
					  (128.0 - ( float )yloop) * yPos,
					  voxelDepth ); 
					  
				glColor4fv( pixcol ); /* Select A Box Color */
				glCallList( voxel );  /* Draw the box */
				
				glPopMatrix();
			}
		}
	}

	/* Draw it to the screen */
	SDL_GL_SwapBuffers( );

	/* Gather our frames per second */
	Frames++;
	{
		GLint t = SDL_GetTicks();
		if (t - T0 >= 5000) 
		{
			GLfloat seconds = (t - T0) / 1000.0;
			GLfloat fps = Frames / seconds;
			printf("%d frames in %g seconds = %g FPS\n", Frames, seconds, fps);
			T0 = t;
			Frames = 0;
		}
	}
	totalFrames++;   
}


