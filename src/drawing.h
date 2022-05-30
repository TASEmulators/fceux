#ifndef __EMSCRIPTEN__
void DrawTextLineBG(uint8 *dest);
#endif
void DrawMessage(bool beforeMovie);
void FCEU_DrawRecordingStatus(uint8* XBuf);
void FCEU_DrawNumberRow(uint8 *XBuf, int *nstatus, int cur);
void DrawTextTrans(uint8 *dest, const uint8 *textmsg, uint8 fgcolor);
void DrawTextTransWH(uint8 *dest, const uint8 *textmsg, uint8 fgcolor, int max_w, int max_h, int border);
