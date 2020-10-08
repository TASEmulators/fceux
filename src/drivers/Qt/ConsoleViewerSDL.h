// GameViewerSDL.h
//

#pragma  once

#include <QWidget>
#include <QPaintEvent>
#include <QResizeEvent>
#include <SDL.h>

class ConsoleViewSDL_t : public QWidget
{
    Q_OBJECT

	public:
		ConsoleViewSDL_t(QWidget *parent = 0);
	   ~ConsoleViewSDL_t(void);

		int  init(void);
		void reset(void);
		void cleanup(void);
		void render(void);

		void transfer2LocalBuffer(void);

	protected:

	//void paintEvent(QPaintEvent *event);
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

	 uint32_t  *localBuf;
	uint32_t   localBufSize;

 	SDL_Window   *sdlWindow;
	SDL_Renderer *sdlRenderer;
	SDL_Texture  *sdlTexture;
	//SDL_Rect      sdlViewport;

	private slots:
};

