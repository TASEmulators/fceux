my_list =  Split("""
input.cpp
config.cpp
sdl.cpp
sdl-joystick.cpp
sdl-sound.cpp
sdl-throttle.cpp
sdl-video.cpp
unix-netplay.cpp
""")

Import('env')
if 'GL' in env['LIBS']:
  my_list.append('sdl-opengl.cpp')

for x in range(len(my_list)):
  my_list[x] = 'drivers/sdl/' + my_list[x]
Return('my_list')
