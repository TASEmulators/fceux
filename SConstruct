import os
import sys

# XXX path separator fixed right now
opts = Options()
opts.AddOptions(
  BoolOption('DEBUG', 'Build with debugging information', 1),
  #BoolOption('PSS_STYLE', 'Path separator style', 1),
  BoolOption('LSB_FIRST', 'Least significant byte first?', None),
  BoolOption('FRAMESKIP', 'Enable frameskipping', 0),
  BoolOption('OPENGL',    'Enable OpenGL support (SDL only)', 1)
)

env = Environment(options = opts)

if os.environ.has_key('PLATFORM'):
  env.Replace(PLATFORM = os.environ['PLATFORM'])
if os.environ.has_key('CC'):
  env.Replace(CC = os.environ['CC'])
if os.environ.has_key('CXX'):
  env.Replace(CXX = os.environ['CXX'])
if os.environ.has_key('WINDRES'):
  env.Append(WINDRES = os.environ['WINDRES'])
if os.environ.has_key('CFLAGS'):
  env.Append(CCFLAGS = os.environ['CFLAGS'])
if os.environ.has_key('LDFLAGS'):
  env.Append(LINKFLAGS = os.environ['LDFLAGS'])

print "platform: ", env['PLATFORM']

# special flags for cygwin
# we have to do this here so that the function and lib checks will go through
# mingw
if env['PLATFORM'] == 'cygwin':
  env.Append(CCFLAGS = " -mno-cygwin")
  env.Append(LINKFLAGS = " -mno-cygwin")
  env['LIBS'] = ['wsock32'];

conf = Configure(env)
if env['PLATFORM'] == 'win32':
  conf.env.Append(CPPPATH = [".", "drivers/win/", "drivers/common/", "drivers/", "drivers/win/zlib", "drivers/win/directx"])
  conf.env.Append(CPPDEFINES = ["PSS_STYLE=2", "WIN32", "_USE_SHARED_MEMORY_", "NETWORK", "FCEUDEF_DEBUGGER", "NOMINMAX", "NEED_MINGW_HACKS", "_WIN32_IE=0x0600"])
  conf.env.Append(LIBS = ["rpcrt4", "comctl32", "vfw32", "winmm", "ws2_32", "comdlg32", "ole32", "gdi32"])
  if env.has_key('DEBUG'):
    if env['DEBUG']:
      conf.env.Append(CCFLAGS = " -g")
      conf.env.Append(CPPDEFINES = ["_DEBUG"])
      conf.env.Append(CCFLAGS = " -O3 -Wall")
    else: # Should we do this?
      conf.env.Append(CCFLAGS = " -O3 -fomit-frame-pointer")
else:
  if not conf.CheckLib('SDL'):
    print 'Did not find libSDL or SDL.lib, exiting!'
    Exit(1)
  if not conf.CheckLib('z', autoadd=1):
    print 'Did not find libz or z.lib, exiting!'
    Exit(1)
  if conf.CheckFunc('asprintf'):
    conf.env.Append(CCFLAGS = " -DHAVE_ASPRINTF")
  if env['OPENGL'] and conf.CheckLibWithHeader('GL', 'GL/gl.h', 'c++', autoadd=1):
    conf.env.Append(CCFLAGS = " -DOPENGL")
  # parse SDL cflags/libs
  env.ParseConfig('sdl-config --cflags --libs')
  conf.env.Append(CPPDEFINES = " PSS_STYLE=1")

env = conf.Finish()

# option specified
if env.has_key('LSB_FIRST'):
  if env['LSB_FIRST']:
    env.Append(CPPDEFINES = ['LSB_FIRST'])
# option not specified, check host system
elif sys.byteorder == 'little' or env['PLATFORM'] == 'win32':
  env.Append(CPPDEFINES = ['LSB_FIRST'])

if env['FRAMESKIP']:
  env.Append(CPPDEFINES = ['FRAMESKIP'])

print "CPPDEFINES:",env['CPPDEFINES']
print "CCFLAGS:",env['CCFLAGS']
#print "LIBS:",env['LIBS']
Export('env')

SConscript(['src/SConscript'])
