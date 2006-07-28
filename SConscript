file_list = Split("""
cart.cpp
cheat.cpp
crc32.cpp
config.cpp
debug.cpp
endian.cpp
fceu.cpp
fds.cpp
file.cpp 
filter.cpp 
general.cpp
ines.cpp
input.cpp
md5.cpp
memory.cpp
netplay.cpp
nsf.cpp
palette.cpp
ppu.cpp
sound.cpp
state.cpp
unif.cpp
video.cpp
vsuni.cpp
wave.cpp
x6502.cpp
movie.cpp
unzip.cpp""")

boards_file_list = Split("""
01-222.cpp
112.cpp
117.cpp
164.cpp
183.cpp
185.cpp
186.cpp
187.cpp
189.cpp
199.cpp
208.cpp
222.cpp
235.cpp
43.cpp
57.cpp
603-5052.cpp
8157.cpp
8237.cpp
88.cpp
90.cpp
95.cpp
a9711.cpp
addrlatch.cpp
bmc13in1jy110.cpp
bmc42in1r.cpp
bmc64in1nr.cpp
bmc70in1.cpp
bonza.cpp
datalatch.cpp
deirom.cpp
dream.cpp
__dummy_mapper.cpp
edu2000.cpp
fk23c.cpp
h2288.cpp
karaoke.cpp
kof97.cpp
konami-qtai.cpp
malee.cpp
mmc1.cpp
mmc3.cpp
mmc5.cpp
n106.cpp
n-c22m.cpp
novel.cpp
sachen.cpp
sheroes.cpp
sl1632.cpp
subor.cpp
super24.cpp
supervision.cpp
t-262.cpp
tengen.cpp
mapinc.h
mmc3.h
""")

for x in range(len(boards_file_list)):
  boards_file_list[x] = 'boards/' + boards_file_list[x]
  
fir_file_list = Split("""
c44100ntsc.h
c44100pal.h
c48000ntsc.h
c48000pal.h
c96000ntsc.h 
c96000pal.h
""")
for x in range(len(fir_file_list)):
  fir_file_list[x] = 'fir/' + fir_file_list[x]



input_file_list = Split("""
arkanoid.cpp
bworld.cpp
cursor.cpp
fkb.cpp
fkb.h
ftrainer.cpp
hypershot.cpp
mahjong.cpp
mouse.cpp
oekakids.cpp
powerpad.cpp
quiz.cpp
shadow.cpp
share.h
suborkb.cpp
suborkb.h
toprider.cpp
zapper.cpp
""")
for x in range(len(input_file_list)):
  input_file_list[x] = 'input/' + input_file_list[x]
  
mappers_file_list = Split("""
151.cpp
15.cpp
16.cpp
17.cpp
18.cpp
193.cpp
200.cpp
201.cpp
202.cpp
203.cpp
204.cpp
212.cpp
213.cpp
214.cpp
215.cpp
217.cpp
21.cpp
225.cpp
__226.cpp
227.cpp
228.cpp
229.cpp
22.cpp
230.cpp
231.cpp
232.cpp
234.cpp
23.cpp
240.cpp
241.cpp
242.cpp
244.cpp
246.cpp
24and26.cpp
255.cpp
25.cpp
27.cpp
32.cpp
33.cpp
40.cpp
41.cpp
42.cpp
43.cpp
46.cpp
50.cpp
51.cpp
59.cpp
60.cpp
61.cpp
62.cpp
65.cpp
67.cpp
68.cpp
69.cpp
6.cpp
71.cpp
72.cpp
73.cpp
75.cpp
76.cpp
77.cpp
79.cpp
80.cpp
82.cpp
83.cpp
85.cpp
86.cpp
89.cpp
8.cpp
91.cpp
92.cpp
97.cpp
99.cpp
emu2413.c
emu2413.h
emutypes.h
mapinc.h
mmc2and4.cpp
simple.cpp
vrc7tone.h
""")
for x in range(len(mappers_file_list)):
  mappers_file_list[x] = 'mappers/' + mappers_file_list[x]
  
palettes_file_list = Split("""
conv.c
palettes.h
rp2c04001.h 
rp2c04002.h 
rp2c04003.h
rp2c05004.h
""")
for x in range(len(palettes_file_list)):
  palettes_file_list[x] = 'palettes/' + palettes_file_list[x]
  
common_file_list = Split("""
args.cpp
args.h
cheat.cpp
cheat.h
config.cpp
config.h
hq2x.cpp
hq2x.h
hq3x.cpp
hq3x.h
scale2x.cpp
scale2x.h
scale3x.cpp
scale3x.h
scalebit.cpp
scalebit.h
vidblit.cpp
vidblit.h
""")

for x in range(len(common_file_list)):
  common_file_list[x] = 'drivers/common/' + common_file_list[x]
  
pc_file_list = Split("""
dface.h
input.cpp
input.h
keyscan.h
main.cpp
main.h
sdl.cpp
sdl.h
sdl-icon.h
sdl-joystick.cpp
sdl-netplay.cpp
sdl-netplay.h
sdl-opengl.cpp
sdl-opengl.h
sdl-sound.cpp
sdl-throttle.cpp
sdl-video.cpp
sdl-video.h
throttle.cpp
throttle.h
unix-netplay.cpp
unix-netplay.h
usage.h
""")

for x in range(len(pc_file_list)):
  pc_file_list[x] = 'drivers/pc/' + pc_file_list[x]

Export('file_list')

SConscript(Split("""
boards/SConscript
input/SConscript
fir/SConscript
mappers/SConscript
palettes/SConscript
drivers/common/SConscript
drivers/pc/SConscript
"""))
Import('file_list')
env = Environment()
#env.Program('fceu', boards_file_list + input_file_list + fir_file_list + mappers_file_list + palettes_file_list + common_file_list + pc_file_list + file_list )
env.Program('fceu', file_list)
