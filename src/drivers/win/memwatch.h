void UpdateMemWatch();
void CreateMemWatch();
void CloseMemoryWatch();
void AddMemWatch(char memaddress[32]);
extern char * MemWatchDir;
extern bool MemWatchLoadOnStart;
extern bool MemWatchLoadFileOnStart;
extern char *memw_recent_files[];
extern HWND memw_pwindow;
