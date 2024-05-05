/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Xodnizel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QTemporaryFile>

#include "../../fceu.h"
#include "../../cart.h"
#include "../../cheat.h"
#include "../../state.h"
#include "../../movie.h"
#include "../../debug.h"
#include "utils/crc32.h"
#include "utils/timeStamp.h"
#include "utils/StringUtils.h"
#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/input.h"
#include "Qt/NetPlay.h"
#include "Qt/fceuWrapper.h"
#include "Qt/ConsoleWindow.h"
#include "Qt/ConsoleUtilities.h"
#include "Qt/NetPlayMsgDef.h"

//-----------------------------------------------------------------------------
//--- NetPlay State Monitoring Metrics
//-----------------------------------------------------------------------------
static uint32_t opsCrc32 = 0;
static void *traceRegistrationHandle = nullptr;
static bool  serverRequestedStateLoad = false;

struct NetPlayFrameData
{
	uint32_t frameNum = 0;
	uint32_t opsCrc32 = 0;
	uint32_t ramCrc32 = 0;

	void reset()
	{
		frameNum = 0;
		opsCrc32 = 0;
		ramCrc32 = 0;
	}
};

struct NetPlayFrameDataHist_t
{
	static constexpr int numFrames = 10;

	void push( NetPlayFrameData& newData )
	{

		data[bufHead] = newData;

		bufHead = (bufHead + 1) % numFrames;
	}

	int getLast( NetPlayFrameData& out )
	{
		int i;
		const int head = bufHead;
		if (head == 0)
		{
			i = numFrames - 1;
		}
		else
		{
			i = head - 1;
		}
		out = data[i];

		return 0;
	}

	int find( uint32_t frame, NetPlayFrameData& out )
	{
		int i, retval = -1;
		const int head = bufHead;

		if (head == 0)
		{
			i = numFrames - 1;
		}
		else
		{
			i = head - 1;
		}

		while (i != head)
		{
			if (data[i].frameNum == frame)
			{
				out = data[i];
				retval = 0;
				break;
			}

			if (i == 0)
			{
				i = numFrames - 1;
			}
			else
			{
				i = i - 1;
			}
		}
		return retval;
	}


	void reset(void)
	{
		bufHead = 0;

		for (int i=0; i<numFrames; i++)
		{
			data[i].reset();
		}
	}

	NetPlayFrameData  data[numFrames];
	int bufHead = 0;
};
static NetPlayFrameDataHist_t  netPlayFrameData;
//-----------------------------------------------------------------------------
const char* NetPlayPlayerRoleToString(int role)
{
	const char* roleString = nullptr;

	switch (role)
	{
		default:
		case netPlayerId::NETPLAY_SPECTATOR:
			roleString = "Spectator";
			break;
		case netPlayerId::NETPLAY_PLAYER1:
			roleString = "Player 1";
			break;
		case netPlayerId::NETPLAY_PLAYER2:
			roleString = "Player 2";
			break;
		case netPlayerId::NETPLAY_PLAYER3:
			roleString = "Player 3";
			break;
		case netPlayerId::NETPLAY_PLAYER4:
			roleString = "Player 4";
			break;
	}
	return roleString;
}
//-----------------------------------------------------------------------------
//--- NetPlayServer
//-----------------------------------------------------------------------------
NetPlayServer *NetPlayServer::instance = nullptr;

NetPlayServer::NetPlayServer(QObject *parent)
	: QTcpServer(parent)
{
	instance = this;

	connect(this, SIGNAL(newConnection(void)), this, SLOT(newConnectionRdy(void)));

	connect(consoleWindow, SIGNAL(romLoaded(void)), this, SLOT(onRomLoad(void)));
	connect(consoleWindow, SIGNAL(romUnload(void)), this, SLOT(onRomUnload(void)));
	connect(consoleWindow, SIGNAL(stateLoaded(void)), this, SLOT(onStateLoad(void)));
	connect(consoleWindow, SIGNAL(nesResetOccurred(void)), this, SLOT(onNesReset(void)));
	connect(consoleWindow, SIGNAL(cheatsChanged(void)), this, SLOT(onCheatsChanged(void)));
	connect(consoleWindow, SIGNAL(pauseToggled(bool)), this, SLOT(onPauseToggled(bool)));

	FCEU_WRAPPER_LOCK();
	inputFrameCount = static_cast<uint32_t>(currFrameCounter);

	if (currCartInfo != nullptr)
	{
		romCrc32 = currCartInfo->CRC32;
	}
	FCEU_WRAPPER_UNLOCK();
}


NetPlayServer::~NetPlayServer(void)
{
	instance = nullptr;

	closeAllConnections();

	printf("NetPlayServer Destructor\n");
}

int NetPlayServer::Create(QObject *parent)
{
	if (NetPlayServer::GetInstance() == nullptr)
	{
		NetPlayServer *server = new NetPlayServer(parent);

		if (server == nullptr)
		{
			printf("Error Creating Netplay Server!!!\n");
		}
	}
	FCEU_WRAPPER_LOCK();
	if (traceRegistrationHandle == nullptr)
	{
		traceRegistrationHandle = FCEUI_TraceInstructionRegister( NetPlayTraceInstruction );
	}
	FCEU_WRAPPER_UNLOCK();

	if (consoleWindow != nullptr)
	{
		consoleWindow->onNetPlayChange();
	}
	return 0;
}

int NetPlayServer::Destroy()
{
	NetPlayServer* server = NetPlayServer::GetInstance();
	if (server != nullptr)
	{
		delete server;
		server = nullptr;
	}
	FCEU_WRAPPER_LOCK();
	if (traceRegistrationHandle != nullptr)
	{
		if ( !FCEUI_TraceInstructionUnregisterHandle( traceRegistrationHandle ) )
		{
			printf("Unregister Trace Callback Error\n");
		}
		traceRegistrationHandle = nullptr;
	}
	FCEU_WRAPPER_UNLOCK();

	if (consoleWindow != nullptr)
	{
		consoleWindow->onNetPlayChange();
	}
	return 0;
}

void NetPlayServer::newConnectionRdy(void)
{
	printf("New Connection Ready!\n");

	processPendingConnections();
}

void NetPlayServer::processPendingConnections(void)
{
	QTcpSocket *newSock;

	newSock = nextPendingConnection();

	while (newSock != nullptr)
	{
		NetPlayClient *client = new NetPlayClient(this);

		if (debugMode)
		{
			// File is owned by the server
			QTemporaryFile* debugFile = new QTemporaryFile( QDir::tempPath() + QString("/NetPlayServerConnection_XXXXXX.log"), this);
			debugFile->open();
			client->setDebugLog( debugFile );
			printf("Client Debug File: %s\n", debugFile->fileName().toLocal8Bit().constData());
		}
		client->setSocket(newSock);

		clientList.push_back(client);

		connect( newSock, SIGNAL(readyRead(void)), client, SLOT(serverReadyRead(void)) );

		newSock = nextPendingConnection();

		printf("Added Client: %p   %zu\n", client, clientList.size() );

		netPlayAuthReq msg;

		sendMsg( client, &msg, sizeof(msg), [&msg]{ msg.toNetworkByteOrder(); } );

		emit clientConnected();
	}
}

int NetPlayServer::closeAllConnections(void)
{
	std::list <NetPlayClient*>::iterator it;

	for (int i=0; i<4; i++)
	{
		clientPlayer[i] = nullptr;
	}

	for (it = clientList.begin(); it != clientList.end(); it++)
	{
		auto* client = *it;

		delete client;
	}
	clientList.clear();
	emit clientDisconnected();

	roleMask = 0;

	return 0;
}

//-----------------------------------------------------------------------------
int NetPlayServer::sendMsg( NetPlayClient *client, const void *msg, size_t msgSize, std::function<void(void)> netByteOrderConvertFunc)
{
	QTcpSocket* sock;

	sock = client->getSocket();

	netByteOrderConvertFunc();

	sock->write( static_cast<const char *>(msg), msgSize );

	return 0;
}
//-----------------------------------------------------------------------------
int NetPlayServer::sendRomLoadReq( NetPlayClient *client )
{
	constexpr size_t BufferSize = 32 * 1024;
	char buf[BufferSize];
	size_t bytesRead;
	long fileSize = 0;
	int userCancel = 0;
	int archiveIndex = -1;
	netPlayLoadRomReq msg;
	const char* romextensions[] = { "nes", "fds", "nsf", 0 };

	const char *filepath = nullptr;

	if ( GameInfo == nullptr)
	{
		return -1;
	}

	//printf("filename: '%s' \n", GameInfo->filename );
	//printf("archiveFilename: '%s' \n", GameInfo->archiveFilename );

	if (GameInfo->archiveFilename)
	{
		filepath = GameInfo->archiveFilename;
	}
	else
	{
		filepath = GameInfo->filename;
	}
	archiveIndex = GameInfo->archiveIndex;

	if (archiveIndex >= 0)
	{
		msg.archiveIndex = archiveIndex;
	}

	if (filepath == nullptr)
	{
		return -1;
	}
	printf("Prep ROM Load Request: %s \n", filepath );

	FCEUFILE* fp = FCEU_fopen( filepath, nullptr, "rb", 0, archiveIndex, romextensions, &userCancel);

	if (fp == nullptr)
	{
		return -1;
	}
	fileSize = fp->size;

	QFileInfo fileInfo(GameInfo->filename);

	msg.hdr.msgSize += fileSize;
	msg.fileSize     = fileSize;
	Strlcpy( msg.fileName, fileInfo.fileName().toLocal8Bit().constData(), sizeof(msg.fileName) );

	printf("Sending ROM Load Request: %s  %lu\n", filepath, fileSize );

	sendMsg( client, &msg, sizeof(netPlayLoadRomReq), [&msg]{ msg.toNetworkByteOrder(); } );

	while ( (bytesRead = FCEU_fread( buf, 1, sizeof(buf), fp )) > 0 )
	{
		sendMsg( client, buf, bytesRead );
	}
	client->flushData();

	FCEU_fclose(fp);

	return 0;
}
//-----------------------------------------------------------------------------
struct NetPlayServerCheatQuery
{
	int  numLoaded = 0;

	netPlayLoadStateResp::CheatData data[netPlayLoadStateResp::MaxCheats];
};

