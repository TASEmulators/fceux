// GameViewerGL.h
//

#pragma  once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>

class gameViewGL_t : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

	public:
		gameViewGL_t(QWidget *parent = 0);
	   ~gameViewGL_t(void);

		int  init( void );

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

	private slots:
};

