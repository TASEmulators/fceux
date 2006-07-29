void GetFileBase(const char *f);
extern uint32 uppow2(uint32 n);

char* FCEU_GetPath(int type);
char *FCEU_MakePath(int type, const char* filebase);
char *FCEU_MakeFName(int type, int id1, char *cd1);

#define FCEUMKF_STATE	1
#define FCEUMKF_SNAP	2
#define FCEUMKF_SAV	3
#define FCEUMKF_CHEAT	4
#define FCEUMKF_FDSROM	5
#define FCEUMKF_PALETTE	6
#define FCEUMKF_GGROM	7
#define FCEUMKF_IPS	8
#define FCEUMKF_FDS	9
#define FCEUMKF_MOVIE	10
#define FCEUMKF_NPTEMP	11
#define FCEUMKF_MOVIEGLOB	12
#define FCEUMKF_STATEGLOB	13
#define FCEUMKF_MOVIEGLOB2	14
#define FCEUMKF_REWINDSTATE 15
