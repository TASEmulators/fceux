import os
import sys
import platform 

opts = Variables()
opts.AddVariables(
  BoolVariable('FRAMESKIP', 'Enable frameskipping', 1),
  BoolVariable('OPENGL',    'Enable OpenGL support', 1),
  BoolVariable('LSB_FIRST', 'Least signficant byte first (non-PPC)', 1),
  BoolVariable('DEBUG',     'Build with debugging symbols', 1),
  BoolVariable('LUA',       'Enable Lua support', 1),
  BoolVariable('NEWPPU',    'Enable new PPU core', 1),
  BoolVariable('CREATE_AVI', 'Enable avi creation support (SDL only)', 1),
  BoolVariable('LOGO', 'Enable a logoscreen when creating avis (SDL only)', '1'),
  BoolVariable('GTK', 'Enable GTK2 GUI (SDL only)', 1),
)

env = Environment(options = opts)

# LSB_FIRST must be off for PPC to compile
if platform.system == "ppc":
  env['LSB_FIRST'] = 0

# Default compiler flags:
env.Append(CCFLAGS = ['-Wall', '-Wno-write-strings', '-Wno-sign-compare', '-O2', '-Isrc/lua/src'])

if os.environ.has_key('PLATFORM'):
  env.Replace(PLATFORM = os.environ['PLATFORM'])
if os.environ.has_key('CC'):
  env.Replace(CC = os.environ['CC'])
if os.environ.has_key('CXX'):
  env.Replace(CXX = os.environ['CXX'])
if os.environ.has_key('WINDRES'):
  env.Replace(WINDRES = os.environ['WINDRES'])
if os.environ.has_key('CFLAGS'):
  env.Append(CCFLAGS = os.environ['CFLAGS'].split())
if os.environ.has_key('LDFLAGS'):
  env.Append(LINKFLAGS = os.environ['LDFLAGS'].split())

print "platform: ", env['PLATFORM']

# special flags for cygwin
# we have to do this here so that the function and lib checks will go through mingw
if env['PLATFORM'] == 'cygwin':
  env.Append(CCFLAGS = " -mno-cygwin")
  env.Append(LINKFLAGS = " -mno-cygwin")
  env['LIBS'] = ['wsock32'];

if env['PLATFORM'] == 'win32':
  env.Append(CPPPATH = [".", "drivers/win/", "drivers/common/", "drivers/", "drivers/win/zlib", "drivers/win/directx", "drivers/win/lua/include"])
  env.Append(CPPDEFINES = ["PSS_STYLE=2", "WIN32", "_USE_SHARED_MEMORY_", "NETWORK", "FCEUDEF_DEBUGGER", "NOMINMAX", "NEED_MINGW_HACKS", "_WIN32_IE=0x0600"])
  env.Append(LIBS = ["rpcrt4", "comctl32", "vfw32", "winmm", "ws2_32", "comdlg32", "ole32", "gdi32", "htmlhelp"])
else:
  conf = Configure(env)
  assert conf.CheckLibWithHeader('z', 'zlib.h', 'C', 'inflate;', 1), "please install: zlib"
  if not conf.CheckLib('SDL'):
    print 'Did not find libSDL or SDL.lib, exiting!'
    Exit(1)
  if env['GTK']:
    # Add compiler and linker flags from pkg-config
    env.ParseConfig('pkg-config --cflags --libs gtk+-2.0')
    env.Append(CPPDEFINES=["_GTK2"])
  if env['GTK']:
    env.Append(CCFLAGS = ["-D_GTK"])

  ### Lua platform defines
  ### Applies to all files even though only lua needs it, but should be ok
  if env['LUA']:
    env.Append(CPPDEFINES=["_S9XLUA_H"])
    if env['PLATFORM'] == 'darwin':
      # Define LUA_USE_MACOSX otherwise we can't bind external libs from lua
      env.Append(CCFLAGS = ["-DLUA_USE_MACOSX"])      
    if env['PLATFORM'] == 'posix':
      # If we're POSIX, we use LUA_USE_LINUX since that combines usual lua posix defines with dlfcn calls for dynamic library loading.
      # Should work on any *nix
      env.Append(CCFLAGS = ["-DLUA_USE_LINUX"])
  
  ### Search for gd if we're not in Windows
  if env['PLATFORM'] != 'win32' and env['PLATFORM'] != 'cygwin' and env['CREATE_AVI'] and env['LOGO']:
    gd = conf.CheckLib('gd', autoadd=1)
    if gd == 0:
      env['LOGO'] = 0
      print 'Did not find libgd, you won\'t be able to create a logo screen for your avis.'
   
  if conf.CheckFunc('asprintf'):
    conf.env.Append(CCFLAGS = "-DHAVE_ASPRINTF")
  if env['OPENGL'] and conf.CheckLibWithHeader('GL', 'GL/gl.h', 'c++', autoadd=1):
    conf.env.Append(CCFLAGS = "-DOPENGL")
  conf.env.Append(CPPDEFINES = ['PSS_STYLE=1'])
  # parse SDL cflags/libs
  env.ParseConfig('sdl-config --cflags --libs')
  
  env = conf.Finish()

if sys.byteorder == 'little' or env['PLATFORM'] == 'win32':
  env.Append(CPPDEFINES = ['LSB_FIRST'])

if env['FRAMESKIP']:
  env.Append(CPPDEFINES = ['FRAMESKIP'])

print "base CPPDEFINES:",env['CPPDEFINES']
print "base CCFLAGS:",env['CCFLAGS']

if env['DEBUG']:
  env.Append(CPPDEFINES=["_DEBUG"], CCFLAGS = ['-g'])

if env['PLATFORM'] != 'win32' and env['PLATFORM'] != 'cygwin' and env['CREATE_AVI']:
  env.Append(CPPDEFINES=["CREATE_AVI"])
else:
  env['CREATE_AVI']=0;

Export('env')
SConscript('src/SConscript')

# Install rules
exe_suffix = ''
if env['PLATFORM'] == 'win32':
  exe_suffix = '.exe'

fceux_src = 'src/fceux' + exe_suffix
fceux_dst = 'bin/fceux' + exe_suffix

auxlib_src = 'src/auxlib.lua'
auxlib_dst = 'bin/auxlib.lua'

fceux_h_src = 'src/drivers/win/help/fceux.chm'
fceux_h_dst = 'bin/fceux.chm'

env.Command(fceux_h_dst, fceux_h_src, [Copy(fceux_h_dst, fceux_h_src)])
env.Command(fceux_dst, fceux_src, [Copy(fceux_dst, fceux_src)])
env.Command(auxlib_dst, auxlib_src, [Copy(auxlib_dst, auxlib_src)])

# TODO: Fix this build script to gracefully install auxlib and the man page
#env.Alias(target="install", source=env.Install(dir="/usr/local/bin/", source=("bin/fceux", "bin/auxlib.lua")))
env.Alias(target="install", source=env.Install(dir="/usr/local/bin/", source="bin/fceux"))
