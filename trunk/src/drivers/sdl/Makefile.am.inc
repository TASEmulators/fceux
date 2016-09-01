fceux_SOURCES += drivers/sdl/config.cpp drivers/sdl/input.cpp drivers/sdl/sdl-joystick.cpp drivers/sdl/sdl-sound.cpp drivers/sdl/sdl-throttle.cpp drivers/sdl/sdl-video.cpp drivers/sdl/sdl.cpp drivers/sdl/unix-netplay.cpp

if OPENGL
TMP_OGL = drivers/sdl/sdl-opengl.cpp
endif

if GTK2
TMP_GTK = drivers/sdl/gui.cpp
fceux_CPPFLAGS += @GTK_CFLAGS@
fceux_LDADD += @GTK_LIBS@
fceux_LDADD += -lX11
endif

if GTK3
TMP_GTK = drivers/sdl/gui.cpp
fceux_CPPFLAGS += @GTK_CFLAGS@
fceux_LDADD += @GTK_LIBS@
fceux_LDADD += -lX11
endif

fceux_SOURCES   +=  $(TMP_OGL)
fceux_SOURCES   +=  $(TMP_GTK)

# fceux_SOURCES += drivers/sdl/sdl-netplay.cpp  # Doesn't work!
