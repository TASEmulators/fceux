AC_INIT([fceux], [2.2.3])
AC_CONFIG_SRCDIR([src/fceu.cpp])
AM_INIT_AUTOMAKE

AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_INSTALL
AC_CONFIG_MACRO_DIR([m4])

AC_CHECK_FUNC(asprintf, AC_DEFINE([HAVE_ASPRINTF]))

## This is almost sure to not work
AC_ARG_WITH(nativewin32,
	[AC_HELP_STRING([--with-nativewin32],
	[use nativewin32])],  
	use_nativewin32=$withval,
	use_nativewin32="no")

AM_CONDITIONAL(WIN32, false)
AM_CONDITIONAL(UNIX, false)
AM_CONDITIONAL(NATIVEWIN32,false)
AM_CONDITIONAL(OPENGL, false)
AM_CONDITIONAL(GTK2, false)
AM_CONDITIONAL(GTK3, false)
AM_CONDITIONAL(LUA, false)
AM_CONDITIONAL(LUA_BUILTIN, false)
AM_CONDITIONAL(SYSTEM_MINIZIP, false)
AM_CONDITIONAL(GD, false)
AM_CONDITIONAL(FRAMESKIP, true)

## Check for zlib
AC_CHECK_LIB([z], [zlibVersion],[], AC_MSG_ERROR([*** zlib not found!]))
LIBS="$LIBS -lz"
AC_CHECK_LIB([pthread], [pthread_create],[], AC_MSG_ERROR([*** pthread not found!]))
LIBS="$LIBS -lpthread"

## Platform specific setup
if expr x"$target" : 'x.*beos' > /dev/null; then
	CFLAGS="-no-fpic $CFLAGS"
	CPPFLAGS="-no-fpic $CPPFLAGS"
	AC_DEFINE([PSS_STYLE],[1])
elif expr x"$target" : 'x.*mingw' > /dev/null; then
	## Probably doesn't work
	AC_DEFINE([PSS_STYLE],[2])
	AC_DEFINE([WIN32])
	AM_CONDITIONAL(WIN32, true)

	if test x$use_nativewin32 = xyes; then
		LIBS="$LIBS -mwindows -lddraw -ldinput -ldsound -lgdi32 -ldxguid -lwinmm -lshell32 -lwsock32 -lcomdlg32 -lole32"
		AM_CONDITIONAL(NATIVEWIN32,true)
	else
		LIBS="$LIBS -ldsound -lwinmm"
	fi
elif expr x"$target" : 'x.*darwin' > /dev/null; then
	## Probably doesn't work
	AC_DEFINE([MACOSX])
else
	AM_CONDITIONAL(UNIX, true)
	AC_DEFINE([UNIX])
	AC_DEFINE([PSS_STYLE],[1])
