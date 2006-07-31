# XXX path separator fixed right now
opts = Options()
opts.Add('PSS_STYLE', 'Path separator style', 1)
opts.Add('LSB_FIRST', 'Least significant byte first?', 1)

#Import('env')
env = Environment(options = opts,
                  CPPDEFINES={'PSS_STYLE' : '${PSS_STYLE}',
                              'LSB_FIRST' : '${LSB_FIRST}'})

print "platform: ", env['PLATFORM']

#special flags for cygwin
#we have to do this here so that the function and lib checks will go through mingw
if env['PLATFORM'] == 'cygwin':
  env['CCFLAGS'] += "-mno-cygwin";
  env['LINKFLAGS'] += "-mno-cygwin";
  env['LIBS'] = ['ddraw','dinput','dsound','gdi32','dxguid','winmm','shell32','wsock32','comdlg32','ole32'];

conf = Configure(env)
if not conf.CheckLib('SDL'):
  print 'Did not find libSDL.a or SDL.lib, exiting!'
  Exit(1)
if not conf.CheckLib('z'):
  print 'Did not find libz.a or z.lib, exiting!'
  Exit(1)
if conf.CheckFunc('asprintf'):
  conf.env['CCFLAGS'].append("-DHAVE_ASPRINTF");


env = conf.Finish()
Export('env')

SConscript(['src/SConscript'])
