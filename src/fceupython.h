#ifndef _FCEUPYTHON_H
#define _FCEUPYTHON_H

void FCEU_PythonFrameBoundary();
void FCEU_LoadPythonCode(const char* filename);

uint8 FCEU_PythonReadJoypad(int,uint8);

#endif //_FCEUPYTHON_H