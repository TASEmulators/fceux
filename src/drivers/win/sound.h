extern int32 fps_scale;

void RefreshThrottleFPS();
void ConfigSound();
int InitSound();
void TrashSound();
void win_SoundSetScale(int scale);
void win_SoundWriteData(int32 *buffer, int count);
void win_Throttle();