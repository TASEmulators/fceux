// NetPlayMsgDef.h

#pragma once

#include <string.h>
#include <stdint.h>

// Network Byte Swap Functions
uint16_t netPlayByteSwap(uint16_t);
uint32_t netPlayByteSwap(uint32_t);
uint64_t netPlayByteSwap(uint64_t);

#pragma pack(push,4)

enum netPlayMsgType
{
	NETPLAY_AUTH_REQ,
	NETPLAY_AUTH_RESP,
	NETPLAY_LOAD_ROM_REQ,
	NETPLAY_SYNC_STATE_REQ,
	NETPLAY_SYNC_STATE_RESP,
	NETPLAY_RUN_FRAME_REQ,
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

	void toHostByteOrder()
	{
		magic[0] = netPlayByteSwap(magic[0]);
		magic[1] = netPlayByteSwap(magic[1]);
		msgId    = netPlayByteSwap(msgId);
		msgSize  = netPlayByteSwap(msgSize);
	};
	void toNetworkByteOrder()
	{
		magic[0] = netPlayByteSwap(magic[0]);
		magic[1] = netPlayByteSwap(magic[1]);
		msgId    = netPlayByteSwap(msgId);
		msgSize  = netPlayByteSwap(msgSize);
	}
};

struct netPlayAuthReq
{
	netPlayMsgHdr  hdr;

	netPlayAuthReq(void)
		: hdr(NETPLAY_AUTH_REQ, sizeof(netPlayAuthReq))
	{
	}

	void toHostByteOrder()
	{
		hdr.toHostByteOrder();
	}

	void toNetworkByteOrder()
	{
		hdr.toNetworkByteOrder();
	}
};

struct netPlayAuthResp
{
	netPlayMsgHdr  hdr;

	char playerId;
	char userName[64];
	char pswd[72];

	netPlayAuthResp(void)
		: hdr(NETPLAY_AUTH_RESP, sizeof(netPlayAuthResp)), playerId(NETPLAY_SPECTATOR)
	{
		memset(pswd, 0, sizeof(pswd));
	}

	void toHostByteOrder()
	{
		hdr.toHostByteOrder();
	}

	void toNetworkByteOrder()
	{
		hdr.toNetworkByteOrder();
	}
};

template <size_t N=8>
struct netPlayErrorMsg
{
	netPlayMsgHdr  hdr;

	unsigned short code;
	unsigned short flags;
	char data[N];

	static const uint32_t DISCONNECT_FLAG = 0x00000001;

	netPlayErrorMsg(void)
		: hdr(NETPLAY_ERROR_MSG, sizeof(netPlayErrorMsg)), code(0), flags(0)
	{
		memset(data, 0, N);
	}

	void setDisconnectFlag()
	{
		flags |= DISCONNECT_FLAG;
	}

	bool isDisconnectFlagSet()
	{
		return (flags & DISCONNECT_FLAG) ? true : false;
	}

	const char *getBuffer()
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

		hdr.msgSize = sizeof(netPlayErrorMsg) - N + strlen(data) + 1;

		return retval;
	}

	void toHostByteOrder()
	{
		hdr.toHostByteOrder();
		code  = netPlayByteSwap(code);
		flags = netPlayByteSwap(flags);
	}

	void toNetworkByteOrder()
	{
		hdr.toNetworkByteOrder();
		code  = netPlayByteSwap(code);
		flags = netPlayByteSwap(flags);
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

	void toHostByteOrder()
	{
		hdr.toHostByteOrder();
		fileSize  = netPlayByteSwap(fileSize);
	}

	void toNetworkByteOrder()
	{
		hdr.toNetworkByteOrder();
		fileSize  = netPlayByteSwap(fileSize);
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

	void toHostByteOrder()
	{
		hdr.toHostByteOrder();
		flags    = netPlayByteSwap(flags);
		frameNum = netPlayByteSwap(frameNum);
	}

	void toNetworkByteOrder()
	{
		hdr.toNetworkByteOrder();
		flags    = netPlayByteSwap(flags);
		frameNum = netPlayByteSwap(frameNum);
	}
};

struct netPlayClientState
{
	netPlayMsgHdr  hdr;

	uint32_t  flags;
	uint32_t  frameRdy; // What frame we have input ready for
	uint32_t  frameRun;
	uint32_t  opsFrame; // Last frame for ops data
	uint32_t  opsChkSum;
	uint32_t  ramChkSum;
	uint8_t   ctrlState[4];

	static constexpr uint32_t  PAUSE_FLAG = 0x0001;

	netPlayClientState(void)
		: hdr(NETPLAY_CLIENT_STATE, sizeof(netPlayClientState)), flags(0),
		frameRdy(0), frameRun(0), opsChkSum(0), ramChkSum(0)
	{
		memset( ctrlState, 0, sizeof(ctrlState) );
	}

	void toHostByteOrder()
	{
		hdr.toHostByteOrder();
		flags     = netPlayByteSwap(flags);
		frameRdy  = netPlayByteSwap(frameRdy);
		frameRun  = netPlayByteSwap(frameRun);
		opsFrame  = netPlayByteSwap(opsFrame);
		opsChkSum = netPlayByteSwap(opsChkSum);
		ramChkSum = netPlayByteSwap(ramChkSum);
	}

	void toNetworkByteOrder()
	{
		hdr.toNetworkByteOrder();
		flags     = netPlayByteSwap(flags);
		frameRdy  = netPlayByteSwap(frameRdy);
		frameRun  = netPlayByteSwap(frameRun);
		opsFrame  = netPlayByteSwap(opsFrame);
		opsChkSum = netPlayByteSwap(opsChkSum);
		ramChkSum = netPlayByteSwap(ramChkSum);
	}
};


#pragma pack(pop)
