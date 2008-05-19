void UpdateMemWatch();
void CreateMemWatch();
void AddMemWatch(char memaddress[32]);
extern char * MemWatchDir;
extern bool MemWatchLoadOnStart;
extern bool MemWatchLoadFileOnStart;
extern char *memw_recent_files[];
extern char *memw_recent_directories[10];
extern HWND memw_pwindow;