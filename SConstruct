#fetch environment
env = Environment()

conf = Configure(env)
if not conf.CheckLib('SDL'):
  print 'Did not find libSDL.a or SDL.lib, exiting!'
  Exit(1)
if not conf.CheckLib('z'):
  print 'Did not find libz.a or z.lib, exiting!'
  Exit(1)
if not conf.CheckFunc('asprintf'):
  env['CCFLAGS'] += " -DHAVE_ASPRINTF"

env = conf.Finish()
Export('env')

SConscript(['src/SConscript'])
