void FCEU_CheatResetRAM(void);
void FCEU_CheatAddRAM(int s, uint32 A, uint8 *p);

void FCEU_LoadGameCheats(FILE *override);
void FCEU_FlushGameCheats(FILE *override, int nosave);
void FCEU_ApplyPeriodicCheats(void);
void FCEU_PowerCheats(void);
int FCEU_CalcCheatAffectedBytes(uint32 address, uint32 size);


int FCEU_CheatGetByte(uint32 A);
void FCEU_CheatSetByte(uint32 A, uint8 V);

extern int savecheats;

int FCEU_DisableAllCheats();

typedef struct {
	uint16 addr;
	uint8 val;
	int compare;
	readfunc PrevRead;
} CHEATF_SUBFAST;

struct CHEATF {
	struct CHEATF *next;
	char *name;
	uint16 addr;
	uint8 val;
	int compare;	/* -1 for no compare. */
	int type;	/* 0 for replace, 1 for substitute(GG). */
	int status;
};


#define CalcAddressRangeCheatCount(count, address, size) \
	count = 0; \
	for (int i = 0; i < numsubcheats && count < size; ++i) \
		if (SubCheats[i].addr >= address && SubCheats[i].addr < address + size) \
			++count;
