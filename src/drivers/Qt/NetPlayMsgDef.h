// NetPlayMsgDef.h

#pragma once

#include <string.h>
#include <stdint.h>

#pragma pack(push,4)

enum netPlayMsgType
{
	NETPLAY_AUTH_REQ,
	NETPLAY_AUTH_RESP,
	NETPLAY_LOAD_ROM_REQ,
	NETPLAY_SYNC_STATE,
	NETPLAY_RUN_FRAME_REQ,
	NETPLAY_RUN_FRAME_RESP,
	NETPLAY_CLIENT_STATE,
	NETPLAY_ERROR_MSG,
};

enum netPlayerId
{
	NETPLAY_SPECTATOR = -1,
	NETPLAY_PLAYER1,
	NETPLAY_PLAYER2,
	NETPLAY_PLAYER3,
	NETPLAY_PLAYER4
};

static const uint32_t NETPLAY_MAGIC_NUMBER = 0xaa55aa55;

struct netPlayMsgHdr
{
	uint32_t  magic[2];
	uint32_t  msgId;
	uint32_t  msgSize;

	netPlayMsgHdr( uint32_t id, uint32_t size = sizeof(netPlayMsgHdr) )
	{
		magic[0] = NETPLAY_MAGIC_NUMBER;
		magic[1] = NETPLAY_MAGIC_NUMBER;
		msgId = id; msgSize = size;
	}
};

struct netPlayAuthReq
{
	netPlayMsgHdr  hdr;

	uint8_t  ctrlMask;

	netPlayAuthReq(void)
		: hdr(NETPLAY_AUTH_REQ, sizeof(netPlayAuthReq)), ctrlMask(0)
	{
	}
};

struct netPlayAuthResp
{
	netPlayMsgHdr  hdr;

	char playerId;
	char pswd[128];

	netPlayAuthResp(void)
		: hdr(NETPLAY_AUTH_RESP, sizeof(netPlayAuthResp)), playerId(NETPLAY_SPECTATOR)
	{
		memset(pswd, 0, sizeof(pswd));
	}
};

template <size_t N=8>
struct netPlayErrorMsg
{
	netPlayMsgHdr  hdr;

	char data[N];

	netPlayErrorMsg(void)
		: hdr(NETPLAY_ERROR_MSG, sizeof(netPlayErrorMsg))
	{
	}

	char *getMsgBuffer()
	{
		return &data[0];
	}

	int printf(const char* format, ...)
	{
		int retval;
		va_list args;
		va_start(args, format);
		retval = ::vsnprintf(data, sizeof(data), format, args);
		va_end(args);

		hdr.msgSize = sizeof(netPlayMsgHdr) + strlen(data);

		return retval;
	}
};

struct netPlayLoadRomReq
{
	netPlayMsgHdr  hdr;

	uint32_t  fileSize;
	char fileName[256];

	netPlayLoadRomReq(void)
		: hdr(NETPLAY_LOAD_ROM_REQ, sizeof(netPlayLoadRomReq)), fileSize(0)
	{
		memset(fileName, 0, sizeof(fileName));
	}
};


struct netPlayRunFrameReq
{
	netPlayMsgHdr  hdr;

	uint32_t  flags;
	uint32_t  frameNum;
	uint8_t   ctrlState[4];

	netPlayRunFrameReq(void)
		: hdr(NETPLAY_RUN_FRAME_REQ, sizeof(netPlayRunFrameReq)), flags(0), frameNum(0) 
	{
		memset( ctrlState, 0, sizeof(ctrlState) );
	}
};

struct netPlayRunFrameResp
{
	netPlayMsgHdr  hdr;

	uint32_t  flags;
	uint32_t  frameNum;
	uint32_t  frameRun;
	uint8_t   ctrlState[4];

	netPlayRunFrameResp(void)
		: hdr(NETPLAY_RUN_FRAME_RESP, sizeof(netPlayRunFrameResp)), flags(0), frameNum(0), frameRun(0)
	{
		memset( ctrlState, 0, sizeof(ctrlState) );
	}
};

struct netPlayClientState
{
	netPlayMsgHdr  hdr;

	uint32_t  flags;
	uint32_t  frameRdy;
	uint32_t  frameRun;
	uint8_t   ctrlState[4];

	netPlayClientState(void)
		: hdr(NETPLAY_CLIENT_STATE, sizeof(netPlayClientState)), flags(0), frameRdy(0), frameRun(0)
	{
		memset( ctrlState, 0, sizeof(ctrlState) );
	}
};


#pragma pack(pop)