static int serverActiveCheatListCB(const char *name, uint32 a, uint8 v, int c, int s, int type, void *data)
{
	NetPlayServerCheatQuery* query = static_cast<NetPlayServerCheatQuery*>(data);

	const int i = query->numLoaded;

	if (i < netPlayLoadStateResp::MaxCheats)
	{
		auto& cheat = query->data[i];
		cheat.addr = a;
		cheat.val  = v;
		cheat.cmp  = c;
		cheat.type = type;
		cheat.stat = s;
		Strlcpy( cheat.name, name, sizeof(cheat.name));

		query->numLoaded++;
	}

	return 1;
}
//-----------------------------------------------------------------------------
int NetPlayServer::sendStateSyncReq( NetPlayClient *client )
{
	EMUFILE_MEMORY em;
	int numCtrlFrames = 0, numCheats = 0, compressionLevel = 1;
	static constexpr size_t maxBytesPerWrite = 32 * 1024;
	netPlayLoadStateResp resp;
	netPlayLoadStateResp::CtrlData ctrlData[netPlayLoadStateResp::MaxCtrlFrames];
	NetPlayServerCheatQuery cheatQuery;

	if ( GameInfo == nullptr )
	{
		return -1;
	}
	FCEUSS_SaveMS( &em, compressionLevel );

	resp.stateSize    = em.size();
	resp.opsCrc32     = opsCrc32;
	resp.romCrc32     = romCrc32;

	NetPlayFrameData lastFrameData;
	netPlayFrameData.getLast( lastFrameData );

	resp.lastFrame.num      = lastFrameData.frameNum;
	resp.lastFrame.opsCrc32 = lastFrameData.opsCrc32;
	resp.lastFrame.ramCrc32 = lastFrameData.ramCrc32;
	resp.numCtrlFrames = 0;

	{
		int i = 0;
		FCEU::autoScopedLock alock(inputMtx);
		for (auto& inputFrame : input)
		{
			if (i < netPlayLoadStateResp::MaxCtrlFrames)
			{
				ctrlData[i].frameNum = netPlayByteSwap(inputFrame.frameCounter);
				ctrlData[i].ctrlState[0] = inputFrame.ctrl[0];
				ctrlData[i].ctrlState[1] = inputFrame.ctrl[1];
				ctrlData[i].ctrlState[2] = inputFrame.ctrl[2];
				ctrlData[i].ctrlState[3] = inputFrame.ctrl[3];
				i++;
			}
		}
		resp.numCtrlFrames = numCtrlFrames = i;
	}

	FCEUI_ListCheats(::serverActiveCheatListCB, (void *)&cheatQuery);
	resp.numCheats = numCheats = cheatQuery.numLoaded;

	resp.calcTotalSize();

	printf("Sending ROM Sync Request: %zu\n", em.size());

	sendMsg( client, &resp, sizeof(netPlayLoadStateResp), [&resp]{ resp.toNetworkByteOrder(); } );

	if (numCtrlFrames > 0)
	{
		sendMsg( client, ctrlData, numCtrlFrames * sizeof(netPlayLoadStateResp::CtrlData) );
	}
	if (numCheats > 0)
	{
		sendMsg( client, &cheatQuery.data, numCheats * sizeof(netPlayLoadStateResp::CheatData) );
	}
	//sendMsg( client, em.buf(), em.size() );

	const unsigned char* bufPtr = em.buf();
	size_t dataSize = em.size();

	while (dataSize > 0)
	{
		size_t bytesToWrite = dataSize;

		if (bytesToWrite > maxBytesPerWrite)
		{
			bytesToWrite = maxBytesPerWrite;
		}
		sendMsg( client, bufPtr, bytesToWrite );

		bufPtr += bytesToWrite;
		dataSize -= bytesToWrite;
	}

	client->flushData();

	return 0;
}
//-----------------------------------------------------------------------------
int NetPlayServer::sendPause( NetPlayClient *client )
{
	netPlayMsgHdr msg(NETPLAY_CLIENT_PAUSE_REQ);

	sendMsg( client, &msg, sizeof(netPlayMsgHdr), [&msg]{ msg.toNetworkByteOrder(); } );

	return 0;
}
//-----------------------------------------------------------------------------
int NetPlayServer::sendUnpause( NetPlayClient *client )
{
	netPlayMsgHdr msg(NETPLAY_CLIENT_UNPAUSE_REQ);

	sendMsg( client, &msg, sizeof(netPlayMsgHdr), [&msg]{ msg.toNetworkByteOrder(); } );

	return 0;
}
//-----------------------------------------------------------------------------
int NetPlayServer::sendPauseAll()
{
	int ret = 0;
	for (auto& client : clientList )
	{
		ret |= sendPause( client );
	}
	return ret;
}
//-----------------------------------------------------------------------------
int NetPlayServer::sendUnpauseAll()
{
	int ret = 0;
	for (auto& client : clientList )
	{
		ret |= sendUnpause( client );
	}
	return ret;
}
//-----------------------------------------------------------------------------
void NetPlayServer::setRole(int _role)
{
	role = _role;

	if ( (role >= NETPLAY_PLAYER1) && (role <= NETPLAY_PLAYER4) )
	{
		roleMask |= (0x01 << role);
	}
}
//-----------------------------------------------------------------------------
bool NetPlayServer::claimRole(NetPlayClient* client, int _role)
{
	bool success = true; // Default to spectator

	if ( (_role >= NETPLAY_PLAYER1) && (_role <= NETPLAY_PLAYER4) )
	{
		int mask = (0x01 << _role);

		if (roleMask & mask)
		{
			// Already Taken
			success = false;
		}
		else
		{
			success = true;
			roleMask |= mask;
			clientPlayer[_role] = client;
			client->role = _role;
		}
	}
	else
	{
		client->role = NETPLAY_SPECTATOR;
	}
	return success;
}
//-----------------------------------------------------------------------------
void NetPlayServer::releaseRole(NetPlayClient* client)
{
	int role = client->role;

	if ( (role >= NETPLAY_PLAYER1) && (role <= NETPLAY_PLAYER4) )
	{
		int mask = (0x01 << role);

		roleMask = roleMask & ~mask;
	}
}
//-----------------------------------------------------------------------------
void NetPlayServer::onRomLoad()
{
	//printf("New ROM Loaded!\n");
	FCEU_WRAPPER_LOCK();

	if (currCartInfo != nullptr)
	{
		romCrc32 = currCartInfo->CRC32;
	}

	opsCrc32 = 0;
	netPlayFrameData.reset();

	inputClear();
	inputFrameCount = static_cast<uint32_t>(currFrameCounter);

	sendPauseAll();

	// New ROM has been loaded by server, signal clients to load and sync
	for (auto& client : clientList )
	{
		sendRomLoadReq( client );
		sendStateSyncReq( client );
	}
	FCEU_WRAPPER_UNLOCK();
}
//-----------------------------------------------------------------------------
void NetPlayServer::onRomUnload()
{
	//printf("ROM UnLoaded!\n");
	netPlayMsgHdr unloadMsg(NETPLAY_UNLOAD_ROM_REQ);

	romCrc32 = 0;

	unloadMsg.toNetworkByteOrder();

	sendPauseAll();

	// New ROM has been loaded by server, signal clients to load and sync
	for (auto& client : clientList )
	{
		sendMsg( client, &unloadMsg, sizeof(unloadMsg) );
	}
}
//-----------------------------------------------------------------------------
void NetPlayServer::onStateLoad()
{
	//printf("New State Loaded!\n");
	FCEU_WRAPPER_LOCK();

	opsCrc32 = 0;
	netPlayFrameData.reset();

	inputClear();
	inputFrameCount = static_cast<uint32_t>(currFrameCounter);

	sendPauseAll();

	// New State has been loaded by server, signal clients to load and sync
	for (auto& client : clientList )
	{
		sendRomLoadReq( client );
		sendStateSyncReq( client );
	}
	FCEU_WRAPPER_UNLOCK();
}
//-----------------------------------------------------------------------------
void NetPlayServer::onNesReset()
{
	//printf("NES Reset Event!\n");
	FCEU_WRAPPER_LOCK();

	opsCrc32 = 0;
	netPlayFrameData.reset();

	inputClear();
	inputFrameCount = static_cast<uint32_t>(currFrameCounter);

	sendPauseAll();

	// NES Reset has occurred on server, signal clients sync
	for (auto& client : clientList )
	{
		sendRomLoadReq( client );
		sendStateSyncReq( client );
	}
	FCEU_WRAPPER_UNLOCK();
}
//-----------------------------------------------------------------------------
void NetPlayServer::onCheatsChanged()
{
	//printf("NES Cheats Event!\n");
	if (romCrc32 == 0)
	{
		return;
	}
	FCEU_WRAPPER_LOCK();

	opsCrc32 = 0;
	netPlayFrameData.reset();

	inputClear();
	inputFrameCount = static_cast<uint32_t>(currFrameCounter);

	sendPauseAll();

	// NES Reset has occurred on server, signal clients sync
	for (auto& client : clientList )
	{
		//sendRomLoadReq( client );
		sendStateSyncReq( client );
	}
	FCEU_WRAPPER_UNLOCK();
}
//-----------------------------------------------------------------------------
void NetPlayServer::onPauseToggled( bool isPaused )
{
	if (isPaused)
	{
		sendPauseAll();
	}
	else
	{
		sendUnpauseAll();
	}
}
//-----------------------------------------------------------------------------
void NetPlayServer::resyncClient( NetPlayClient *client )
{
	FCEU_WRAPPER_LOCK();
	sendRomLoadReq( client );
	sendStateSyncReq( client );
	FCEU_WRAPPER_UNLOCK();
}
//-----------------------------------------------------------------------------
void NetPlayServer::resyncAllClients()
{
	FCEU_WRAPPER_LOCK();

	opsCrc32 = 0;
	netPlayFrameData.reset();

	inputClear();
	inputFrameCount = static_cast<uint32_t>(currFrameCounter);

	for (auto& client : clientList )
	{
		resyncClient( client );
	}
	FCEU_WRAPPER_UNLOCK();
}
//-----------------------------------------------------------------------------
static void serverMessageCallback( void *userData, void *msgBuf, size_t msgSize )
{
	NetPlayServer *server = NetPlayServer::GetInstance();

	server->serverProcessMessage( static_cast<NetPlayClient*>(userData), msgBuf, msgSize );
}
//-----------------------------------------------------------------------------
void NetPlayServer::serverProcessMessage( NetPlayClient *client, void *msgBuf, size_t msgSize )
{
	netPlayMsgHdr *hdr = (netPlayMsgHdr*)msgBuf;

	const uint32_t msgId = netPlayByteSwap(hdr->msgId);

	//printf("Server Received Message: ID: %i   Bytes:%zu\n", msgId, msgSize );

	switch (msgId)
	{
		case NETPLAY_AUTH_RESP:
		{
			bool authentication_passed = false;
			netPlayAuthResp *msg = static_cast<netPlayAuthResp*>(msgBuf);
			msg->toHostByteOrder();
			printf("Authorize: Player: %i   Passwd: %s\n", msg->playerId, msg->pswd);

			bool version_chk_ok = true;

			if (enforceAppVersionCheck)
			{
				version_chk_ok = (msg->appVersionMajor == FCEU_VERSION_MAJOR) &&
			        	         (msg->appVersionMinor == FCEU_VERSION_MINOR) &&
			                	 (msg->appVersionPatch == FCEU_VERSION_PATCH);
			}

			if (!version_chk_ok)
			{
				netPlayTextMsg<128>  errorMsg(NETPLAY_ERROR_MSG);
				errorMsg.setFlag(netPlayTextMsgFlags::Disconnect);
				errorMsg.setFlag(netPlayTextMsgFlags::Error);
				errorMsg.printf("Client/Host Version Mismatch:\nHost version is %i.%i.%i\nClient version is %i.%i.%i", 
						FCEU_VERSION_MAJOR, FCEU_VERSION_MINOR, FCEU_VERSION_PATCH,
						msg->appVersionMajor, msg->appVersionMinor, msg->appVersionPatch);
				sendMsg( client, &errorMsg, errorMsg.hdr.msgSize, [&errorMsg]{ errorMsg.toNetworkByteOrder(); } );
				client->flushData();
			}
			else if (sessionPasswd.isEmpty())
			{
				authentication_passed = true;
			}
			else
			{
				authentication_passed = sessionPasswd.compare(msg->pswd, Qt::CaseSensitive) == 0;

				if (!authentication_passed)
				{
					netPlayTextMsg<128>  errorMsg(NETPLAY_ERROR_MSG);
					errorMsg.setFlag(netPlayTextMsgFlags::Disconnect);
					errorMsg.setFlag(netPlayTextMsgFlags::Error);
					errorMsg.printf("Invalid Password");
					sendMsg( client, &errorMsg, errorMsg.hdr.msgSize, [&errorMsg]{ errorMsg.toNetworkByteOrder(); } );
					client->flushData();
				}
			}

			if (authentication_passed)
			{
				if ( claimRole(client, msg->playerId) )
				{
					client->userName = msg->userName;
					FCEU_WRAPPER_LOCK();
					resyncClient(client);
					client->state = 1;
					if (FCEUI_EmulationPaused())
					{
						sendPauseAll();
					}
					else
					{
						sendUnpauseAll();
					}
					FCEU_WRAPPER_UNLOCK();
					FCEU_DispMessage("%s Joined",0, client->userName.toLocal8Bit().constData());
				}
				else
				{
					netPlayTextMsg<128>  errorMsg(NETPLAY_ERROR_MSG);
					errorMsg.setFlag(netPlayTextMsgFlags::Disconnect);
					errorMsg.setFlag(netPlayTextMsgFlags::Error);
					errorMsg.printf("Player %i role is not available", msg->playerId+1);
					sendMsg( client, &errorMsg, errorMsg.hdr.msgSize, [&errorMsg]{ errorMsg.toNetworkByteOrder(); } );
					client->flushData();
				}
			}
			else
			{
				// If authentication failed, force disconnect client.
				client->forceDisconnect();
			}
		}
		break;
		case NETPLAY_CLIENT_STATE:
		{
			netPlayClientState *msg = static_cast<netPlayClientState*>(msgBuf);
			msg->toHostByteOrder();

			client->currentFrame = msg->frameRun;
			client->readyFrame   = msg->frameRdy;
			client->gpData[0] = msg->ctrlState[0];
			client->gpData[1] = msg->ctrlState[1];
			client->gpData[2] = msg->ctrlState[2];
			client->gpData[3] = msg->ctrlState[3];

			client->setPaused( (msg->flags & netPlayClientState::PauseFlag ) ? true : false );
			//client->setDesync( (msg->flags & netPlayClientState::DesyncFlag) ? true : false );

			client->romMatch = (romCrc32 == msg->romCrc32);

			NetPlayFrameData data;
			if ( (msg->opsFrame == 0) || netPlayFrameData.find( msg->opsFrame, data ) )
			{
				//printf("Error: Server Could not find data for frame: %u\n", msg->opsFrame );
			}
			else
			{
				bool opsSync   = (data.opsCrc32 == msg->opsChkSum);
				bool ramSync   = (data.ramCrc32 == msg->ramChkSum);

				client->syncOk = client->romMatch && opsSync && ramSync;

				if (!client->syncOk)
				{
					printf("Client %s Frame:%u  is NOT in Sync: OPS:%i  RAM:%i\n",
							client->userName.toLocal8Bit().constData(), msg->frameRun, opsSync, ramSync);

					if (0 == client->desyncCount)
					{
						client->desyncSinceReset++;
						client->totalDesyncCount++;
					}
					client->desyncCount++;

					client->setDesync(true);

					if (client->desyncCount > forceResyncCount)
					{
						FCEU_WRAPPER_LOCK();
						resyncClient( client );
						FCEU_WRAPPER_UNLOCK();

						client->desyncCount = 0;
					}
				}
				else
				{
					if (client->desyncCount > 0)
					{
						client->desyncCount--;
					}
					else
					{
						client->setDesync(false);
						client->desyncCount = 0;
					}
				}
			}
		}
		break;
		case NETPLAY_PING_RESP:
		{

			netPlayPingResp *ping = static_cast<netPlayPingResp*>(msgBuf);
			ping->toHostByteOrder();

			FCEU::timeStampRecord ts;
			ts.readNew();

			uint64_t diff = ts.toMilliSeconds() - ping->hostTimeStamp;

			client->recordPingResult( diff );

			//printf("Ping Latency ms: %llu    Avg:%f\n", static_cast<unsigned long long>(diff), client->getAvgPingDelay());
		}
		break;
		case NETPLAY_LOAD_ROM_REQ:
		{
			netPlayLoadRomReq *msg = static_cast<netPlayLoadRomReq*>(msgBuf);
			msg->toHostByteOrder();

			if (allowClientRomLoadReq)
			{
				if (client->romLoadData.buf != nullptr)
				{
					::free(client->romLoadData.buf);
					client->romLoadData.buf = nullptr;
				}
				client->romLoadData.size = 0;
				client->romLoadData.fileName.clear();

				const uint32_t fileSize = msg->fileSize;
				constexpr uint32_t maxRomDataSize = 1024 * 1024;
				const char *romData = &static_cast<const char*>(msgBuf)[ sizeof(netPlayLoadRomReq) ];

				if ( (fileSize >= 16) && (fileSize < maxRomDataSize) )
				{
					client->romLoadData.fileName = msg->fileName;
					client->romLoadData.size = fileSize;
					client->romLoadData.buf  = static_cast<char*>(::malloc(fileSize));
					::memcpy( client->romLoadData.buf, romData, fileSize );
					QTimer::singleShot( 100, this, SLOT(processClientRomLoadRequests(void)) );
				}

			}
		}
		break;
		case NETPLAY_SYNC_STATE_REQ:
		{
			FCEU_WRAPPER_LOCK();
			resyncClient( client );
			FCEU_WRAPPER_UNLOCK();
		}
		break;
		case NETPLAY_SYNC_STATE_RESP:
		{
			netPlayLoadStateResp* msg = static_cast<netPlayLoadStateResp*>(msgBuf);
			msg->toHostByteOrder();

			FCEU_printf("Sync state request received from client '%s'\n", client->userName.toLocal8Bit().constData());

			if (allowClientStateLoadReq)
			{
				const char *stateData = msg->stateDataBuf();
				const uint32_t stateDataSize = msg->stateDataSize();
				constexpr uint32_t maxStateDataSize = 1024*1024;

				if (client->stateLoadData.buf != nullptr)
				{
					::free(client->stateLoadData.buf);
					client->stateLoadData.buf = nullptr;
				}
				client->stateLoadData.size = 0;

				if ( (stateDataSize >= 16) && (stateDataSize < maxStateDataSize) )
				{
					client->stateLoadData.size = stateDataSize;
					client->stateLoadData.buf  = static_cast<char*>(::malloc(stateDataSize));
					::memcpy( client->stateLoadData.buf, stateData, stateDataSize );
					QTimer::singleShot( 100, this, SLOT(processClientStateLoadRequests(void)) );
				}
			}
		}
		break;
		default:
			printf("Unknown Msg: %08X\n", msgId);
		break;
	}

}
//-----------------------------------------------------------------------------
void NetPlayServer::processClientRomLoadRequests(void)
{
	for (auto& client : clientList )
	{
		if (client->romLoadData.pending())
		{
			bool acceptRomLoadReq = false;

			if (allowClientRomLoadReq)
			{
				QString msgBoxTxt = tr("Client '") + client->userName + tr("' has requested to load this ROM:\n\n");
	       			msgBoxTxt += client->romLoadData.fileName + tr("\n\nDo you want to load it?");
				int ans = QMessageBox::question( consoleWindow, tr("Client ROM Load Request"), msgBoxTxt, QMessageBox::Yes | QMessageBox::No );

				if (ans == QMessageBox::Yes)
				{
					acceptRomLoadReq = true;
				}
			}

			if (acceptRomLoadReq)
			{
				FILE *fp;
				QString filepath = QDir::tempPath();
				QFileInfo rompath = QFileInfo(client->romLoadData.fileName);
				const char *romData = client->romLoadData.buf;
				const size_t romSize = client->romLoadData.size;

				filepath.append( "/" );
				filepath.append( rompath.fileName() );

				//printf("Load ROM Request Received: %s\n", filepath.c_str());
				//printf("Dumping Temp Rom to: %s\n", filepath.c_str());
				fp = ::fopen( filepath.toLocal8Bit().constData(), "wb");

				if (fp == nullptr)
				{
					return;
				}
				::fwrite( romData, 1, romSize, fp );
				::fclose(fp);

				FCEU_WRAPPER_LOCK();
				LoadGame( filepath.toLocal8Bit().constData(), true, true );
				FCEUI_SetEmulationPaused(EMULATIONPAUSED_PAUSED);
				FCEU_WRAPPER_UNLOCK();

				sendPauseAll();
				resyncAllClients();
			}
			else
			{
				netPlayTextMsg<128>  errorMsg(NETPLAY_ERROR_MSG);
				errorMsg.setFlag(netPlayTextMsgFlags::Warning);
				errorMsg.printf("Host has rejected ROM load request");
				sendMsg( client, &errorMsg, errorMsg.hdr.msgSize, [&errorMsg]{ errorMsg.toNetworkByteOrder(); } );
			}

			::free(client->romLoadData.buf);
			client->romLoadData.buf  = nullptr;
			client->romLoadData.size = 0;
			client->romLoadData.fileName.clear();
		}
	}
}
//-----------------------------------------------------------------------------
void NetPlayServer::processClientStateLoadRequests(void)
{
	for (auto& client : clientList )
	{
		if (client->stateLoadData.pending())
		{
			EMUFILE_MEMORY em( client->stateLoadData.buf, client->stateLoadData.size );

			bool acceptStateLoadReq = false;

			if (allowClientStateLoadReq)
			{
				QString msgBoxTxt = tr("Client '") + client->userName + tr("' has requested to load a new state:\n");
				msgBoxTxt += tr("\nDo you want to load it?");
				int ans = QMessageBox::question( consoleWindow, tr("Client State Load Request"), msgBoxTxt, QMessageBox::Yes | QMessageBox::No );

				if (ans == QMessageBox::Yes)
				{
					acceptStateLoadReq = true;
				}
			}

			if (acceptStateLoadReq)
			{
				FCEU_WRAPPER_LOCK();
				serverRequestedStateLoad = true;
				// Clients will be resync'd during this load call.
				FCEUSS_LoadFP( &em, SSLOADPARAM_NOBACKUP );
				serverRequestedStateLoad = false;
				FCEU_WRAPPER_UNLOCK();
			}

			::free(client->stateLoadData.buf);
			client->stateLoadData.buf  = nullptr;
			client->stateLoadData.size = 0;
		}
	}
}
//-----------------------------------------------------------------------------
void NetPlayServer::update(void)
{
	bool hostRdyFrame = false;
	bool shouldRunFrame = false;
	unsigned int clientMinFrame = 0xFFFFFFFF;
	unsigned int clientMaxFrame = 0;
	const uint32_t maxLead = maxLeadFrames;
	const uint32_t currFrame = static_cast<uint32_t>(currFrameCounter);
	const uint32_t leadFrame = currFrame + maxLead;
	//const uint32_t lastFrame = inputFrameBack();
	uint32_t lagFrame = 0;
	uint8_t  localGP[4] = { 0 };
	uint8_t  gpData[4] = { 0 };
	int   numClientsPaused = 0;

	if (currFrame > maxLead)
	{
		lagFrame = currFrame - maxLead;
	}

	uint32_t ctlrData = GetGamepadPressedImmediate();
	localGP[0] = (ctlrData      ) & 0x000000ff;
	localGP[1] = (ctlrData >>  8) & 0x000000ff;
	localGP[2] = (ctlrData >> 16) & 0x000000ff;
	localGP[3] = (ctlrData >> 24) & 0x000000ff;

	if ( (role >= NETPLAY_PLAYER1) && (role <= NETPLAY_PLAYER4) )
	{
		gpData[role] = localGP[role];
	}

	// Input Processing
	for (auto it = clientList.begin(); it != clientList.end(); )
	{
		NetPlayClient *client = *it;

		client->readMessages( serverMessageCallback, client );

		if (client->isAuthenticated())
		{
			if (client->currentFrame < clientMinFrame)
			{
				clientMinFrame = client->currentFrame;
			}
			if (client->currentFrame > clientMaxFrame)
			{
				clientMaxFrame = client->currentFrame;
			}

			if (client->isPlayerRole())
			{
				gpData[client->role] = client->gpData[client->role];
			}

			if (client->isPaused())
			{
				numClientsPaused++;
			}
		}

		if (client->shouldDestroy())
		{
			it = clientList.erase(it);
			for (int i=0; i<4; i++)
			{
				if (client == clientPlayer[i])
				{
					clientPlayer[i] = nullptr;
				}
			}
			releaseRole(client);
			delete client;
			//printf("Emit clientDisconnected!\n");
			emit clientDisconnected();
		}
		else
		{
			it++;
		}
	}

	hostRdyFrame = (currFrame >= inputFrameCount);

	shouldRunFrame = (clientMinFrame != 0xFFFFFFFF) && 
		(clientMinFrame >= lagFrame ) &&
		(clientMaxFrame  < leadFrame) &&
		(numClientsPaused == 0) &&
		 hostRdyFrame;

	if (hostRdyFrame && !shouldRunFrame)
	{
		clientWaitCounter++;
	}
	else
	{
		clientWaitCounter = 0;
	}

	//printf("Host Frame: Run:%u   Input:%u   Last:%u\n", currFrame, inputFrameCount, lastFrame);
	//printf("Client Frame: Min:%u  Max:%u\n", clientMinFrame, clientMaxFrame);

	if (shouldRunFrame)
	{
		// Output Processing
		NetPlayFrameInput  inputFrame;
		netPlayRunFrameReq  runFrameReq;

		inputFrame.frameCounter = ++inputFrameCount;
		inputFrame.ctrl[0] = gpData[0];
		inputFrame.ctrl[1] = gpData[1];
		inputFrame.ctrl[2] = gpData[2];
		inputFrame.ctrl[3] = gpData[3];

		runFrameReq.frameNum     = inputFrame.frameCounter;
		runFrameReq.ctrlState[0] = inputFrame.ctrl[0];
		runFrameReq.ctrlState[1] = inputFrame.ctrl[1];
		runFrameReq.ctrlState[2] = inputFrame.ctrl[2];
		runFrameReq.ctrlState[3] = inputFrame.ctrl[3];

		uint32_t  catchUpThreshold = maxLead;
		if (catchUpThreshold < 3)
		{
			catchUpThreshold = 3;
		}
		runFrameReq.catchUpThreshold = catchUpThreshold;

		pushBackInput( inputFrame );

		runFrameReq.toNetworkByteOrder();

		for (auto& client : clientList )
		{
			if (client->state > 0)
			{
				sendMsg( client, &runFrameReq, sizeof(runFrameReq) );
			}
		}
	}

	bool shouldRunPing = (cycleCounter % 120) == 0;
	
	if (shouldRunPing)
	{
		FCEU::timeStampRecord ts;
		ts.readNew();

		netPlayPingReq  ping;
		ping.hostTimeStamp = ts.toMilliSeconds();
		ping.toNetworkByteOrder();

		for (auto& client : clientList )
		{
			if (client->state > 0)
			{
				sendMsg( client, &ping, sizeof(ping) );
			}
		}
	}

	bool shouldFlushOutput = true;
	if (shouldFlushOutput)
	{
		for (auto& client : clientList )
		{
			client->flushData();
		}
	}
	cycleCounter++;
}
//-----------------------------------------------------------------------------
//--- NetPlayClient
//-----------------------------------------------------------------------------
NetPlayClient *NetPlayClient::instance = nullptr;

