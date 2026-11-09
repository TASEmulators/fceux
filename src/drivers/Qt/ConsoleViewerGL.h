// GameViewerGL.h
//

#pragma  once

#include <stdint.h>

#include <QColor>
#include <QScreen>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>

#include "Qt/ConsoleViewerInterface.h"

class ConsoleViewGL_t : public QOpenGLWidget, protected QOpenGLFunctions, public ConsoleViewerBase
{
    Q_OBJECT

	public:
		ConsoleViewGL_t(QWidget *parent = 0);
		~ConsoleViewGL_t(void);

		int  init(void) override;
		void reset(void) override;
		void queueRedraw(void) override;
		int  driver(void) override { return VIDEO_DRIVER_OPENGL; };

		void transfer2LocalBuffer(void) override;
		QImage getFrameImage(void) override;

		void setVsyncEnable( bool ena ) override;
		void setLinearFilterEnable( bool ena ) override;

		bool   getForceAspectOpt(void) override { return forceAspect; };
		void   setForceAspectOpt( bool val ) override { forceAspect = val; return; };
		bool   getAutoScaleOpt(void) override { return autoScaleEna; };
		void   setAutoScaleOpt( bool val ) override { autoScaleEna = val; return; };
		double getScaleX(void) override { return xscale; };
		double getScaleY(void) override { return yscale; };
		void   setScaleXY( double xs, double ys ) override;
		void   getNormalizedCursorPos( double &x, double &y ) override;
		bool   getMouseButtonState( unsigned int btn ) override;
		void   setAspectXY( double x, double y ) override;
		void   getAspectXY( double &x, double &y ) override;
		double getAspectRatio(void) override;

		void   screenChanged(QScreen *scr);
		void   setBgColor( QColor &c ) override;
		void   setCursor(const QCursor &c) override { QOpenGLWidget::setCursor(c); };
		void   setCursor( Qt::CursorShape s ) override { QOpenGLWidget::setCursor(s); };

		QSize   size(void) override { return QOpenGLWidget::size(); };
		QCursor cursor(void) override { return QOpenGLWidget::cursor(); };
		void    setMinimumSize(const QSize &s) override { return QOpenGLWidget::setMinimumSize(s); };
		void    setMaximumSize(const QSize &s) override { return QOpenGLWidget::setMaximumSize(s); };

	protected:
	void initializeGL(void) override;
	void resizeGL(int w, int h) override;
	void paintGL(void) override;
	void mousePressEvent(QMouseEvent * event) override;
	void mouseReleaseEvent(QMouseEvent * event) override;

	void buildTextures(void);
	void drawTextOverlay(void);
	void destroyOverlayTexture(void);
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
	GLuint overlayTexture;
	int    overlayTexW;
	int    overlayTexH;
	bool   linearFilter;
	bool   forceAspect;
	bool   autoScaleEna;
	bool   reqPwr2;
	bool   vsyncEnabled;

	unsigned int  textureType;
	unsigned int  mouseButtonMask;
	QColor *bgColor;
	QTimer *drawTimer;

	uint32_t  *localBuf;
	uint32_t   localBufSize;

	private slots:
		void cleanupGL(void);
		void renderFinished(void);
		void onDrawSignal();
};
