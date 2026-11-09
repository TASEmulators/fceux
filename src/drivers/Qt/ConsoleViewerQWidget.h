// ConsoleViewerQWidget.h
//

#pragma  once

#include <QWidget>
#include <QColor>
#include <QCursor>
#include <QImage>
#include <QPaintEvent>
#include <QResizeEvent>

#include "Qt/ConsoleViewerInterface.h"

class ConsoleViewQWidget_t : public QWidget, public ConsoleViewerBase
{
    Q_OBJECT

	public:
		ConsoleViewQWidget_t(QWidget *parent = 0);
		~ConsoleViewQWidget_t(void);

		int  init(void) override;
		void reset(void) override;
		void cleanup(void);
		void queueRedraw(void) override;
		int  driver(void) override { return VIDEO_DRIVER_QPAINTER; };

		void transfer2LocalBuffer(void) override;
		QImage getFrameImage(void) override;

		void setVsyncEnable( bool ena ) override;
		void setLinearFilterEnable( bool ena ) override;

		bool   getForceAspectOpt(void) override { return forceAspect; };
		void   setForceAspectOpt( bool val ) override { forceAspect = val; fullRepaint = true; return; };
		bool   getAutoScaleOpt(void) override { return autoScaleEna; };
		void   setAutoScaleOpt( bool val ) override { autoScaleEna = val; fullRepaint = true; return; };
		double getScaleX(void) override { return xscale; };
		double getScaleY(void) override { return yscale; };
		void   setScaleXY( double xs, double ys ) override;
		void   getNormalizedCursorPos( double &x, double &y ) override;
		bool   getMouseButtonState( unsigned int btn ) override;
		void   setAspectXY( double x, double y ) override;
		void   getAspectXY( double &x, double &y ) override;
		double getAspectRatio(void) override;

		void   setCursor(const QCursor &c) override;
		void   setCursor( Qt::CursorShape s ) override;
		void   setBgColor( QColor &c ) override;

		QSize   size(void) override { return QWidget::size(); };
		QCursor cursor(void) override { return QWidget::cursor(); };
		void    setMinimumSize(const QSize &s) override { return QWidget::setMinimumSize(s); };
		void    setMaximumSize(const QSize &s) override { return QWidget::setMaximumSize(s); };

	protected:

	void paintEvent(QPaintEvent *event) override;
	void showEvent(QShowEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;
	void mousePressEvent(QMouseEvent * event) override;
	void mouseReleaseEvent(QMouseEvent * event) override;

	// Works out where the emulator image goes, sets sx, sy, rw and rh
	void calcViewport(int &nesWidth, int &nesHeight);

	int  view_width;
	int  view_height;

	double devPixRatio;
	double aspectRatio;
	double aspectX;
	double aspectY;
	double xscale;
	double yscale;
	int  rw;
	int  rh;
	int  sx;
	int  sy;
	int  sdlRendW;
	int  sdlRendH;

	bool vsyncEnabled;
	bool linearFilter;
	bool forceAspect;
	bool autoScaleEna;
	QColor *bgColor;

	uint32_t  *localBuf;
	uint32_t   localBufSize;

	// Frames normally only repaint the image rectangle. Anything that can change the area
	// outside of it needs the whole widget repainted.
	QRect     lastPaintRect;
	QColor    lastBgColor;
	uint64_t  lastOverlayRevision = 0;
	bool      fullRepaint = true;
	unsigned int mouseButtonMask;

	private slots:
};
