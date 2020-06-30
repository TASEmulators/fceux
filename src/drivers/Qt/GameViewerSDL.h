// GameViewerSDL.h
//

#pragma  once

#include <QWidget>
#include <QResizeEvent>
#include <SDL.h>

class gameViewSDL_t : public QWidget
{
    Q_OBJECT

	public:
		gameViewSDL_t(QWidget *parent = 0);
	   ~gameViewSDL_t(void);

		int  init(void);
		void reset(void);
		void cleanup(void);
		void update(void);

	protected:

	void resizeEvent(QResizeEvent *event);
	 int  view_width;
	 int  view_height;

	 int  rw;
	 int  rh;
	 int  sx;
	 int  sy;

	 bool vsyncEnabled;

 	SDL_Window   *sdlWindow;
	SDL_Renderer *sdlRenderer;
	SDL_Texture  *sdlTexture;

	private slots:
};

