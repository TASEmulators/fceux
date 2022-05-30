int InitNetplay(void);
#ifndef __EMSCRIPTEN__
void NetplayUpdate(uint8 *joyp);
extern int FCEUnetplay;
#else
#define NetplayUpdate(...)
#define FCEUnetplay (0) // 0 = no netplay
#endif


#define FCEUNPCMD_RESET   0x01
#define FCEUNPCMD_POWER   0x02

#define FCEUNPCMD_VSUNICOIN     0x07
#define FCEUNPCMD_VSUNIDIP0	0x08
#define FCEUNPCMD_FDSINSERTx	0x10
#define FCEUNPCMD_FDSINSERT	0x18
//#define FCEUNPCMD_FDSEJECT	0x19
#define FCEUNPCMD_FDSSELECT	0x1A

#define FCEUNPCMD_LOADSTATE     0x80

#define FCEUNPCMD_SAVESTATE     0x81 /* Sent from server to client. */
#define FCEUNPCMD_LOADCHEATS	0x82
#define FCEUNPCMD_TEXT		0x90

#ifndef __EMSCRIPTEN__
int FCEUNET_SendCommand(uint8, uint32);
#else
#define FCEUNET_SendCommand(...) (0) // 0 = fail
#endif
int FCEUNET_SendFile(uint8 cmd, char *);
