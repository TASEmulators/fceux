fceud_SOURCES	=	drivers/pc/input.cpp drivers/pc/main.cpp drivers/pc/sdl.cpp drivers/pc/sdl-joystick.cpp drivers/pc/sdl-sound.cpp drivers/pc/sdl-throttle.cpp drivers/pc/sdl-video.cpp 
#fceud_SOURCES	+=	drivers/pc/unix-netplay.cpp

if OPENGL
TMP_OGL	= drivers/pc/sdl-opengl.cpp
endif

fceud_SOURCES	+=	$(TMP_OGL)
