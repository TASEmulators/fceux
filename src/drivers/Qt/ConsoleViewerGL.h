// GameViewerGL.h
//

#pragma  once

#include <stdint.h>

#include <QOpenGLWidget>
#include <QOpenGLFunctions>

class ConsoleViewGL_t : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

	public:
		ConsoleViewGL_t(QWidget *parent = 0);
	   ~ConsoleViewGL_t(void);

		int  init( void );

		void transfer2LocalBuffer(void);

      void setLinearFilterEnable( bool ena );

		bool   getSqrPixelOpt(void){ return sqrPixels; };
		void   setSqrPixelOpt( bool val ){ sqrPixels = val; return; };
		bool   getAutoScaleOpt(void){ return autoScaleEna; };
		void   setAutoScaleOpt( bool val ){ autoScaleEna = val; return; };
		double getScaleX(void){ return xscale; };
		double getScaleY(void){ return yscale; };
		void   setScaleXY( double xs, double ys );
		void   getNormalizedCursorPos( double &x, double &y );
		bool   getMouseButtonState( unsigned int btn );

	protected:
   void initializeGL(void);
	void resizeGL(int w, int h);
	void paintGL(void);
	void mousePressEvent(QMouseEvent * event);
	void mouseReleaseEvent(QMouseEvent * event);

	void buildTextures(void);
	void calcPixRemap(void);
	void doRemap(void);

	double devPixRatio;
	double xscale;
	double yscale;
	int  view_width;
	int  view_height;
	int  sx;
	int  sy;
	int  rw;
	int  rh;
	GLuint gltexture;
	bool   linearFilter;
	bool   sqrPixels;
	bool   autoScaleEna;

	unsigned int  mouseButtonMask;

	uint32_t  *localBuf;
	uint32_t   localBufSize;

	private slots:
};