NetPlayClient::NetPlayClient(QObject *parent, bool outGoing)
	: role(0), state(0), currentFrame(0), sock(nullptr), recvMsgId(-1), recvMsgSize(0),
		recvMsgBytesLeft(0), recvMsgByteIndex(0), recvMsgBuf(nullptr)
{
	FCEU::timeStampRecord ts;
	ts.readNew();
	spawnTimeStampMs = ts.toMilliSeconds();

	if (outGoing)
	{
		instance = this;
	}

	recvMsgBuf = static_cast<char*>( ::malloc( recvMsgBufSize ) );

	if (recvMsgBuf == nullptr)
	{
		printf("Error: NetPlayClient failed to allocate recvMsgBuf\n");
	}

	if (outGoing)
	{
		connect(consoleWindow, SIGNAL(romLoaded(void)), this, SLOT(onRomLoad(void)));
		connect(consoleWindow, SIGNAL(romUnload(void)), this, SLOT(onRomUnload(void)));

		FCEU_WRAPPER_LOCK();
		if (currCartInfo != nullptr)
		{
			romCrc32 = currCartInfo->CRC32;
		}
		FCEU_WRAPPER_UNLOCK();
	}
}


NetPlayClient::~NetPlayClient(void)
{
	if (instance == this)
	{
		instance = nullptr;
	}

	if (romLoadData.buf != nullptr)
	{
		::free(romLoadData.buf);
		romLoadData.buf = nullptr;
	}
	romLoadData.size = 0;
	romLoadData.fileName.clear();

	if (stateLoadData.buf != nullptr)
	{
		::free(stateLoadData.buf);
		stateLoadData.buf = nullptr;
	}
	stateLoadData.size = 0;

	if (sock != nullptr)
	{
		sock->close();
		delete sock; sock = nullptr;
	}
	if (recvMsgBuf)
	{
		::free(recvMsgBuf); recvMsgBuf = nullptr;
	}

	if (numMsgBoxObjs != 0)
	{
		printf("Warning: Client has leaked message dialogs\n");
	}

	if (debugLog != nullptr)
	{
		debugLog->close();
	}
	printf("NetPlayClient Destructor\n");
}

