// GameViewerSDL.h
//

#pragma  once

#include <QWidget>
#include <QPaintEvent>
#include <QResizeEvent>
#include <SDL.h>

class gameViewSDL_t : public QWidget
{
    Q_OBJECT

	public:
		gameViewSDL_t(QWidget *parent = 0);
	   ~gameViewSDL_t(void);

		int  init(double devPixRatioIn = 1.0);
		void reset(void);
		void cleanup(void);

	protected:

	void paintEvent(QPaintEvent *event);
	void resizeEvent(QResizeEvent *event);
	 int  view_width;
	 int  view_height;

	 double devPixRatio;
	 int  rw;
	 int  rh;
	 int  sx;
	 int  sy;
	 int  sdlRendW;
	 int  sdlRendH;

	 bool vsyncEnabled;

 	SDL_Window   *sdlWindow;
	SDL_Renderer *sdlRenderer;
	SDL_Texture  *sdlTexture;
	SDL_Rect      sdlViewport;

	private slots:
};

