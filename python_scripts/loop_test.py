import emu

while True:
    print(f"PY: Frame {emu.framecount()}")
    emu.frameadvance()