int NetPlayClient::Create(QObject *parent)
{
	if (NetPlayClient::GetInstance() == nullptr)
	{
		NetPlayClient *client = new NetPlayClient(parent, true);

		if (client == nullptr)
		{
			printf("Error Creating Netplay Client!!!\n");
		}
	}
	FCEU_WRAPPER_LOCK();
	if (traceRegistrationHandle == nullptr)
	{
		traceRegistrationHandle = FCEUI_TraceInstructionRegister( NetPlayTraceInstruction );
	}
	FCEU_WRAPPER_UNLOCK();

	if (consoleWindow != nullptr)
	{
		consoleWindow->onNetPlayChange();
	}
	return 0;
}

int NetPlayClient::Destroy()
{
	NetPlayClient* client = NetPlayClient::GetInstance();
	if (client != nullptr)
	{
		delete client;
		client = nullptr;
	}
	FCEU_WRAPPER_LOCK();
	if (traceRegistrationHandle != nullptr)
	{
		if ( !FCEUI_TraceInstructionUnregisterHandle( traceRegistrationHandle ) )
		{
			printf("Unregister Trace Callback Error\n");
		}
		traceRegistrationHandle = nullptr;
	}
	FCEU_WRAPPER_UNLOCK();

	if (consoleWindow != nullptr)
	{
		consoleWindow->onNetPlayChange();
	}
	return 0;
}
//-----------------------------------------------------------------------------
void NetPlayClient::onRomLoad()
{
	FCEU_WRAPPER_LOCK();
	if (currCartInfo != nullptr)
	{
		romCrc32 = currCartInfo->CRC32;
	}
	else
	{
		romCrc32 = 0;
	}
	FCEU_WRAPPER_UNLOCK();
}
//-----------------------------------------------------------------------------
void NetPlayClient::onRomUnload()
{
	romCrc32 = 0;
}
//-----------------------------------------------------------------------------
void NetPlayClient::forceDisconnect()
{
	disconnectPending = true;
	needsDestroy = true;
}
//-----------------------------------------------------------------------------
bool NetPlayClient::isAuthenticated()
{
	return (state > 0);
}
//-----------------------------------------------------------------------------
bool NetPlayClient::isPlayerRole()
{
	return (role >= NETPLAY_PLAYER1) && (role <= NETPLAY_PLAYER4);
}
//-----------------------------------------------------------------------------
bool NetPlayClient::flushData()
{
	bool success = false;

	if (sock != nullptr)
	{
		success = sock->flush();
	}
	return success;
}
//-----------------------------------------------------------------------------
void NetPlayClient::setSocket(QTcpSocket *s)
{
	sock = s;

	if (sock != nullptr)
	{
		if (debugLog != nullptr)
		{
			FCEU::FixedString<256> logMsg;


			if (isNetPlayHost())
			{
				QHostAddress addr = sock->peerAddress();
				int port = sock->peerPort();

				logMsg.sprintf("Client Socket Connected: %s:%i\n", addr.toString().toLocal8Bit().constData(), port );
			}
			else
			{
				QHostAddress addr = sock->localAddress();
				int port = sock->localPort();

				logMsg.sprintf("Client Socket Connected: %s:%i\n", addr.toString().toLocal8Bit().constData(), port );
			}

			//printf("LogMsg: %zu : %s\n", logMsg.size(), logMsg.c_str());
			debugLog->write( logMsg.c_str(), logMsg.size() );
			debugLog->flush();
		}
		sock->setSocketOption( QAbstractSocket::QAbstractSocket::LowDelayOption, 1);
		sock->setSocketOption( QAbstractSocket::SendBufferSizeSocketOption, static_cast<int>(recvMsgBufSize) );
		sock->setSocketOption( QAbstractSocket::ReceiveBufferSizeSocketOption, static_cast<int>(recvMsgBufSize) );

		connect(sock, SIGNAL(connected(void))   , this, SLOT(onConnect(void)));
		connect(sock, SIGNAL(disconnected(void)), this, SLOT(onDisconnect(void)));
	}
}
//-----------------------------------------------------------------------------
QTcpSocket* NetPlayClient::createSocket(void)
{
	if (sock == nullptr)
	{
		sock = new QTcpSocket(this);

		sock->setSocketOption( QAbstractSocket::QAbstractSocket::LowDelayOption, 1);
		sock->setSocketOption( QAbstractSocket::SendBufferSizeSocketOption, static_cast<int>(recvMsgBufSize) );
		sock->setSocketOption( QAbstractSocket::ReceiveBufferSizeSocketOption, static_cast<int>(recvMsgBufSize) );

		connect(sock, SIGNAL(readyRead(void))   , this, SLOT(clientReadyRead(void)) );
		connect(sock, SIGNAL(connected(void))   , this, SLOT(onConnect(void)));
		connect(sock, SIGNAL(disconnected(void)), this, SLOT(onDisconnect(void)));
		connect(sock, SIGNAL(errorOccurred(QAbstractSocket::SocketError)), this, SLOT(onSocketError(QAbstractSocket::SocketError)));
	}

	return sock;
}
//-----------------------------------------------------------------------------
int NetPlayClient::connectToHost( const QString host, int port )
{
	createSocket();

	sock->connectToHost( host, port );

	//if (sock->waitForConnected(10000))
	//{
    	//	qDebug("Connected!");
	//}
	//else
	//{
    	//	qDebug("Failed to Connect!");
	//}


	return 0;
}
//-----------------------------------------------------------------------------
void NetPlayClient::onConnect(void)
{
	printf("Client Connected!!!\n");
	FCEU_DispMessage("Joined Host",0);
	_connected = true;

	emit connected();
}
//-----------------------------------------------------------------------------
void NetPlayClient::onDisconnect(void)
{
	printf("Client Disconnected!!!\n");

	NetPlayServer *server = NetPlayServer::GetInstance();

	if (server)
	{
		FCEU_DispMessage("%s Disconnected",0, userName.toLocal8Bit().constData());
	}
	else
	{
		FCEU_DispMessage("Host Disconnected",0);
	}
	needsDestroy = true;
}
//-----------------------------------------------------------------------------
void NetPlayClient::onSocketError(QAbstractSocket::SocketError error)
{
	//FCEU_DispMessage("Socket Error",0);

	QString errorMsg = sock->errorString();
	printf("Error: %s\n", errorMsg.toLocal8Bit().constData());

	FCEU_DispMessage("Socket Error: %s", 0, errorMsg.toLocal8Bit().constData());

	emit errorOccurred(errorMsg);
}
//-----------------------------------------------------------------------------
int NetPlayClient::requestRomLoad( const char *romPathIn )
{
	constexpr size_t BufferSize = 32 * 1024;
	char buf[BufferSize];
	size_t bytesRead;
	long fileSize = 0;
	int userCancel = 0;
	const int archiveIndex = -1;
	std::string romPath(romPathIn);
	netPlayLoadRomReq msg;
	const char* romextensions[] = { "nes", "fds", "nsf", 0 };

	printf("Prep ROM Load Request: %s \n", romPath.c_str() );
	FCEUFILE* fp = FCEU_fopen( romPath.c_str(), nullptr, "rb", 0, archiveIndex, romextensions, &userCancel);

	if (fp == nullptr)
	{
		return -1;
	}
	QFileInfo fi( fp->filename.c_str() );
	fileSize = fp->size;

	msg.hdr.msgSize += fileSize;
	msg.fileSize     = fileSize;
	Strlcpy( msg.fileName, fi.fileName().toLocal8Bit().constData(), sizeof(msg.fileName) );

	printf("Sending ROM Load Request: %s  %lu\n", msg.fileName, fileSize );

	msg.toNetworkByteOrder();
	sock->write( reinterpret_cast<const char*>(&msg), sizeof(netPlayLoadRomReq) );

	while ( (bytesRead = FCEU_fread( buf, 1, sizeof(buf), fp )) > 0 )
	{
		sock->write( buf, bytesRead );
	}
	sock->flush();

	FCEU_fclose(fp);

	return 0;
}
//-----------------------------------------------------------------------------
int NetPlayClient::requestStateLoad(EMUFILE *is)
{
	size_t dataSize;
	char *dataBuf;
	static constexpr size_t maxBytesPerWrite = 32 * 1024;
	netPlayLoadStateResp resp;

	dataSize = is->size();
	resp.hdr.msgSize += dataSize;
	resp.stateSize    = dataSize;

	if (dataSize == 0)
	{
		return -1;
	}

	dataBuf = static_cast<char*>(::malloc(dataSize));

	if (dataBuf == nullptr)
	{
		return -1;
	}
	is->fseek( 0, SEEK_SET );
	size_t readResult = is->fread( dataBuf, dataSize );

	if (readResult != dataSize )
	{
		printf("Read Error\n");
	}

	if (currCartInfo != nullptr)
	{
		resp.romCrc32 = romCrc32;
	}
	printf("Sending Client ROM Sync Request: %u\n", resp.stateSize);

	resp.toNetworkByteOrder();
	sock->write( reinterpret_cast<const char*>(&resp), sizeof(netPlayLoadStateResp));

	const char* bufPtr = dataBuf;
	while (dataSize > 0)
	{
		size_t bytesToWrite = dataSize;

		if (bytesToWrite > maxBytesPerWrite)
		{
			bytesToWrite = maxBytesPerWrite;
		}
		sock->write( bufPtr, bytesToWrite );

		bufPtr += bytesToWrite;
		dataSize -= bytesToWrite;
	}
	sock->flush();

	::free(dataBuf);

	return 0;
}
//-----------------------------------------------------------------------------
int NetPlayClient::requestSync(void)
{
	netPlayMsgHdr hdr(NETPLAY_SYNC_STATE_REQ);

	hdr.toNetworkByteOrder();
	sock->write( reinterpret_cast<const char*>(&hdr), sizeof(netPlayMsgHdr));
	return 0;
}
//-----------------------------------------------------------------------------
void NetPlayClient::recordPingResult( const uint64_t delay_ms )
{
	pingNumSamples++;
	pingDelayLast = delay_ms;
	pingDelaySum += delay_ms;

	if (delay_ms < pingDelayMin)
	{
		pingDelayMin = delay_ms;
	}

	if (delay_ms > pingDelayMax)
	{
		pingDelayMax = delay_ms;
	}
}
//-----------------------------------------------------------------------------
void NetPlayClient::resetPingData()
{
	pingNumSamples = 0;
	pingDelayLast = 0;
	pingDelaySum = 0;
	pingDelayMax = 0;
	pingDelayMin = 1000;
}
//-----------------------------------------------------------------------------
double NetPlayClient::getAvgPingDelay()
{
	double ms = 0.0;
	if (pingNumSamples > 0)
	{
		ms = static_cast<double>(pingDelaySum) / static_cast<double>(pingNumSamples);
	}
	return ms;
}
//-----------------------------------------------------------------------------
static void clientMessageCallback( void *userData, void *msgBuf, size_t msgSize )
{
	NetPlayClient *client = static_cast<NetPlayClient*>(userData);

	client->clientProcessMessage( msgBuf, msgSize );
}
//-----------------------------------------------------------------------------
void NetPlayClient::update(void)
{
	readMessages( clientMessageCallback, this );

	if (_connected)
	{
		uint32_t ctlrData = GetGamepadPressedImmediate();
		uint32_t currFrame = static_cast<uint32_t>(currFrameCounter);

		NetPlayFrameData lastFrameData;
		netPlayFrameData.getLast( lastFrameData );

		netPlayClientState  statusMsg;
		statusMsg.flags     = 0;
		if (FCEUI_EmulationPaused())
		{
			statusMsg.flags |= netPlayClientState::PauseFlag;
		}
		if (desyncCount > 0)
		{
			statusMsg.flags |= netPlayClientState::DesyncFlag;
		}
		statusMsg.frameRdy  = inputFrameBack();
		statusMsg.frameRun  = currFrame;
		statusMsg.opsFrame  = lastFrameData.frameNum;
		statusMsg.opsChkSum = lastFrameData.opsCrc32;
		statusMsg.ramChkSum = lastFrameData.ramCrc32;
		statusMsg.romCrc32  = romCrc32;
		statusMsg.ctrlState[0] = (ctlrData      ) & 0x000000ff;
		statusMsg.ctrlState[1] = (ctlrData >>  8) & 0x000000ff;
		statusMsg.ctrlState[2] = (ctlrData >> 16) & 0x000000ff;
		statusMsg.ctrlState[3] = (ctlrData >> 24) & 0x000000ff;

		statusMsg.toNetworkByteOrder();
		sock->write( reinterpret_cast<const char*>(&statusMsg), sizeof(statusMsg) );

		flushData();
	}
}
//-----------------------------------------------------------------------------
int NetPlayClient::readMessages( void (*msgCallback)( void *userData, void *msgBuf, size_t msgSize ), void *userData )
{
	FCEU::FixedString<256> logMsg;

	if (readMessageProcessing)
	{
		printf("Read Message is Processing in callstack, don't allow re-entrantancy.\n");
		return 0;
	}
	readMessageProcessing = true;

	if (sock)
	{
		bool readReq;
		netPlayMsgHdr *hdr;
		constexpr int netPlayMsgHdrSize = sizeof(netPlayMsgHdr);
		long int ts = 0;

		int bytesAvailable = sock->bytesAvailable();
		readReq = bytesAvailable > 0;

		if (debugLog != nullptr)
		{
			FCEU::timeStampRecord tsRec;
			tsRec.readNew();
			ts = tsRec.toMilliSeconds() - spawnTimeStampMs;
		}

		while (readReq)
		{
			//printf("Read Bytes Available: %lu  %i  %i\n", ts.toMilliSeconds(), bytesAvailable, recvMsgBytesLeft);

			if (recvMsgBytesLeft > 0)
			{
				bytesAvailable = sock->bytesAvailable();
				readReq = (bytesAvailable > 0);

				if (readReq)
				{
					int dataRead;
					size_t readSize = recvMsgBytesLeft;

					dataRead = sock->read( &recvMsgBuf[recvMsgByteIndex], readSize );

					if (dataRead > 0)
					{
						recvMsgBytesLeft -= dataRead;
						recvMsgByteIndex += dataRead;
					}
					//printf("   Data: Id: %u   Size: %zu  Read: %i\n", recvMsgId, readSize, dataRead );
					if (debugLog != nullptr)
					{
						logMsg.sprintf("%lu: Recv Chunk: Size:%i     Left:%i\n", ts, dataRead, recvMsgBytesLeft);
						debugLog->write( logMsg.c_str(), logMsg.size() );
					}

					if (recvMsgBytesLeft > 0)
					{
						bytesAvailable = sock->bytesAvailable();
						readReq = (bytesAvailable > 0);
					}
					else
					{
						if (debugLog != nullptr)
						{
							logMsg.sprintf("%lu: Recv End: Size:%i   \n", ts, recvMsgSize);
							debugLog->write( logMsg.c_str(), logMsg.size() );
						}
						msgCallback( userData, recvMsgBuf, recvMsgSize );
						bytesAvailable = sock->bytesAvailable();
						readReq = (bytesAvailable > 0);
					}
				}
			}
			else if (bytesAvailable >= netPlayMsgHdrSize)
			{
				sock->read( recvMsgBuf, netPlayMsgHdrSize );

				hdr = (netPlayMsgHdr*)recvMsgBuf;

				recvMsgId   = netPlayByteSwap(hdr->msgId);
				recvMsgSize = netPlayByteSwap(hdr->msgSize) - sizeof(netPlayMsgHdr);
				recvMsgBytesLeft = recvMsgSize;

				if ( (netPlayByteSwap(hdr->magic[0]) != NETPLAY_MAGIC_NUMBER) || 
				     (netPlayByteSwap(hdr->magic[1]) != NETPLAY_MAGIC_NUMBER) )
				{
					FCEU_printf("Netplay Error: Message Header Validity Check Failed: %08X\n", recvMsgId);
					forceDisconnect();
					return -1;
				}

				if (netPlayByteSwap(hdr->msgSize) > recvMsgBufSize)
				{
					FCEU_printf("Netplay Error: Message size too large: %08X\n", recvMsgId);
					forceDisconnect();
					return -1;
				}

				if (debugLog != nullptr)
				{
					logMsg.sprintf("%lu: Recv Start: HDR Info: Id:%u   ExpectedSize:%i\n", ts, netPlayByteSwap(hdr->msgId), recvMsgSize );
					debugLog->write( logMsg.c_str(), logMsg.size() );
				}
				//printf("HDR: Id: %u   Size: %u\n", netPlayByteSwap(hdr->msgId), netPlayByteSwap(hdr->msgSize) );

				recvMsgByteIndex = sizeof(netPlayMsgHdr);

				if (recvMsgBytesLeft == 0)
				{
					msgCallback( userData, recvMsgBuf, recvMsgSize );
				}
				bytesAvailable = sock->bytesAvailable();
				readReq = (bytesAvailable > 0);
			}
			else
			{
				readReq = false;
			}
		}
	}
	readMessageProcessing = false;

	return 0;
}
//-----------------------------------------------------------------------------
void NetPlayClient::clientProcessMessage( void *msgBuf, size_t msgSize )
{
	netPlayMsgHdr *hdr = (netPlayMsgHdr*)msgBuf;

	const uint32_t msgId = netPlayByteSwap(hdr->msgId);

	switch (msgId)
	{
		case NETPLAY_INFO_MSG:
		{
			auto *msg = static_cast<netPlayTextMsg<256>*>(msgBuf);
			msg->toHostByteOrder();
			FCEU_printf("NetPlay Info: %s\n", msg->getBuffer());

			if (msg->isFlagSet(netPlayTextMsgFlags::Disconnect))
			{
				sock->disconnectFromHost();
			}
			FCEU_DispMessage("NetPlay Errors... check message log",0);
		}
		break;
		case NETPLAY_ERROR_MSG:
		{
			auto *msg = static_cast<netPlayTextMsg<256>*>(msgBuf);
			msg->toHostByteOrder();
			FCEU_printf("NetPlay Error: %s\n", msg->getBuffer());

			numMsgBoxObjs++;
			QString msgBoxTxt  = tr("Host has replied with an error:\n\n");
	       		msgBoxTxt += tr(msg->getBuffer());

			QMessageBox* msgBox = new QMessageBox( QMessageBox::Critical, tr("NetPlay Error"), msgBoxTxt, QMessageBox::Ok, consoleWindow );
			connect( msgBox->button(QMessageBox::Ok), SIGNAL(clicked(void)), msgBox, SLOT(accept(void)));
			connect( msgBox, &QMessageBox::finished, this, [this, msgBox](){ msgBox->deleteLater(); } );
			connect( msgBox, SIGNAL(destroyed(QObject*)), this, SLOT(onMessageBoxDestroy(QObject*)));
			msgBox->show();

			if (msg->isFlagSet(netPlayTextMsgFlags::Disconnect))
			{
				sock->disconnectFromHost();
			}
			FCEU_DispMessage("NetPlay Errors... check message log",0);
		}
		break;
		case NETPLAY_AUTH_REQ:
		{
			netPlayAuthResp msg;
			msg.playerId = role;
			Strlcpy( msg.userName, userName.toLocal8Bit().constData(), sizeof(msg.userName));
			Strlcpy( msg.pswd, password.toLocal8Bit().constData(), sizeof(msg.pswd) );

			FCEU_printf("Authentication Request Received\n");
			msg.toNetworkByteOrder();
			sock->write( (const char*)&msg, sizeof(netPlayAuthResp) );
		}
		break;
		case NETPLAY_LOAD_ROM_REQ:
		{
			netPlayLoadRomReq *msg = static_cast<netPlayLoadRomReq*>(msgBuf);
			msg->toHostByteOrder();
			const char *romData = &static_cast<const char*>(msgBuf)[ sizeof(netPlayLoadRomReq) ];

			QFileInfo fileInfo = QFileInfo(msg->fileName);
			QString tempPath = consoleWindow->getTempDir() + QString("/") + fileInfo.fileName();
			FCEU_printf("Load ROM Request Received: %s\n", fileInfo.fileName().toLocal8Bit().constData());

			QFile tmpFile( tempPath );
			//tmpFile.setFileTemplate(QDir::tempPath() + QString("/tmpRom_XXXXXX.nes"));
			tmpFile.open(QIODevice::ReadWrite);
			QString filepath = tmpFile.fileName();
			printf("Dumping Temp Rom to: %s\n", tmpFile.fileName().toLocal8Bit().constData());
			tmpFile.write( romData, msgSize );
			tmpFile.close();

			FCEU_WRAPPER_LOCK();
			fceuWrapperSetArchiveFileLoadIndex( msg->archiveIndex );
			LoadGame( filepath.toLocal8Bit().constData(), true, true );
			fceuWrapperClearArchiveFileLoadIndex();

			opsCrc32 = 0;
			netPlayFrameData.reset();
			inputClear();
			FCEU_WRAPPER_UNLOCK();
		}
		break;
		case NETPLAY_UNLOAD_ROM_REQ:
		{
			FCEU_WRAPPER_LOCK();
			CloseGame();
			FCEU_WRAPPER_UNLOCK();
		}
		break;
		case NETPLAY_SYNC_STATE_RESP:
		{
			netPlayLoadStateResp* msg = static_cast<netPlayLoadStateResp*>(msgBuf);
			msg->toHostByteOrder();

			const bool romMatch = (msg->romCrc32 = romCrc32);
			char *stateData = msg->stateDataBuf();
			const uint32_t stateDataSize = msg->stateDataSize();

			FCEU_printf("Sync state Request Received: %u\n", stateDataSize);

			EMUFILE_MEMORY em( stateData, stateDataSize );

			FCEU_WRAPPER_LOCK();

			bool dataValid = romMatch;

			if (dataValid)
			{
				serverRequestedStateLoad = true;
				FCEUSS_LoadFP( &em, SSLOADPARAM_NOBACKUP );
				serverRequestedStateLoad = false;

				opsCrc32 = msg->opsCrc32;
				netPlayFrameData.reset();

				NetPlayFrameData data;
				data.frameNum = msg->lastFrame.num;
				data.opsCrc32 = msg->lastFrame.opsCrc32;
				data.ramCrc32 = msg->lastFrame.ramCrc32;

				netPlayFrameData.push( data );

				inputClear();

				const int numInputFrames = msg->numCtrlFrames;
				for (int i=0; i<numInputFrames; i++)
				{
					NetPlayFrameInput inputFrame;
					auto ctrlData = msg->ctrlDataBuf();

					ctrlData[i].toHostByteOrder();
					inputFrame.frameCounter = ctrlData[i].frameNum;
					inputFrame.ctrl[0] = ctrlData[i].ctrlState[0];
					inputFrame.ctrl[1] = ctrlData[i].ctrlState[1];
					inputFrame.ctrl[2] = ctrlData[i].ctrlState[2];
					inputFrame.ctrl[3] = ctrlData[i].ctrlState[3];

					pushBackInput( inputFrame );
				}

				const int numCheats = msg->numCheats;

				if (numCheats > 0)
				{
					const int lastCheatIdx = numCheats - 1;

					FCEU_FlushGameCheats(0, 1);
					for (int i=0; i<numCheats; i++)
					{
						auto cheatBuf = msg->cheatDataBuf();
						auto& cheatData = cheatBuf[i];
						// Set cheat rebuild flag on last item.
						bool lastItem = (i == lastCheatIdx);

						FCEUI_AddCheat( cheatData.name, cheatData.addr, cheatData.val, cheatData.cmp, cheatData.type, cheatData.stat, lastItem );
					}
				}
				else
				{
					FCEU_DeleteAllCheats();
				}
			}
			FCEU_WRAPPER_UNLOCK();

		}
		break;
		case NETPLAY_RUN_FRAME_REQ:
		{
			NetPlayFrameInput  inputFrame;
			netPlayRunFrameReq *msg = static_cast<netPlayRunFrameReq*>(msgBuf);
			msg->toHostByteOrder();

			inputFrame.frameCounter = msg->frameNum;
			inputFrame.ctrl[0] = msg->ctrlState[0];
			inputFrame.ctrl[1] = msg->ctrlState[1];
			inputFrame.ctrl[2] = msg->ctrlState[2];
			inputFrame.ctrl[3] = msg->ctrlState[3];

			catchUpThreshold   = msg->catchUpThreshold;

			uint32_t lastInputFrame = inputFrameBack();
			uint32_t currFrame = static_cast<uint32_t>(currFrameCounter);

			if (inputFrame.frameCounter > lastInputFrame)
			{
				pushBackInput( inputFrame );
			}
			else
			{
				printf("Drop Frame: LastRun:%u   LastInput:%u   NewInput:%u\n", currFrame, lastInputFrame, inputFrame.frameCounter);
			}
			//printf("Run Frame: LastRun:%u   LastInput:%u   NewInput:%u\n", currFrame, lastInputFrame, inputFrame.frameCounter);
		}
		break;
		case NETPLAY_PING_REQ:
		{
			netPlayPingResp pong;
			netPlayPingReq *ping = static_cast<netPlayPingReq*>(msgBuf);
			ping->toHostByteOrder();

			pong.hostTimeStamp = ping->hostTimeStamp;
			pong.toNetworkByteOrder();
			sock->write( (const char*)&pong, sizeof(netPlayPingResp) );
		}
		break;
		case NETPLAY_CLIENT_PAUSE_REQ:
		{
			if (!FCEUI_EmulationPaused())
			{
				FCEUI_ToggleEmulationPause();
			}
		}
		break;
		case NETPLAY_CLIENT_UNPAUSE_REQ:
		{
			if (FCEUI_EmulationPaused())
			{
				FCEUI_ToggleEmulationPause();
			}
		}
		break;
		default:
			printf("Unknown Msg: %08X\n", msgId);
		break;
	}
}
//-----------------------------------------------------------------------------
void NetPlayClient::serverReadyRead()
{
	//printf("Server Ready Read\n");
	readMessages( serverMessageCallback, this );
}
//-----------------------------------------------------------------------------
void NetPlayClient::clientReadyRead()
{
	//printf("Client Ready Read\n");
	readMessages( clientMessageCallback, this );
}
//-----------------------------------------------------------------------------
void NetPlayClient::onMessageBoxDestroy(QObject* obj)
{
	//printf("MessageBox Destroyed: %p\n", obj);
	if (numMsgBoxObjs > 0)
	{
		numMsgBoxObjs--;
	}
	else
	{
		printf("Warning: Unexpected MessageBox Destroy: %p\n", obj);
	}
}
//-----------------------------------------------------------------------------
//--- NetPlayHostDialog
//-----------------------------------------------------------------------------
NetPlayHostDialog* NetPlayHostDialog::instance = nullptr;
//-----------------------------------------------------------------------------
NetPlayHostDialog::NetPlayHostDialog(QWidget *parent)
	: QDialog(parent)
{
	QVBoxLayout *mainLayout;
	QHBoxLayout *hbox;
	QGridLayout *grid;
	QGroupBox   *networkGroupBox;
	QGroupBox   *settingsGroupBox;
	QPushButton *cancelButton, *startButton;
	QLabel *lbl;

	instance = this;

	setWindowTitle("NetPlay Host Game");

	mainLayout = new QVBoxLayout();
	networkGroupBox = new QGroupBox(tr("Network Setup"));
	settingsGroupBox = new QGroupBox(tr("Server Settings"));
	hbox = new QHBoxLayout();
	grid = new QGridLayout();

	mainLayout->addLayout(hbox);
	hbox->addWidget(networkGroupBox);
	hbox->addWidget(settingsGroupBox);

	// Network Settings 
	networkGroupBox->setLayout(grid);

	lbl = new QLabel( tr("Server Name:") );
	grid->addWidget( lbl, 0, 0 );

	sessionNameEntry = new QLineEdit();
	sessionNameEntry->setText("My Game");
	grid->addWidget( sessionNameEntry, 0, 1 );

	lbl = new QLabel( tr("Port:") );
	grid->addWidget( lbl, 1, 0 );

	int netPort = NetPlayServer::DefaultPort;
	g_config->getOption("SDL.NetworkPort", &netPort);
	portEntry = new QSpinBox();
	portEntry->setRange(0,65535);
	portEntry->setValue(netPort);
	grid->addWidget( portEntry, 1, 1 );

	lbl = new QLabel( tr("Role:") );
	grid->addWidget( lbl, 2, 0 );

	playerRoleBox = new QComboBox();
	playerRoleBox->addItem( tr("Spectator"), NETPLAY_SPECTATOR);
	playerRoleBox->addItem( tr("Player 1") , NETPLAY_PLAYER1  );
	playerRoleBox->addItem( tr("Player 2") , NETPLAY_PLAYER2  );
	playerRoleBox->addItem( tr("Player 3") , NETPLAY_PLAYER3  );
	playerRoleBox->addItem( tr("Player 4") , NETPLAY_PLAYER4  );
	playerRoleBox->setCurrentIndex(1);
	grid->addWidget( playerRoleBox, 2, 1 );

	passwordRequiredCBox = new QCheckBox(tr("Password Required"));
	grid->addWidget( passwordRequiredCBox, 3, 0, 1, 2 );

	lbl = new QLabel( tr("Password:") );
	grid->addWidget( lbl, 4, 0 );

	passwordEntry = new QLineEdit();
	grid->addWidget( passwordEntry, 4, 1 );
	passwordEntry->setEnabled( passwordRequiredCBox->isChecked() );

	// Misc Settings 
	grid = new QGridLayout();
	settingsGroupBox->setLayout(grid);

	lbl = new QLabel( tr("Max Frame Lead:") );
	frameLeadSpinBox = new QSpinBox();
	frameLeadSpinBox->setRange(5,60);
	frameLeadSpinBox->setValue(30);
	grid->addWidget( lbl, 0, 0, 1, 1 );
	grid->addWidget( frameLeadSpinBox, 0, 1, 1, 1 );

	bool enforceAppVersionChk = false;
	enforceAppVersionChkCBox = new QCheckBox(tr("Enforce Client Versions Match"));
	grid->addWidget( enforceAppVersionChkCBox, 1, 0, 1, 2 );
	g_config->getOption("SDL.NetPlayHostEnforceAppVersionChk", &enforceAppVersionChk);
	enforceAppVersionChkCBox->setChecked(enforceAppVersionChk);

	bool romLoadReqEna = false;
	allowClientRomReqCBox = new QCheckBox(tr("Allow Client ROM Load Requests"));
	grid->addWidget( allowClientRomReqCBox, 2, 0, 1, 2 );
	g_config->getOption("SDL.NetPlayHostAllowClientRomLoadReq", &romLoadReqEna);
	allowClientRomReqCBox->setChecked(romLoadReqEna);

	bool stateLoadReqEna = false;
	allowClientStateReqCBox = new QCheckBox(tr("Allow Client State Load Requests"));
	grid->addWidget( allowClientStateReqCBox, 3, 0, 1, 2 );
	g_config->getOption("SDL.NetPlayHostAllowClientStateLoadReq", &stateLoadReqEna);
	allowClientStateReqCBox->setChecked(stateLoadReqEna);

	debugModeCBox = new QCheckBox(tr("Debug Network Messaging"));
	grid->addWidget( debugModeCBox, 4, 0, 1, 2 );

	connect(passwordRequiredCBox, SIGNAL(stateChanged(int)), this, SLOT(passwordRequiredChanged(int)));
	connect(allowClientRomReqCBox, SIGNAL(stateChanged(int)), this, SLOT(allowClientRomReqChanged(int)));
	connect(allowClientStateReqCBox, SIGNAL(stateChanged(int)), this, SLOT(allowClientStateReqChanged(int)));
	connect(enforceAppVersionChkCBox, SIGNAL(stateChanged(int)), this, SLOT(enforceAppVersionChkChanged(int)));

	startButton = new QPushButton( tr("Start") );
	startButton->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));
	connect(startButton, SIGNAL(clicked(void)), this, SLOT(onStartClicked(void)));

	cancelButton = new QPushButton( tr("Cancel") );
	cancelButton->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton));
	connect(cancelButton, SIGNAL(clicked(void)), this, SLOT(closeWindow(void)));

	hbox = new QHBoxLayout();
	hbox->addWidget( cancelButton, 1 );
	hbox->addStretch(5);
	hbox->addWidget( startButton, 1 );
	mainLayout->addLayout( hbox );

	setLayout(mainLayout);
}
//----------------------------------------------------------------------------
NetPlayHostDialog::~NetPlayHostDialog(void)
{
	instance = nullptr;
	//printf("Destroy NetPlay Host Window\n");
}
//----------------------------------------------------------------------------
void NetPlayHostDialog::closeEvent(QCloseEvent *event)
{
	//printf("NetPlay Host Close Window Event\n");
	done(0);
	deleteLater();
	event->accept();
}
//----------------------------------------------------------------------------
void NetPlayHostDialog::closeWindow(void)
{
	//printf("Close Window\n");
	done(0);
	deleteLater();
}
//-----------------------------------------------------------------------------
void NetPlayHostDialog::passwordRequiredChanged(int state)
{
	passwordEntry->setEnabled( state != Qt::Unchecked );
}
//-----------------------------------------------------------------------------
void NetPlayHostDialog::allowClientRomReqChanged(int state)
{
	g_config->setOption("SDL.NetPlayHostAllowClientRomLoadReq", state != Qt::Unchecked);
	g_config->save();
}
//-----------------------------------------------------------------------------
void NetPlayHostDialog::allowClientStateReqChanged(int state)
{
	g_config->setOption("SDL.NetPlayHostAllowClientStateLoadReq", state != Qt::Unchecked);
	g_config->save();
}
//-----------------------------------------------------------------------------
void NetPlayHostDialog::enforceAppVersionChkChanged(int state)
{
	g_config->setOption("SDL.NetPlayHostEnforceAppVersionChk", state != Qt::Unchecked);
	g_config->save();
}
//-----------------------------------------------------------------------------
void NetPlayHostDialog::onStartClicked(void)
{
	NetPlayServer *server = NetPlayServer::GetInstance();

	if (server != nullptr)
	{
		delete server;
		server = nullptr;
	}
	NetPlayServer::Create(consoleWindow);

	const int netPort = portEntry->value();
	server = NetPlayServer::GetInstance();
	server->setRole( playerRoleBox->currentData().toInt() );
	server->sessionName = sessionNameEntry->text();
	server->setMaxLeadFrames( frameLeadSpinBox->value() );
	server->setEnforceAppVersionCheck( enforceAppVersionChkCBox->isChecked() );
	server->setAllowClientRomLoadRequest( allowClientRomReqCBox->isChecked() );
	server->setAllowClientStateLoadRequest( allowClientStateReqCBox->isChecked() );
	server->setDebugMode( debugModeCBox->isChecked() );

	if (passwordRequiredCBox->isChecked())
	{
		server->sessionPasswd = passwordEntry->text();
	}
	bool listenSucceeded = server->listen( QHostAddress::Any, netPort );

	if (listenSucceeded)
	{
		g_config->setOption("SDL.NetworkPort", netPort);
		done(0);
		deleteLater();
	}
	else
	{
		QString msg = "Failed to start TCP server on port ";
	       	msg += QString::number(netPort);
		msg += "\n\nReason: ";
	       	msg += server->errorString();
		QMessageBox::warning( this, tr("TCP Server Error"), msg, QMessageBox::Ok );

		// Server init failed, destruct server
		if (server != nullptr)
		{
			delete server;
			server = nullptr;
		}
	}
}
//-----------------------------------------------------------------------------
//--- NetPlayJoinDialog
//-----------------------------------------------------------------------------
NetPlayJoinDialog* NetPlayJoinDialog::instance = nullptr;
//-----------------------------------------------------------------------------
NetPlayJoinDialog::NetPlayJoinDialog(QWidget *parent)
	: QDialog(parent)
{
	QVBoxLayout *mainLayout;
	QHBoxLayout *hbox;
	QGridLayout *grid;
	QPushButton *cancelButton, *startButton;
	QLabel *lbl;

	instance = this;

	setWindowTitle("NetPlay Join Game");

	mainLayout = new QVBoxLayout();
	grid = new QGridLayout();

	lbl = new QLabel( tr("Host:") );
	grid->addWidget( lbl, 0, 0 );

	QString hostAddress = "localhost";
	g_config->getOption("SDL.NetworkIP", &hostAddress);
	if (hostAddress.isEmpty())
	{
		hostAddress = "localhost";
	}
	hostEntry = new QLineEdit();
	hostEntry->setText(hostAddress);
	grid->addWidget( hostEntry, 0, 1 );

	lbl = new QLabel( tr("Port:") );
	grid->addWidget( lbl, 1, 0 );

	int netPort = NetPlayServer::DefaultPort;
	g_config->getOption("SDL.NetworkPort", &netPort);
	portEntry = new QSpinBox();
	portEntry->setRange(0,65535);
	portEntry->setValue(netPort);
	grid->addWidget( portEntry, 1, 1 );

	lbl = new QLabel( tr("Role:") );
	grid->addWidget( lbl, 2, 0 );

	playerRoleBox = new QComboBox();
	playerRoleBox->addItem( tr("Spectator"), NETPLAY_SPECTATOR);
	playerRoleBox->addItem( tr("Player 1") , NETPLAY_PLAYER1  );
	playerRoleBox->addItem( tr("Player 2") , NETPLAY_PLAYER2  );
	playerRoleBox->addItem( tr("Player 3") , NETPLAY_PLAYER3  );
	playerRoleBox->addItem( tr("Player 4") , NETPLAY_PLAYER4  );
	playerRoleBox->setCurrentIndex(2);
	grid->addWidget( playerRoleBox, 2, 1 );

	lbl = new QLabel( tr("User:") );
	grid->addWidget( lbl, 3, 0 );

	QString name;
	g_config->getOption("SDL.NetworkUsername", &name);

	userNameEntry = new QLineEdit();
	userNameEntry->setMaxLength(63);
	userNameEntry->setText(name);
	grid->addWidget( userNameEntry, 3, 1 );

	lbl = new QLabel( tr("Password:") );
	grid->addWidget( lbl, 4, 0 );

	passwordEntry = new QLineEdit();
	passwordEntry->setMaxLength(64);
	passwordEntry->setEnabled(false);
	grid->addWidget( passwordEntry, 4, 1 );

	mainLayout->addLayout(grid);

	startButton = new QPushButton( tr("Join") );
	startButton->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));
	connect(startButton, SIGNAL(clicked(void)), this, SLOT(onJoinClicked(void)));

	cancelButton = new QPushButton( tr("Cancel") );
	cancelButton->setIcon(style()->standardIcon(QStyle::SP_DialogCancelButton));
	connect(cancelButton, SIGNAL(clicked(void)), this, SLOT(closeWindow(void)));

	hbox = new QHBoxLayout();
	hbox->addWidget( cancelButton, 1 );
	hbox->addStretch(5);
	hbox->addWidget( startButton, 1 );
	mainLayout->addLayout( hbox );

	setLayout(mainLayout);
}
//----------------------------------------------------------------------------
NetPlayJoinDialog::~NetPlayJoinDialog(void)
{
	instance = nullptr;
	//printf("Destroy NetPlay Host Window\n");
}
//----------------------------------------------------------------------------
void NetPlayJoinDialog::closeEvent(QCloseEvent *event)
{
	//printf("NetPlay Host Close Window Event\n");
	done(0);
	deleteLater();
	event->accept();
}
//----------------------------------------------------------------------------
void NetPlayJoinDialog::closeWindow(void)
{
	//printf("Close Window\n");
	done(0);
	deleteLater();
}
//----------------------------------------------------------------------------
void NetPlayJoinDialog::onJoinClicked(void)
{
	NetPlayClient *client = NetPlayClient::GetInstance();

	if (client != nullptr)
	{
		delete client;
		client = nullptr;
	}
	NetPlayClient::Create(consoleWindow);

	const int netPort = portEntry->value();
	client = NetPlayClient::GetInstance();
	client->role = playerRoleBox->currentData().toInt();
	client->userName = userNameEntry->text();
	client->password = passwordEntry->text();

	QString hostAddress = hostEntry->text();

	connect(client, SIGNAL(connected(void))   , this, SLOT(onConnect(void)));
	connect(client, SIGNAL(errorOccurred(const QString&)), this, SLOT(onSocketError(const QString&)));

	if (client->connectToHost( hostAddress, netPort ))
	{
		FCEU_DispMessage("Host connect failed",0);
	}
	g_config->setOption("SDL.NetworkIP", hostAddress);
	g_config->setOption("SDL.NetworkPort", netPort);

}
//----------------------------------------------------------------------------
void NetPlayJoinDialog::onConnect()
{
	//printf("Close Window\n");
	done(0);
	deleteLater();
}
//----------------------------------------------------------------------------
void NetPlayJoinDialog::onSocketError(const QString& errorMsg)
{
	QString msg = "Failed to connect to server";
	msg += "\n\nReason: ";
	msg += errorMsg;
	QMessageBox::warning( this, tr("Connection Error"), msg, QMessageBox::Ok );

	NetPlayClient *client = NetPlayClient::GetInstance();

	if (client != nullptr)
	{
		delete client;
		client = nullptr;
	}
}
//-----------------------------------------------------------------------------
//--- NetPlayClientStatusDialog
//-----------------------------------------------------------------------------
NetPlayClientStatusDialog* NetPlayClientStatusDialog::instance = nullptr;
//-----------------------------------------------------------------------------
NetPlayClientStatusDialog::NetPlayClientStatusDialog(QWidget *parent)
	: QDialog(parent)
{
	QVBoxLayout *mainLayout;
	QHBoxLayout *hbox;
	QGridLayout *grid;
	QGroupBox   *gbox;
	QPushButton *closeButton;
	//QLabel *lbl;

	instance = this;
	mainLayout = new QVBoxLayout();
	grid       = new QGridLayout();
	gbox       = new QGroupBox(tr("Connection"));

	gbox->setLayout(grid);
	mainLayout->addWidget(gbox);

	hostStateLbl = new QLabel(tr("Unknown"));
	grid->addWidget( new QLabel(tr("Host Frame:")), 0, 0 );
	grid->addWidget( hostStateLbl, 0, 1 );

	requestResyncButton = new QPushButton(tr("Resync State"));
	grid->addWidget( requestResyncButton, 1, 0, 1, 2 );

	hbox = new QHBoxLayout();
	mainLayout->addLayout(hbox);

	closeButton = new QPushButton( tr("Close") );
	closeButton->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
	connect(closeButton, SIGNAL(clicked(void)), this, SLOT(closeWindow(void)));

	hbox->addStretch(3);
	hbox->addWidget(closeButton);

	setWindowTitle("NetPlay Status");
	//resize( 512, 256 );

	setLayout(mainLayout);

	connect(requestResyncButton, SIGNAL(clicked(void)), this, SLOT(resyncButtonClicked(void)));

	periodicTimer = new QTimer(this);
	periodicTimer->start(200); // 5hz
	connect(periodicTimer, &QTimer::timeout, this, &NetPlayClientStatusDialog::updatePeriodic);
}
//----------------------------------------------------------------------------
NetPlayClientStatusDialog::~NetPlayClientStatusDialog(void)
{
	instance = nullptr;
	periodicTimer->stop();
	delete periodicTimer;
	//printf("Destroy NetPlay Status Window\n");
}
//----------------------------------------------------------------------------
void NetPlayClientStatusDialog::closeEvent(QCloseEvent *event)
{
	//printf("NetPlay Client Close Window Event\n");
	done(0);
	deleteLater();
	event->accept();
}
//----------------------------------------------------------------------------
void NetPlayClientStatusDialog::closeWindow(void)
{
	//printf("Close Window\n");
	done(0);
	deleteLater();
}
//----------------------------------------------------------------------------
void NetPlayClientStatusDialog::updatePeriodic()
{
	updateStatusDisplay();
}
//----------------------------------------------------------------------------
void NetPlayClientStatusDialog::updateStatusDisplay()
{
	NetPlayClient* client = NetPlayClient::GetInstance();

	if (client == nullptr)
	{
		return;
	}
	char stmp[64];
	uint32_t inputFrame = client->inputFrameBack();

	if (inputFrame == 0)
	{
		inputFrame = static_cast<uint32_t>(currFrameCounter);
	}
	snprintf( stmp, sizeof(stmp), "%u", inputFrame);
	hostStateLbl->setText(tr(stmp));
}
//----------------------------------------------------------------------------
void NetPlayClientStatusDialog::resyncButtonClicked()
{
	NetPlayClient* client = NetPlayClient::GetInstance();

	if (client == nullptr)
	{
		return;
	}
	client->requestSync();
}
//-----------------------------------------------------------------------------
//--- NetPlayHostStatusDialog
//-----------------------------------------------------------------------------
NetPlayHostStatusDialog* NetPlayHostStatusDialog::instance = nullptr;
//-----------------------------------------------------------------------------
NetPlayHostStatusDialog::NetPlayHostStatusDialog(QWidget *parent)
	: QDialog(parent)
{
	QVBoxLayout *mainLayout, *vbox1;
	QHBoxLayout *hbox;
	//QGridLayout *grid;
	QGroupBox   *gbox;
	QPushButton *closeButton;
	//QLabel *lbl;

	instance = this;

	setWindowTitle("NetPlay Status");
	resize( 512, 256 );

	mainLayout = new QVBoxLayout();
	vbox1      = new QVBoxLayout();

	gbox       = new QGroupBox(tr("Connected Players / Spectators"));
	clientTree = new QTreeWidget();
	dropPlayerButton   = new QPushButton( tr("Drop Player") );
	resyncPlayerButton = new QPushButton( tr("Resync Player") );
	resyncAllButton    = new QPushButton( tr("Resync All") );

	dropPlayerButton->setEnabled(false);
	resyncPlayerButton->setEnabled(false);

	gbox->setLayout(vbox1);
	vbox1->addWidget( clientTree );

	hbox = new QHBoxLayout();
	hbox->addWidget( dropPlayerButton, 1 );
	hbox->addStretch(3);
	hbox->addWidget( resyncPlayerButton, 1 );
	hbox->addWidget( resyncAllButton, 1 );
	vbox1->addLayout(hbox);

	mainLayout->addWidget( gbox );

	clientTree->setColumnCount(3);

	auto* item = new QTreeWidgetItem();
	item->setText( 0, QString::fromStdString( "Player" ) );
	item->setText( 1, QString::fromStdString( "Role" ) );
	item->setText( 2, QString::fromStdString( "State" ) );
	item->setText( 3, QString::fromStdString( "Frame" ) );
	item->setTextAlignment( 0, Qt::AlignLeft);
	item->setTextAlignment( 1, Qt::AlignLeft);
	item->setTextAlignment( 2, Qt::AlignLeft);
	item->setTextAlignment( 3, Qt::AlignLeft);

	connect( clientTree, SIGNAL(itemClicked(QTreeWidgetItem*, int)), this, SLOT(clientItemClicked( QTreeWidgetItem*, int)) );

	clientTree->setHeaderItem( item );
	clientTree->setContextMenuPolicy(Qt::CustomContextMenu);

	closeButton = new QPushButton( tr("Close") );
	closeButton->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
	connect(closeButton, SIGNAL(clicked(void)), this, SLOT(closeWindow(void)));

	hbox = new QHBoxLayout();
	hbox->addStretch(5);
	hbox->addWidget( closeButton, 1 );
	mainLayout->addLayout( hbox );

	setLayout(mainLayout);

	connect( NetPlayServer::GetInstance(), SIGNAL(clientConnected(void)), this, SLOT(loadClientTree(void)) );
	connect( NetPlayServer::GetInstance(), SIGNAL(clientDisconnected(void)), this, SLOT(loadClientTree(void)) );
	connect( clientTree, &QTreeWidget::customContextMenuRequested, this, &NetPlayHostStatusDialog::onClientTreeContextMenu);

	connect( dropPlayerButton  , SIGNAL(clicked(void)), this, SLOT(dropPlayer(void)) );
	connect( resyncPlayerButton, SIGNAL(clicked(void)), this, SLOT(resyncPlayer(void)) );
	connect( resyncAllButton   , SIGNAL(clicked(void)), this, SLOT(resyncAllPlayers(void)) );

	loadClientTree();

	periodicTimer = new QTimer(this);
	periodicTimer->start(200); // 5hz
	connect(periodicTimer, &QTimer::timeout, this, &NetPlayHostStatusDialog::updatePeriodic);
}
//----------------------------------------------------------------------------
NetPlayHostStatusDialog::~NetPlayHostStatusDialog(void)
{
	instance = nullptr;
	periodicTimer->stop();
	delete periodicTimer;
	//printf("Destroy NetPlay Status Window\n");
}
//----------------------------------------------------------------------------
void NetPlayHostStatusDialog::closeEvent(QCloseEvent *event)
{
	//printf("NetPlay Host Close Window Event\n");
	done(0);
	deleteLater();
	event->accept();
}
//----------------------------------------------------------------------------
void NetPlayHostStatusDialog::closeWindow(void)
{
	//printf("Close Window\n");
	done(0);
	deleteLater();
}
//----------------------------------------------------------------------------
void NetPlayHostStatusDialog::dropPlayer(void)
{
	auto* clientItem = static_cast<NetPlayClientTreeItem*>(clientTree->currentItem());

	if (clientItem != nullptr)
	{
		clientItem->client->forceDisconnect();
	}
}
//----------------------------------------------------------------------------
void NetPlayHostStatusDialog::resyncPlayer(void)
{
	auto* clientItem = static_cast<NetPlayClientTreeItem*>(clientTree->currentItem());

	if (clientItem != nullptr)
	{
		auto* server = NetPlayServer::GetInstance();

		if ( (server != nullptr) && (clientItem->client != nullptr) )
		{
			server->resyncClient( clientItem->client );
		}
	}
}
//----------------------------------------------------------------------------
void NetPlayHostStatusDialog::resyncAllPlayers(void)
{
	auto* server = NetPlayServer::GetInstance();

	if (server != nullptr)
	{
		server->sendPauseAll();
		server->resyncAllClients();
	}
}
//----------------------------------------------------------------------------
void NetPlayHostStatusDialog::clientItemClicked( QTreeWidgetItem* item, int column)
{
	auto* clientItem = static_cast<NetPlayClientTreeItem*>(item);

	bool hasClient = clientItem->client != nullptr;

	dropPlayerButton->setEnabled( hasClient );
	resyncPlayerButton->setEnabled( hasClient );
}
//----------------------------------------------------------------------------
void NetPlayHostStatusDialog::onClientTreeContextMenu(const QPoint &pos)
{
	QAction *act;
	QMenu menu(this);

	printf("Custom Menu (%i,%i)\n", pos.x(), pos.y());

	act = new QAction(tr("Resync Client"), &menu);
	//act->setShortcut( QKeySequence(tr("R")));
	//connect( act, SIGNAL(triggered(void)), this, SLOT(openTileEditor(void)) );
	menu.addAction( act );

	act = new QAction(tr("Drop Client"), &menu);
	//act->setShortcut( QKeySequence(tr("D")));
	//connect( act, SIGNAL(triggered(void)), this, SLOT(openTileEditor(void)) );
	menu.addAction( act );

	menu.exec( clientTree->mapToGlobal(pos) );
}
//----------------------------------------------------------------------------
void NetPlayClientTreeItem::updateData()
{
	const char *roleString = nullptr;

	if (server != nullptr)
	{
		QString state;
		roleString = NetPlayPlayerRoleToString( server->getRole() );

		if (FCEUI_EmulationPaused())
		{
			state = QObject::tr("Paused");
		}
		else if (server->waitingOnClients())
		{
			state = QObject::tr("Waiting");
		}
		else
		{
			state = QObject::tr("Running");
		}

		setText( 0, QObject::tr("Host") );
		setText( 1, QObject::tr(roleString) );
		setText( 2, state);
		setText( 3, QString::number(static_cast<uint32_t>(currFrameCounter)));

	}
	else if (client != nullptr)
	{
		QString state;
		uint32_t currFrame = currFrameCounter;

		bool hasInput = (client->readyFrame > client->currentFrame) &&
			        (client->readyFrame <= currFrame);

		roleString = NetPlayPlayerRoleToString( client->role );

		if (client->isPaused())
		{
			state = QObject::tr("Paused");
		}
		else if (!hasInput)
		{
			state = QObject::tr("Waiting");
		}
		else
		{
			state = QObject::tr("Running");
		}
		if (client->hasDesync())
		{
			state += QObject::tr(",Desync");
		}
		if (!client->romMatch)
		{
			state += QObject::tr(",ROM Mismatch");
		}

		setText( 0, client->userName );
		setText( 1, QObject::tr(roleString) );
		setText( 2, state);
		setText( 3, QString::number(client->currentFrame) );

		if (isExpanded())
		{
			FCEU::FixedString<256> infoLine;
			const int numChildren = childCount();

			//printf("Calculating Expanded Item\n");

			for (int i=0; i<numChildren; i++)
			{
				auto* item = static_cast<NetPlayClientTreeItem*>( child(i) );
				switch (item->type)
				{
					case NetPlayClientTreeItem::PingInfo:
					{
						infoLine.sprintf("Ping ms:     Avg %.2f     Min: %llu     Max: %llu", 
								client->getAvgPingDelay(), client->getMinPingDelay(), client->getMaxPingDelay() );
						item->setFirstColumnSpanned(true);
						item->setText( 0, QObject::tr(infoLine.c_str()) );
					}
					break;
					case NetPlayClientTreeItem::DesyncInfo:
					{
						infoLine.sprintf("Desync Count:  Since Last Sync: %u    Total: %u",
							     client->desyncSinceReset, client->totalDesyncCount );
						item->setFirstColumnSpanned(true);
						item->setText( 0, QObject::tr(infoLine.c_str()) );
					}
					break;
					default:
					break;
				}
			}
		}
	}
}
//----------------------------------------------------------------------------
void NetPlayHostStatusDialog::updatePeriodic()
{
	for (int i=0; i<clientTree->topLevelItemCount(); i++)
	{
		NetPlayClientTreeItem *item = static_cast<NetPlayClientTreeItem*>( clientTree->topLevelItem(i) );

		item->updateData();
	}
}
//----------------------------------------------------------------------------
void NetPlayHostStatusDialog::loadClientTree()
{
	auto* server = NetPlayServer::GetInstance();

	//printf("Load Client Connection Tree\n");

	clientTree->clear();

	if (server == nullptr)
	{
		return;
	}
	QFont font;

	font.setFamily("Courier New");
	font.setStyle( QFont::StyleNormal );
	font.setStyleHint( QFont::Monospace );

	auto* serverTopLvlItem = new NetPlayClientTreeItem();

	serverTopLvlItem->server = server;
	serverTopLvlItem->updateData();

	serverTopLvlItem->setFont( 0, font );
	serverTopLvlItem->setFont( 1, font );
	serverTopLvlItem->setFont( 2, font );
	serverTopLvlItem->setFont( 3, font );

	clientTree->addTopLevelItem( serverTopLvlItem );

	auto& clientList = server->getClientList();

	for (auto& client : clientList)
	{
		auto* clientTopLvlItem = new NetPlayClientTreeItem();

		clientTopLvlItem->client = client;
		clientTopLvlItem->updateData();

		auto* clientPingInfo = new NetPlayClientTreeItem();
		clientPingInfo->setFont( 0, font );
		clientPingInfo->type = NetPlayClientTreeItem::PingInfo;
		clientPingInfo->setFirstColumnSpanned(true);
		clientTopLvlItem->addChild( clientPingInfo );

		auto* clientDesyncInfo = new NetPlayClientTreeItem();
		clientDesyncInfo->setFont( 0, font );
		clientDesyncInfo->type = NetPlayClientTreeItem::DesyncInfo;
		clientDesyncInfo->setFirstColumnSpanned(true);
		clientTopLvlItem->addChild( clientDesyncInfo );

		clientTopLvlItem->setFont( 0, font );
		clientTopLvlItem->setFont( 1, font );
		clientTopLvlItem->setFont( 2, font );
		clientTopLvlItem->setFont( 3, font );

		clientTree->addTopLevelItem( clientTopLvlItem );
	}

}
//----------------------------------------------------------------------------
//---- Global Functions
//----------------------------------------------------------------------------
bool NetPlayActive(void)
{
	return (NetPlayClient::GetInstance() != nullptr) || (NetPlayServer::GetInstance() != nullptr);
}
//----------------------------------------------------------------------------
bool isNetPlayHost(void)
{
	return (NetPlayServer::GetInstance() != nullptr);
}
//----------------------------------------------------------------------------
bool isNetPlayClient(void)
{
	return (NetPlayClient::GetInstance() != nullptr);
}
//----------------------------------------------------------------------------
void NetPlayPeriodicUpdate(void)
{
	NetPlayClient *client = NetPlayClient::GetInstance();

	if (client)
	{
		client->update();

		if ( client->shouldDestroy() )
		{
			NetPlayClient::Destroy();
			client = nullptr;	
		}
	}

	NetPlayServer *server = NetPlayServer::GetInstance();

	if (server)
	{
		server->update();
	}
}
//----------------------------------------------------------------------------
static NetPlayFrameInput netPlayInputFrame;
//----------------------------------------------------------------------------
int NetPlayFrameWait(void)
{
	int wait = 0;
	NetPlayClient *client = NetPlayClient::GetInstance();

	if (client)
	{
		wait = client->inputAvailable() == 0;
	}
	else
	{
		NetPlayServer *server = NetPlayServer::GetInstance();

		if (server)
		{
			wait = server->inputAvailable() == 0;
		}
	}
	return wait;
}
//----------------------------------------------------------------------------
bool NetPlaySkipWait(void)
{
	bool skip = false;

	NetPlayClient *client = NetPlayClient::GetInstance();

	if (client)
	{
		skip = client->inputAvailableCount() > client->catchUpThreshold;
	}
	return skip;
}
//----------------------------------------------------------------------------
void NetPlayReadInputFrame(uint8_t* joy)
{
	NetPlayClient *client = NetPlayClient::GetInstance();

	if (client)
	{
		netPlayInputFrame = client->getNextInput();
	}
	else
	{
		NetPlayServer *server = NetPlayServer::GetInstance();

		if (server)
		{
			NetPlayFrameInput frameInput;
			netPlayInputFrame = server->getNextInput();
		}
	}
	joy[0] = netPlayInputFrame.ctrl[0];
	joy[1] = netPlayInputFrame.ctrl[1];
	joy[2] = netPlayInputFrame.ctrl[2];
	joy[3] = netPlayInputFrame.ctrl[3];

	NetPlayOnFrameBegin();
}
//----------------------------------------------------------------------------
void NetPlayCloseSession(void)
{
	NetPlayClient::Destroy();
	NetPlayServer::Destroy();
}
//----------------------------------------------------------------------------
template <typename T> void openSingletonDialog(QWidget* parent)
{
	T* win = T::GetInstance();

	if (win == nullptr)
	{
		win = new T(parent);

		win->show();
	}
	else
	{
		win->activateWindow();
		win->raise();
	}
}
//----------------------------------------------------------------------------
void openNetPlayHostDialog(QWidget* parent)
{
	openSingletonDialog<NetPlayHostDialog>(parent);
}
//----------------------------------------------------------------------------
void openNetPlayJoinDialog(QWidget* parent)
{
	openSingletonDialog<NetPlayJoinDialog>(parent);
}
//----------------------------------------------------------------------------
void openNetPlayHostStatusDialog(QWidget* parent)
{
	openSingletonDialog<NetPlayHostStatusDialog>(parent);
}
//----------------------------------------------------------------------------
void openNetPlayClientStatusDialog(QWidget* parent)
{
	openSingletonDialog<NetPlayClientStatusDialog>(parent);
}
//----------------------------------------------------------------------------
//----  Network Byte Swap Utilities
//----------------------------------------------------------------------------
#ifdef __BIG_ENDIAN__

	#define NETPLAY_HOST_BE  1
	#define NETPLAY_HOST_LE  0

