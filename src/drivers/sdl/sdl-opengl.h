void SetOpenGLPalette(uint8 *data);
void BlitOpenGL(uint8 *buf);
void KillOpenGL(void);

int InitOpenGL(int l, int r, int t, int b, 
		double xscale, double yscale, 
		int efx, int ipolate,
		int stretchx, int stretchy,
#if SDL_VERSION_ATLEAST(2, 0, 0)
		SDL_Window *window,
#endif
	  	SDL_Surface *screen);

