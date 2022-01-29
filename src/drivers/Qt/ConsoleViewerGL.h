// GameViewerGL.h
//

#pragma  once

#include <stdint.h>

#include <QColor>
#include <QScreen>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>

class ConsoleViewGL_t : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

	public:
		ConsoleViewGL_t(QWidget *parent = 0);
		~ConsoleViewGL_t(void);

		int  init(void);
		void reset(void);

		void transfer2LocalBuffer(void);

		void setVsyncEnable( bool ena );
		void setLinearFilterEnable( bool ena );

		bool   getForceAspectOpt(void){ return forceAspect; };
		void   setForceAspectOpt( bool val ){ forceAspect = val; return; };
		bool   getAutoScaleOpt(void){ return autoScaleEna; };
		void   setAutoScaleOpt( bool val ){ autoScaleEna = val; return; };
		double getScaleX(void){ return xscale; };
		double getScaleY(void){ return yscale; };
		void   setScaleXY( double xs, double ys );
		void   getNormalizedCursorPos( double &x, double &y );
		bool   getMouseButtonState( unsigned int btn );
		void   setAspectXY( double x, double y );
		void   getAspectXY( double &x, double &y );
		double getAspectRatio(void);

		void   screenChanged(QScreen *scr);
		void   setBgColor( QColor &c );

	protected:
	void initializeGL(void);
	void resizeGL(int w, int h);
	void paintGL(void);
	void mousePressEvent(QMouseEvent * event);
	void mouseReleaseEvent(QMouseEvent * event);

	void buildTextures(void);
	void calcPixRemap(void);
	void doRemap(void);
	void chkExtnsGL(void);
	int  forcePwr2( int in );

	double devPixRatio;
	double aspectRatio;
	double aspectX;
	double aspectY;
	double xscale;
	double yscale;
	int  view_width;
	int  view_height;
	int  sx;
	int  sy;
	int  rw;
	int  rh;
	int  txtWidth;
	int  txtHeight;
	GLuint gltexture;
	bool   linearFilter;
	bool   forceAspect;
	bool   autoScaleEna;
	bool   reqPwr2;
	bool   vsyncEnabled;

	unsigned int  textureType;
	unsigned int  mouseButtonMask;
	QColor *bgColor;

	uint32_t  *localBuf;
	uint32_t   localBufSize;

	private slots:
		void cleanupGL(void);
		void renderFinished(void);
};