#elif defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)

	#define NETPLAY_HOST_BE  1
	#define NETPLAY_HOST_LE  0

#else

	#define NETPLAY_HOST_BE  0
	#define NETPLAY_HOST_LE  1
#endif
//----------------------------------------------------------------------------
uint16_t netPlayByteSwap(uint16_t in)
{
	uint16_t out = in;
#if  NETPLAY_HOST_LE
	unsigned char *in8  = (unsigned char*)&in;
	unsigned char *out8 = (unsigned char*)&out;
	out8[0] = in8[1];
	out8[1] = in8[0];
#endif
	return out;
}
//----------------------------------------------------------------------------
uint32_t netPlayByteSwap(uint32_t in)
{
	uint32_t out = in;
#if  NETPLAY_HOST_LE
	unsigned char *in8  = (unsigned char*)&in;
	unsigned char *out8 = (unsigned char*)&out;
	out8[0] = in8[3];
	out8[1] = in8[2];
	out8[2] = in8[1];
	out8[3] = in8[0];
#endif
	return out;
}
//----------------------------------------------------------------------------
uint64_t netPlayByteSwap(uint64_t in)
{
	uint64_t out = in;
#if  NETPLAY_HOST_LE
	unsigned char *in8  = (unsigned char*)&in;
	unsigned char *out8 = (unsigned char*)&out;
	out8[0] = in8[7];
	out8[1] = in8[6];
	out8[2] = in8[5];
	out8[3] = in8[4];
	out8[4] = in8[3];
	out8[5] = in8[2];
	out8[6] = in8[1];
	out8[7] = in8[0];
#endif
	return out;
}
//----------------------------------------------------------------------------
uint32_t netPlayCalcRamChkSum()
{
	constexpr int ramSize = 0x800;
	uint32_t crc = 0;
	uint8_t ram[ramSize];

	for (int i=0; i<ramSize; i++)
	{
		ram[i] = GetMem(i);
	}
	crc = CalcCRC32( crc, ram, sizeof(ram));

	return crc;
}
//----------------------------------------------------------------------------
void NetPlayTraceInstruction(uint8_t *opcode, int size)
{
	opsCrc32 = CalcCRC32( opsCrc32, opcode, size);
}
//----------------------------------------------------------------------------
void NetPlayOnFrameBegin()
{
	NetPlayFrameData data;

	data.frameNum = static_cast<uint32_t>(currFrameCounter);
	data.opsCrc32 = opsCrc32;
	data.ramCrc32 = netPlayCalcRamChkSum();

	netPlayFrameData.push( data );

	//printf("Frame: %u   Ops:%08X  Ram:%08X\n", data.frameNum, data.opsCrc32, data.ramCrc32 );
}
//----------------------------------------------------------------------------
bool NetPlayStateLoadReq(EMUFILE* is)
{
	auto* client = NetPlayClient::GetInstance();

	bool shouldLoad = (client == nullptr) || serverRequestedStateLoad;

	printf("NetPlay Load State: %i\n", shouldLoad);

	if ( (client != nullptr) && !serverRequestedStateLoad)
	{  // Send state to host
	   client->requestStateLoad(is);
	}
	return !shouldLoad;
}
//----------------------------------------------------------------------------