fi
if test x$use_nativewin32 = xno; then
	## Check for SDL
	SDL_VERSION=1.2.0
	AM_PATH_SDL($SDL_VERSION, [:],
		AC_MSG_ERROR([*** SDL version $SDL_VERSION not found!]))
	AC_DEFINE([SDL])

	LIBS="$LIBS $SDL_LIBS"
	CFLAGS="-Wall -fomit-frame-pointer $CFLAGS $SDL_CFLAGS"
	CPPFLAGS="-Wall -fomit-frame-pointer $CPPFLAGS $SDL_CFLAGS"

	## Check for OpenGL
	AC_ARG_ENABLE([opengl],
		AS_HELP_STRING([--enable-opengl], [Enable OpenGL support]))
	AS_IF([test "x$enable_opengl" != "xno"], [
		AC_CHECK_HEADER([GL/gl.h],[AC_DEFINE([OPENGL]) AM_CONDITIONAL(OPENGL, true)],
		[
			AC_CHECK_HEADER([OpenGL/gl.h],[AC_DEFINE([OPENGL]) AM_CONDITIONAL(OPENGL, true)],[])
			AC_DEFINE([APPLEOPENGL])
		])
		LIBS="$LIBS -lGL -lGLU"
	 ])

	## Check for GTK2
	AC_ARG_ENABLE([gtk2],
		AS_HELP_STRING([--enable-gtk2], [Enable GTK2 GUI]))
	AS_IF([test "x$enable_gtk2" = "xyes"], [
		AM_PATH_GTK_2_0([2.24.0],AM_CONDITIONAL(GTK2, true),AC_MSG_ERROR([Gtk+ 2.24.0 or higher required.]))
		AC_DEFINE([_GTK])
		AC_DEFINE([GTK2])
	])

	## Check for GTK3
	AC_ARG_ENABLE([gtk3],
		AS_HELP_STRING([--enable-gtk3], [Enable GTK3 GUI]))
	AS_IF([test "x$enable_gtk2" = "xyes" -a "x$enable_gtk3" = "xyes"], [
		   AC_MSG_ERROR([GTK2 and GTK3 cannot be simulatenously enabled.])
	])
	AS_IF([test "x$enable_gtk3" = "xyes"], [
		AM_PATH_GTK_3_0([3.0.0],AM_CONDITIONAL(GTK3, true),AC_MSG_ERROR([Gtk+ 3.0.0 or higher required.]))
		AC_DEFINE([_GTK])
		AC_DEFINE([GTK3])
	])
fi

## Check for system lua
AC_ARG_ENABLE([lua],
	AS_HELP_STRING([--enable-lua], [Use lua libraries found on this system]))
AS_IF([test "x$enable_lua" = "xyes"], [
	   AC_SUBST(LUA, lua5.1)
	   AX_PROG_LUA([5.1],[5.2])
	   PKG_CHECK_MODULES([lua51], [lua51])
	   AX_LUA_LIBS([], AC_MSG_ERROR([Lua libs not found!]))
	   AC_DEFINE([_S9XLUA_H])
	   AM_CONDITIONAL(LUA, true)
	   LIBS="$LIBS $LUA_LIB"
])

## Check for lua builtin
AC_ARG_ENABLE([lua-builtin],
	AS_HELP_STRING([--enable-lua-builtin], [Use lua5.1 included with fceux]))
AS_IF([test "x$enable_lua-builtin" = "xyes"], [
	   #AX_LUA_HEADERS([], AC_MSG_ERROR([Lua 5.1 headers not found!]))
	   AC_DEFINE([_S9XLUA_H])
	   AM_CONDITIONAL(LUA_BUILTIN, true)
])

## Check for gd
AC_ARG_ENABLE([gd],
	AS_HELP_STRING([--enable-gd], [Use libgd for AVI creation]))
AS_IF([test "x$enable_gd" = "xyes"], [
	   #AX_LUA_HEADERS([], AC_MSG_ERROR([Lua 5.1 headers not found!]))
	   AX_CHECK_GD
	   LIBS="$LIBS $GD_LIBS"
	   AC_DEFINE([CREATE_AVI])
])

## Check for system minizip
AC_ARG_ENABLE([system-minizip],
	AS_HELP_STRING([--enable-system-minizip], [Use minizip from system instead of fceux distribution]))
AS_IF([test "x$enable_system_minizip" = "xyes"], [
	   PKG_CHECK_MODULES([minizip], [minizip])
	   AM_CONDITIONAL(SYSTEM_MINIZIP, true)
	   LIBS="$LIBS $minizip_LIBS"
])

AC_C_BIGENDIAN([], [AC_DEFINE([LSB_FIRST])])

## Check for frameskip disable
AC_ARG_ENABLE([frameskip],
			      AS_HELP_STRING([--disable-frameskip], [Disable frameskip feature]))

AS_IF([test "x$disable_frameskip" != "xno"], [
	AC_DEFINE([FRAMESKIP])
])

AC_OUTPUT([Makefile src/Makefile])
