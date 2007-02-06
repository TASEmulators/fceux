void FCEUD_SaveStateAs(void)
{
 const char filter[]="FCE Ultra Save State(*.fc?)\0*.fc?\0";
 char nameo[2048];
 OPENFILENAME ofn;

 memset(&ofn,0,sizeof(ofn));
 ofn.lStructSize=sizeof(ofn);
 ofn.hInstance=fceu_hInstance;
 ofn.lpstrTitle="Save State As...";
 ofn.lpstrFilter=filter;
 nameo[0]=0;
 ofn.lpstrFile=nameo;
 ofn.nMaxFile=256;
 ofn.Flags=OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY;
 if(GetSaveFileName(&ofn))
  FCEUI_SaveState(nameo);
}

void FCEUD_LoadStateFrom(void)
{
 const char filter[]="FCE Ultra Save State(*.fc?)\0*.fc?\0";
 char nameo[2048];
 OPENFILENAME ofn;

 memset(&ofn,0,sizeof(ofn));
 ofn.lStructSize=sizeof(ofn);
 ofn.hInstance=fceu_hInstance;
 ofn.lpstrTitle="Load State From...";
 ofn.lpstrFilter=filter;
 nameo[0]=0;
 ofn.lpstrFile=nameo;
 ofn.nMaxFile=256;
 ofn.Flags=OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY;
 if(GetOpenFileName(&ofn))
  FCEUI_LoadState(nameo);
}

