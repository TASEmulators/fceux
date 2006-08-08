import os
import sys

# XXX path separator fixed right now
opts = Options()
opts.AddOptions(
  BoolOption('PSS_STYLE', 'Path separator style', 1),
  BoolOption('LSB_FIRST', 'Least significant byte first?', None),
  BoolOption('FRAMESKIP', 'Enable frameskipping', 1),
  BoolOption('OPENGL',    'Enable OpenGL support', 1)
)

env = Environment(options = opts,
                  CPPDEFINES={'PSS_STYLE' : '${PSS_STYLE}'})

if os.environ.has_key('CC'):
  env.Replace(CC = os.environ['CC'])
if os.environ.has_key('CXX'):
  env.Replace(CXX = os.environ['CXX'])
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
  env['LIBS'] = ['ddraw','dinput','dsound','gdi32','dxguid','winmm','shell32','wsock32','comdlg32','ole32'];

conf = Configure(env)
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

env = conf.Finish()

# option specified
if env.has_key('LSB_FIRST'):
  if env['LSB_FIRST']:
    env.Append(CCFLAGS = ' -DLSB_FIRST')
# option not specified, check host system
elif sys.byteorder == 'little':
  env.Append(CCFLAGS = ' -DLSB_FIRST')

if env['FRAMESKIP']:
  env.Append(CCFLAGS = ' -DFRAMESKIP')

# parse SDL cflags/libs
env.ParseConfig('sdl-config --cflags --libs')

Export('env')

SConscript(['src/SConscript'])
