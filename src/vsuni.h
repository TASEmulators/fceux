enum IOPTION {
	VS_OPTION_GUN = 0x1,
	VS_OPTION_SWAPDIRAB = 0x2,
	VS_OPTION_PREDIP = 0x10,
};

typedef struct {
	const char* name;
	uint64 md5partial;
	int mapper;
	int mirroring;
	EGIPPU ppu;
	int ioption;
	int predip;
	EGIVS type;
} VSUNIENTRY;

void FCEU_VSUniPower(void);
void FCEU_VSUniCheck(uint64 md5partial, int *, uint8 *);
void FCEU_VSUniDraw(uint8 *XBuf);

void FCEU_VSUniToggleDIP(int);  /* For movies and netplay */
void FCEU_VSUniCoin(uint8 slot);
void FCEU_VSUniService();
void FCEU_VSUniSwap(uint8 *j0, uint8 *j1);
