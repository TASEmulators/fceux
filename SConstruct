import os

# XXX path separator fixed right now
opts = Options()
opts.Add('PSS_STYLE', 'Path separator style', 1)
opts.Add('LSB_FIRST', 'Least significant byte first?', 1)

env = Environment(options = opts,
                  CPPDEFINES={'PSS_STYLE' : '${PSS_STYLE}',
                              'LSB_FIRST' : '${LSB_FIRST}'})

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
if not conf.CheckLib('z'):
  print 'Did not find libz or z.lib, exiting!'
  Exit(1)
if conf.CheckFunc('asprintf'):
  conf.env.Append(CCFLAGS = " -DHAVE_ASPRINTF")

env = conf.Finish()

# parse SDL cflags/libs
env.ParseConfig('sdl-config --cflags --libs')

Export('env')

SConscript(['src/SConscript'])
