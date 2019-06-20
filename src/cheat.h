#ifndef CHEAT_H
#define CHEAT_H
void FCEU_CheatResetRAM(void);
void FCEU_CheatAddRAM(int s, uint32 A, uint8 *p);

void FCEU_LoadGameCheats(FILE *override, int override_existing = 1);
void FCEU_FlushGameCheats(FILE *override, int nosave);
void FCEU_SaveGameCheats(FILE *fp, int release = 0);
int FCEUI_GlobalToggleCheat(int global_enabled);
void FCEU_ApplyPeriodicCheats(void);
void FCEU_PowerCheats(void);
int FCEU_CalcCheatAffectedBytes(uint32 address, uint32 size);


int FCEU_CheatGetByte(uint32 A);
void FCEU_CheatSetByte(uint32 A, uint8 V);

extern int savecheats;
extern int globalCheatDisabled;
extern int disableAutoLSCheats;

int FCEU_DisableAllCheats();

typedef struct {
	uint16 addr;
	uint8 val;
	int compare;
	readfunc PrevRead;
} CHEATF_SUBFAST;

struct CHEATF {
	struct CHEATF *next;
	char *name = "";
	uint16 addr;
	uint8 val;
	int compare;	/* -1 for no compare. */
	int type;	/* 0 for replace, 1 for substitute(GG). */
	int status;
};

struct SEARCHPOSSIBLE {
	uint16 addr;
	uint8 previous;
	uint8 current;
	bool update;
};

#define FCEU_SEARCH_SPECIFIC_CHANGE         0
#define FCEU_SEARCH_RELATIVE_CHANGE         1
#define FCEU_SEARCH_PUERLY_RELATIVE_CHANGE  2
#define FCEU_SEARCH_ANY_CHANGE              3
#define FCEU_SEARCH_NEWVAL_KNOWN            4
#define FCEU_SEARCH_NEWVAL_GT               5
#define FCEU_SEARCH_NEWVAL_LT               6
#define FCEU_SEARCH_NEWVAL_GT_KNOWN         7
#define FCEU_SEARCH_NEWVAL_LT_KNOWN         8


#define CalcAddressRangeCheatCount(count, address, size) \
	count = 0; \
	for (int i = 0; i < numsubcheats && count < size; ++i) \
		if (SubCheats[i].addr >= address && SubCheats[i].addr < address + size) \
			++count
#endif