import emu
import joypad

while True:
    # print(f"PY: {joypad.read(1)}")

    fc = emu.framecount()
    if (0 <= fc % 60 <= 10):
        print("PY: A press")
        joy_dict = {"A": True}
        joypad.write(1, joy_dict)

    emu.frameadvance()