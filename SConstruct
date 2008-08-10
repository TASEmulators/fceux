import os
import sys

# XXX path separator fixed right now
opts = Options()
opts.AddOptions(
  BoolOption('FRAMESKIP', 'Enable frameskipping', 1),
  BoolOption('OPENGL',    'Enable OpenGL support (SDL only)', 1)
)

env = Environment(options = opts)

# Default compiler flags:
env.Append(CCFLAGS = ['-Wall', '-Wno-write-strings', '-Wno-sign-compare'])

if os.environ.has_key('PLATFORM'):
  env.Replace(PLATFORM = os.environ['PLATFORM'])
if os.environ.has_key('CC'):
  env.Replace(CC = os.environ['CC'])
if os.environ.has_key('CXX'):
  env.Replace(CXX = os.environ['CXX'])
if os.environ.has_key('WINDRES'):
  env.Replace(WINDRES = os.environ['WINDRES'])
if os.environ.has_key('CFLAGS'):
  env.Append(CCFLAGS = os.environ['CFLAGS'])
if os.environ.has_key('LDFLAGS'):
  env.Append(LINKFLAGS = os.environ['LDFLAGS'])

print "platform: ", env['PLATFORM']

# special flags for cygwin
# we have to do this here so that the function and lib checks will go through mingw
if env['PLATFORM'] == 'cygwin':
  env.Append(CCFLAGS = " -mno-cygwin")
  env.Append(LINKFLAGS = " -mno-cygwin")
  env['LIBS'] = ['wsock32'];

if env['PLATFORM'] == 'win32':
  env.Append(CPPPATH = [".", "drivers/win/", "drivers/common/", "drivers/", "drivers/win/zlib", "drivers/win/directx"])
  env.Append(CPPDEFINES = ["PSS_STYLE=2", "WIN32", "_USE_SHARED_MEMORY_", "NETWORK", "FCEUDEF_DEBUGGER", "NOMINMAX", "NEED_MINGW_HACKS", "_WIN32_IE=0x0600"])
  env.Append(LIBS = ["rpcrt4", "comctl32", "vfw32", "winmm", "ws2_32", "comdlg32", "ole32", "gdi32", "htmlhelp"])
else:
  conf = Configure(env)
  if not conf.CheckLib('SDL'):
    print 'Did not find libSDL or SDL.lib, exiting!'
    Exit(1)
  if not conf.CheckLib('z', autoadd=1):
    print 'Did not find libz or z.lib, exiting!'
    Exit(1)
  if not conf.CheckLib('lua5.1', autoadd=1):
    print 'Did not find liblua5.1 or lua5.1.lib, exiting!'
    Exit(1)
    
  ### Search for zenity.
  path = os.getenv('PATH')
  directories = []
  dir = ''
  # check for '$' so last entry is processed
  for x in path + '$':
    if x != ':' and x != '$':
      dir += x
    else:
      directories.append(dir)
      dir = ''
  
  zenity = 0
  print "Checking for zenity..."
  for x in directories:
    if os.path.isfile(os.path.join(x, "zenity")):
      zenity = 1
  if zenity != 1:
    print "*** WARNING ***"
    print "Zenity could not be found in the PATH.  File dialogs will not work without zenity installed."
    raw_input('Press any key to continue. . .')
        
  if conf.CheckFunc('asprintf'):
    conf.env.Append(CCFLAGS = " -DHAVE_ASPRINTF")
  if env['OPENGL'] and conf.CheckLibWithHeader('GL', 'GL/gl.h', 'c++', autoadd=1):
    conf.env.Append(CCFLAGS = " -DOPENGL")
  conf.env.Append(CPPDEFINES = ['PSS_STYLE=1'])
  # parse SDL cflags/libs
  env.ParseConfig('sdl-config --cflags --libs')
  # parse liblua cflags/libs
  env.Append(CPPPATH = ['/usr/local/include/lua5.1', '/usr/include/lua5.1'])
  env.Append(LINKFLAGS = "-llua5.1")
  env = conf.Finish()

# Build for this system's endianness, if not overriden
#if env.has_key('LSB_FIRST'):
#  if env['LSB_FIRST']:
#    env.Append(CPPDEFINES = ['LSB_FIRST'])
if sys.byteorder == 'little' or env['PLATFORM'] == 'win32':
  env.Append(CPPDEFINES = ['LSB_FIRST'])

if env['FRAMESKIP']:
  env.Append(CPPDEFINES = ['FRAMESKIP'])

print "base CPPDEFINES:",env['CPPDEFINES']
print "base CCFLAGS:",env['CCFLAGS']

# Split into release and debug environments:
#release_env = env.Clone(CCFLAGS = ['-O3', '-fomit-frame-pointer'], CPPDEFINES=["NDEBUG"])
#debug_env = env.Clone(CCFLAGS = ['-O', '-g'], CPPDEFINES=["_DEBUG"])
# THAT FAILED! Compromise:
#env.Append(CCFLAGS = ['-O3', '-g'], CPPDEFINES = ["_DEBUG"])

#SConscript('src/SConscript', build_dir='release', exports={'env':release_env})
#SConscript('src/SConscript', build_dir='release', exports={'env':debug_env})
Export('env')
SConscript('src/SConscript')

# Install rules
exe_suffix = ''
if env['PLATFORM'] == 'win32':
  exe_suffix = '.exe'

#fceux_r_src = 'src/release/fceux' + exe_suffix
#fceux_r_dst = 'bin/fceuxREL' + exe_suffix
#fceux_d_src = 'src/debug/fceux' + exe_suffix
#fceux_d_dst = 'bin/fceuxDBG' + exe_suffix
fceux_src = 'src/fceux' + exe_suffix
fceux_dst = 'bin/fceux' + exe_suffix

auxlib_src = 'src/auxlib.lua'
auxlib_dst = 'bin/auxlib.lua'

fceux_h_src = 'src/drivers/win/help/fceux.chm'
fceux_h_dst = 'bin/fceux.chm'

env.Command(fceux_h_dst, fceux_h_src, [Copy(fceux_h_dst, fceux_h_src)])
env.Command(fceux_dst, fceux_src, [Copy(fceux_dst, fceux_src)])
env.Command(auxlib_dst, auxlib_src, [Copy(auxlib_dst, auxlib_src)])

env.Alias(target="install", source=env.Install(dir="/usr/local/bin/", source=("bin/fceux", "bin/auxlib.lua")))
