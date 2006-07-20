extern int NTViewScanline;
extern int NTViewer;
extern int chrchanged;
extern int horzscroll, vertscroll;

void NTViewDoBlit(int autorefresh);
void UpdateNTView(int drawall);
void KillNTView();
void DoNTView();
