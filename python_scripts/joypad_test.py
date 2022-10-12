import emu
import joypad

while True:
    print(f"PY: {joypad.read(1)}")
    emu.frameadvance()