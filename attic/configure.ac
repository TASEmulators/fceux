AC_INIT([src/fceu.cpp])

AC_CANONICAL_HOST
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([fceu], $FCEU_VERSION)

AC_PROG_CC
AC_PROG_CPP
AM_PROG_CC_C_O
AC_PROG_CXX
AC_PROG_CXXCPP
AC_PROG_INSTALL

AC_DEFINE(_GNU_SOURCE)
AC_CHECK_FUNC(asprintf, AC_DEFINE([HAVE_ASPRINTF]), [])
AC_CHECK_FUNC(alloca, AC_DEFINE([HAVE_ALLOCA]), [])

dnl Check for zlib
AC_CHECK_LIB([z], [zlibVersion],[], AC_MSG_ERROR([*** zlib not found!]))
LIBS="$LIBS -lz"

AM_CONDITIONAL(WIN32, false)
AM_CONDITIONAL(UNIX, false)
AM_CONDITIONAL(USE_SEXYAL, false)
AM_CONDITIONAL(OPENGL, false)
AM_CONDITIONAL(HAVE_GTK, false)
AC_DEFINE([NETWORK],[1])

if expr x"$target" : 'x.*beos' > /dev/null; then
CFLAGS="-no-fpic $CFLAGS"
CPPFLAGS="-no-fpic $CPPFLAGS"
AC_DEFINE([PSS_STYLE],[1])
elif expr x"$target" : 'x.*mingw' > /dev/null; then
AC_DEFINE([PSS_STYLE],[2])
AC_DEFINE([WIN32])
AM_CONDITIONAL(WIN32, true)

else
AM_CONDITIONAL(UNIX, true)
AC_DEFINE([UNIX])
AC_DEFINE([PSS_STYLE],[1])
AC_DEFINE([NETWORK],[1])

dnl AC_CHECK_HEADER([sys/soundcard.h],
dnl        AM_CONDITIONAL(USE_SEXYAL, true)
dnl	AC_DEFINE([USE_SEXYAL]),[])
fi

dnl if expr x"$target" : 'x.*darwin' > /dev/null; then
dnl AC_DEFINE([MACOSX])
dnl fi

if test x$use_nativewin32 = xno; then
 dnl Check for SDL
 SDL_VERSION=1.2.0
 AM_PATH_SDL($SDL_VERSION, [:],
             AC_MSG_ERROR([*** SDL version $SDL_VERSION not found!]))

 AC_LANG_CPLUSPLUS
 LIBS="$LIBS $SDL_LIBS"
 CFLAGS="-Wall -fomit-frame-pointer $CFLAGS $SDL_CFLAGS"
 CPPFLAGS="-Wall -fomit-frame-pointer $CPPFLAGS $SDL_CFLAGS"

 dnl Check for SDL_net for net play
 HAS_SDL_NET=no
 dnl AC_CHECK_LIB([SDL_net], [SDLNet_Init],[HAS_SDL_NET=yes])
 dnl if test x$HAS_SDL_NET = xyes; then
 dnl	AC_DEFINE([NETWORK])
 dnl	LIBS="$LIBS -lSDL_net"
 dnl fi

 AC_ARG_WITH(opengl,
 	[AC_HELP_STRING([--with-opengl],
 	[use OpenGL])],
 	 use_opengl=$withval,
 	 use_opengl="no")
 
 if [[ "$use_opengl" = "yes" ]] ; then
 	dnl Check for OpenGL
 	
 	AC_CHECK_HEADER([GL/gl.h],[AC_DEFINE([OPENGL]) AM_CONDITIONAL(OPENGL, true)],
 	[
 		AC_CHECK_HEADER([OpenGL/gl.h],[AC_DEFINE([OPENGL]) AM_CONDITIONAL(OPENGL, true)],[])
 		AC_DEFINE([APPLEOPENGL])
 	])
 fi
 AC_DEFINE([SDL],[1])
fi


if expr match "$target_cpu" 'i.86' > /dev/null; then
	AC_DEFINE([C80x86])
fi

AC_C_BIGENDIAN([], [AC_DEFINE([LSB_FIRST])])


AC_DEFINE(FRAMESKIP)

dnl Output Makefiles
AC_OUTPUT([Makefile src/Makefile])
