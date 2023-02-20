#ifndef CART_H
#define CART_H

#include <vector>

struct CartInfo
{
	// Set by mapper/board code:
	void (*Power)(void);
	void (*Reset)(void);
	void (*Close)(void);

	struct SaveGame_t
	{
		uint8  *bufptr;	// Pointer to memory to save/load.
		uint32  buflen;	// How much memory to save/load.
		void (*resetFunc)(void); // Callback to reset save game memory

		SaveGame_t(void)
			: bufptr(nullptr), buflen(0), resetFunc(nullptr)
		{
		}
	};
	std::vector <SaveGame_t> SaveGame;

	void addSaveGameBuf( uint8* bufptrIn, uint32 buflenIn, void (*resetFuncIn)(void) = nullptr )
	{
		SaveGame_t tmp;

		tmp.bufptr = bufptrIn;
		tmp.buflen = buflenIn;
		tmp.resetFunc = resetFuncIn;

		SaveGame.push_back( tmp );
	}

	// Set by iNES/UNIF loading code.
	int mirror;		// As set in the header or chunk.
				// iNES/UNIF specific.  Intended
				// to help support games like "Karnov"
				// that are not really MMC3 but are
				// set to mapper 4.
	int mirrorAs2Bits;
	int battery;	// Presence of an actual battery.
	int ines2;
	int submapper;	// Submappers as defined by NES 2.0
	int wram_size;
	int battery_wram_size;
	int vram_size;
	int battery_vram_size;
	uint8 MD5[16];
	uint32 CRC32;	// Should be set by the iNES/UNIF loading
					// code, used by mapper/board code, maybe
					// other code in the future.

	CartInfo(void)
	{
		clear();
	}
	
	void clear(void)
	{
		Power = nullptr;
		Reset = nullptr;
		Close = nullptr;

		SaveGame.clear();

		mirror = 0;
		mirrorAs2Bits = 0;
		battery = 0;
		ines2 = 0;
		submapper = 0;
		wram_size = 0;
		battery_wram_size = 0;
		vram_size = 0;
		battery_vram_size = 0;
		memset( MD5, 0, sizeof(MD5));
		CRC32 = 0;
	};
};

extern CartInfo *currCartInfo;

void FCEU_SaveGameSave(CartInfo *LocalHWInfo);
void FCEU_LoadGameSave(CartInfo *LocalHWInfo);
void FCEU_ClearGameSave(CartInfo *LocalHWInfo);

extern uint8 *Page[32], *VPage[8], *MMC5SPRVPage[8], *MMC5BGVPage[8];

void ResetCartMapping(void);
void SetupCartPRGMapping(int chip, uint8 *p, uint32 size, int ram);
void SetupCartCHRMapping(int chip, uint8 *p, uint32 size, int ram);
void SetupCartMirroring(int m, int hard, uint8 *extra);

DECLFR(CartBROB);
DECLFR(CartBR);
DECLFW(CartBW);

extern uint8 PRGram[32];
extern uint8 CHRram[32];

extern uint8 *PRGptr[32];
extern uint8 *CHRptr[32];

extern uint32 PRGsize[32];
extern uint32 CHRsize[32];

extern uint32 PRGmask2[32];
extern uint32 PRGmask4[32];
extern uint32 PRGmask8[32];
extern uint32 PRGmask16[32];
extern uint32 PRGmask32[32];

extern uint32 CHRmask1[32];
extern uint32 CHRmask2[32];
extern uint32 CHRmask4[32];
extern uint32 CHRmask8[32];

void setprg2(uint32 A, uint32 V);
void setprg4(uint32 A, uint32 V);
void setprg8(uint32 A, uint32 V);
void setprg16(uint32 A, uint32 V);
void setprg32(uint32 A, uint32 V);

void setprg2r(int r, unsigned int A, unsigned int V);
void setprg4r(int r, unsigned int A, unsigned int V);
void setprg8r(int r, unsigned int A, unsigned int V);
void setprg16r(int r, unsigned int A, unsigned int V);
void setprg32r(int r, unsigned int A, unsigned int V);

void setchr1r(int r, unsigned int A, unsigned int V);
void setchr2r(int r, unsigned int A, unsigned int V);
void setchr4r(int r, unsigned int A, unsigned int V);
void setchr8r(int r, unsigned int V);

void setchr1(unsigned int A, unsigned int V);
void setchr2(unsigned int A, unsigned int V);
void setchr4(unsigned int A, unsigned int V);
void setchr8(unsigned int V);

void setmirror(int t);
void setmirrorw(int a, int b, int c, int d);
void setntamem(uint8 *p, int ram, uint32 b);

#define MI_H 0
#define MI_V 1
#define MI_0 2
#define MI_1 3

extern int geniestage;

void FCEU_GeniePower(void);

bool FCEU_OpenGenie(void);
void FCEU_CloseGenie(void);
void FCEU_KillGenie(void);

#endif
