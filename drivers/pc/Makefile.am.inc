fceud_SOURCES	=	drivers/pc/input.c drivers/pc/main.c drivers/pc/sdl.c drivers/pc/sdl-joystick.c drivers/pc/sdl-sound.c drivers/pc/sdl-throttle.c drivers/pc/sdl-video.c 
#fceud_SOURCES	+=	drivers/pc/unix-netplay.c

if OPENGL
TMP_OGL	= drivers/pc/sdl-opengl.c
endif

fceud_SOURCES	+=	$(TMP_OGL)
