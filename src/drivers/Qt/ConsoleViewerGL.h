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

	protected:
   void initializeGL(void);
	void resizeGL(int w, int h);
	void paintGL(void);

	void buildTextures(void);
	void calcPixRemap(void);
	void doRemap(void);

	double devPixRatio;
	int  view_width;
	int  view_height;
	GLuint gltexture;
	bool   linearFilter;

	uint32_t  *localBuf;
	uint32_t   localBufSize;

	private slots:
};

