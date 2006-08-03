extern int   frameskip;
extern char *cpalette;
extern char *soundrecfn;
extern int   ntsccol, ntschue, ntsctint;
extern char *DrBaseDirectory;

int  InitConfig(int, char **);
void SaveConfig(void);
void SetDefaults(void);
