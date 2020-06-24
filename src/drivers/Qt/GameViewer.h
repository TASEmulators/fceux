// GameViewer.h
//

#pragma  once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>

class gameView_t : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

	public:
		gameView_t(QWidget *parent = 0);
	   ~gameView_t(void);

		float angle;

	protected:
    void initializeGL(void);
	 void resizeGL(int w, int h);
	 void paintGL(void);

	 int  view_width;
	 int  view_height;
	 GLuint gltexture;

	private slots:
};

