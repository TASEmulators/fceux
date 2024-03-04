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
#include <QMessageBox>

#include "../../fceu.h"
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
	connect(consoleWindow, SIGNAL(nesResetOccurred(void)), this, SLOT(onNesReset(void)));

	FCEU_WRAPPER_LOCK();
	inputFrameCount = static_cast<uint32_t>(currFrameCounter);
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

		client->setSocket(newSock);

		clientList.push_back(client);

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
int NetPlayServer::sendMsg( NetPlayClient *client, void *msg, size_t msgSize, std::function<void(void)> netByteOrderConvertFunc)
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
	constexpr size_t BufferSize = 64 * 1024;
	char buf[BufferSize];
	size_t bytesRead;
	long fileSize = 0;
	netPlayLoadRomReq msg;

	const char *filepath = nullptr;

	if ( GameInfo )
	{
		printf("filename: '%s' \n", GameInfo->filename );
		printf("archiveFilename: '%s' \n", GameInfo->archiveFilename );

		if (GameInfo->archiveFilename)
		{
			filepath = GameInfo->archiveFilename;
		}
		else
		{
			filepath = GameInfo->filename;
		}
	}

	if (filepath == nullptr)
	{
		return -1;
	}
	printf("Prep ROM Load Request: %s \n", filepath );
	FILE *fp = ::fopen( filepath, "r");

	if (fp == nullptr)
	{
		return -1;
	}
	fseek( fp, 0, SEEK_END);

	fileSize = ftell(fp);

	rewind(fp);

	msg.hdr.msgSize += fileSize;
	msg.fileSize     = fileSize;
	Strlcpy( msg.fileName, GameInfo->filename, sizeof(msg.fileName) );

	printf("Sending ROM Load Request: %s  %lu\n", filepath, fileSize );
	FCEUI_SetEmulationPaused(EMULATIONPAUSED_PAUSED);

	sendMsg( client, &msg, sizeof(netPlayLoadRomReq), [&msg]{ msg.toNetworkByteOrder(); } );

	while ( (bytesRead = fread( buf, 1, sizeof(buf), fp )) > 0 )
	{
		sendMsg( client, buf, bytesRead );
	}

	::fclose(fp);

	return 0;
}
//-----------------------------------------------------------------------------
int NetPlayServer::sendStateSyncReq( NetPlayClient *client )
{
	EMUFILE_MEMORY em;
	int compressionLevel = 1;
	netPlayMsgHdr hdr(NETPLAY_SYNC_STATE_RESP);

	if ( GameInfo == nullptr )
	{
		return -1;
	}
	FCEUSS_SaveMS( &em, compressionLevel );

	hdr.msgSize += em.size();

	printf("Sending ROM Sync Request\n");
	FCEUI_SetEmulationPaused(EMULATIONPAUSED_PAUSED);

	sendMsg( client, &hdr, sizeof(netPlayMsgHdr), [&hdr]{ hdr.toNetworkByteOrder(); } );
	sendMsg( client, em.buf(), em.size() );

	opsCrc32 = 0;
	inputClear();

	return 0;
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

	inputClear();
	inputFrameCount = static_cast<uint32_t>(currFrameCounter);

	// New ROM has been loaded by server, signal clients to load and sync
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
	//printf("New ROM Loaded!\n");
	FCEU_WRAPPER_LOCK();

	inputClear();
	inputFrameCount = static_cast<uint32_t>(currFrameCounter);

	// NES Reset has occurred on server, signal clients sync
	for (auto& client : clientList )
	{
		sendStateSyncReq( client );
	}
	FCEU_WRAPPER_UNLOCK();
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

			if (sessionPasswd.isEmpty())
			{
				authentication_passed = true;
			}
			else
			{
				authentication_passed = sessionPasswd.compare(msg->pswd, Qt::CaseSensitive) == 0;

				if (!authentication_passed)
				{
					netPlayTextMsg<128>  errorMsg(NETPLAY_ERROR_MSG);
					errorMsg.setFlag(netPlayTextMsgFlags::DISCONNECT);
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
					sendRomLoadReq( client );
					sendStateSyncReq( client );
					FCEU_WRAPPER_UNLOCK();
					client->state = 1;
					FCEU_DispMessage("%s Joined",0, client->userName.toLocal8Bit().constData());
				}
				else
				{
					netPlayTextMsg<128>  errorMsg(NETPLAY_ERROR_MSG);
					errorMsg.setFlag(netPlayTextMsgFlags::DISCONNECT);
					errorMsg.printf("Player %i role is not available", msg->playerId+1);
					sendMsg( client, &errorMsg, errorMsg.hdr.msgSize, [&errorMsg]{ errorMsg.toNetworkByteOrder(); } );
					client->flushData();
				}
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

			client->setPaused( (msg->flags & netPlayClientState::PAUSE_FLAG ) ? true : false );
			client->setDesync( (msg->flags & netPlayClientState::DESYNC_FLAG) ? true : false );

			NetPlayFrameData data;
			if ( (msg->opsFrame == 0) || netPlayFrameData.find( msg->opsFrame, data ) )
			{
				//printf("Error: Server Could not find data for frame: %u\n", msg->opsFrame );
			}
			else
			{
				bool opsSync   = (data.opsCrc32 == msg->opsChkSum);
				bool ramSync   = (data.ramCrc32 == msg->ramChkSum);

				client->syncOk = opsSync && ramSync;

				if (!client->syncOk)
				{
					printf("Frame:%u  is NOT in Sync: OPS:%i  RAM:%i\n", msg->frameRun, opsSync, ramSync);
					client->desyncCount++;

					if (client->desyncCount > forceResyncCount)
					{
						FCEU_WRAPPER_LOCK();
						sendStateSyncReq( client );
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

			bool acceptRomLoadReq = false;

			if (allowClientRomLoadReq)
			{
				QString msgBoxTxt = tr("Client '") + client->userName + tr("' has requested to load this ROM:\n\n");
	       			msgBoxTxt += tr(msg->fileName) + tr("\n\nDo you want to load it?");
				int ans = QMessageBox::question( consoleWindow, tr("Client ROM Load Request"), msgBoxTxt, QMessageBox::Yes | QMessageBox::No );

				if (ans == QMessageBox::Yes)
				{
					acceptRomLoadReq = true;
				}
			}

			if (acceptRomLoadReq)
			{
				FILE *fp;
				std::string filepath = QDir::tempPath().toLocal8Bit().constData(); 
				const char *romData = &static_cast<const char*>(msgBuf)[ sizeof(netPlayLoadRomReq) ];

				filepath.append( "/" );
				filepath.append( msg->fileName );

				printf("Load ROM Request Received: %s\n", filepath.c_str());

				//printf("Dumping Temp Rom to: %s\n", filepath.c_str());
				fp = ::fopen( filepath.c_str(), "w");

				if (fp == nullptr)
				{
					return;
				}
				::fwrite( romData, 1, msgSize, fp );
				::fclose(fp);

				FCEU_WRAPPER_LOCK();
				LoadGame( filepath.c_str(), true, true );
				FCEUI_SetEmulationPaused(EMULATIONPAUSED_PAUSED);
				FCEU_WRAPPER_UNLOCK();

				resyncAllClients();
			}
			else
			{
				netPlayTextMsg<128>  errorMsg(NETPLAY_ERROR_MSG);
				errorMsg.printf("Host is rejected ROMs load request");
				sendMsg( client, &errorMsg, errorMsg.hdr.msgSize, [&errorMsg]{ errorMsg.toNetworkByteOrder(); } );
			}
		}
		default:
			printf("Unknown Msg: %08X\n", msgId);
		break;
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
	if (outGoing)
	{
		instance = this;
	}

	recvMsgBuf = static_cast<char*>( ::malloc( recvMsgBufSize ) );

	if (recvMsgBuf == nullptr)
	{
		printf("Error: NetPlayClient failed to allocate recvMsgBuf\n");
	}
}


NetPlayClient::~NetPlayClient(void)
{
	if (instance == this)
	{
		instance = nullptr;
	}

	if (sock != nullptr)
	{
		sock->close();
		delete sock; sock = nullptr;
	}
	if (recvMsgBuf)
	{
		::free(recvMsgBuf); recvMsgBuf = nullptr;
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
	return 0;
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
int NetPlayClient::requestRomLoad( const char *romPath )
{
	constexpr size_t BufferSize = 64 * 1024;
	char buf[BufferSize];
	size_t bytesRead;
	long fileSize = 0;
	netPlayLoadRomReq msg;
	QFileInfo fi( romPath );

	printf("Prep ROM Load Request: %s \n", romPath );
	FILE *fp = ::fopen( romPath, "r");

	if (fp == nullptr)
	{
		return -1;
	}
	fseek( fp, 0, SEEK_END);

	fileSize = ftell(fp);

	rewind(fp);

	msg.hdr.msgSize += fileSize;
	msg.fileSize     = fileSize;
	Strlcpy( msg.fileName, fi.fileName().toLocal8Bit().constData(), sizeof(msg.fileName) );

	printf("Sending ROM Load Request: %s  %lu\n", romPath, fileSize );
	FCEUI_SetEmulationPaused(EMULATIONPAUSED_PAUSED);

	msg.toNetworkByteOrder();
	sock->write( reinterpret_cast<const char*>(&msg), sizeof(netPlayLoadRomReq) );

	while ( (bytesRead = fread( buf, 1, sizeof(buf), fp )) > 0 )
	{
		sock->write( buf, bytesRead );
	}

	::fclose(fp);

	return 0;
}
//-----------------------------------------------------------------------------
void NetPlayClient::recordPingResult( uint64_t delay_ms )
{
	pingNumSamples++;
	pingDelayLast = delay_ms;
	pingDelaySum += delay_ms;
}
//-----------------------------------------------------------------------------
void NetPlayClient::resetPingData()
{
	pingNumSamples = 0;
	pingDelayLast = 0;
	pingDelaySum = 0;
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
			statusMsg.flags |= netPlayClientState::PAUSE_FLAG;
		}
		if (desyncCount > 0)
		{
			statusMsg.flags |= netPlayClientState::DESYNC_FLAG;
		}
		statusMsg.frameRdy  = inputFrameBack();
		statusMsg.frameRun  = currFrame;
		statusMsg.opsFrame  = lastFrameData.frameNum;
		statusMsg.opsChkSum = lastFrameData.opsCrc32;
		statusMsg.ramChkSum = lastFrameData.ramCrc32;
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
	if (sock)
	{
		bool readReq;
		netPlayMsgHdr *hdr;
		constexpr int netPlayMsgHdrSize = sizeof(netPlayMsgHdr);
		FCEU::timeStampRecord ts;

		ts.readNew();
		int bytesAvailable = sock->bytesAvailable();
		readReq = bytesAvailable > 0;

		//printf("Read Bytes Available: %lu  %i\n", ts.toMilliSeconds(), bytesAvailable);

		while (readReq)
		{
			if (recvMsgBytesLeft > 0)
			{
				bytesAvailable = sock->bytesAvailable();
				readReq = (bytesAvailable >= recvMsgBytesLeft);

				if (readReq)
				{
					int dataRead;
					//size_t readSize = recvMsgBytesLeft;

					dataRead = sock->read( &recvMsgBuf[recvMsgByteIndex], recvMsgBytesLeft );

					if (dataRead > 0)
					{
						recvMsgBytesLeft -= dataRead;
						recvMsgByteIndex += dataRead;
					}
					//printf("   Data: Id: %u   Size: %zu  Read: %i\n", recvMsgId, readSize, dataRead );

					if (recvMsgBytesLeft > 0)
					{
						bytesAvailable = sock->bytesAvailable();
						readReq = (bytesAvailable >= recvMsgBytesLeft);
					}
					else
					{
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

				if (netPlayByteSwap(hdr->msgSize) > recvMsgBufSize)
				{
					printf("Error: Message size too large: %08X\n", recvMsgId);
				}
				//printf("HDR: Id: %u   Size: %u\n", netPlayByteSwap(hdr->msgId), netPlayByteSwap(hdr->msgSize) );

				recvMsgByteIndex = sizeof(netPlayMsgHdr);

				if (recvMsgBytesLeft == 0)
				{
					msgCallback( userData, recvMsgBuf, recvMsgSize );
				}
				bytesAvailable = sock->bytesAvailable();
				readReq = (bytesAvailable >= recvMsgSize);
			}
			else
			{
				readReq = false;
			}
		}
	}

	return 0;
}
//-----------------------------------------------------------------------------
void NetPlayClient::clientProcessMessage( void *msgBuf, size_t msgSize )
{
	netPlayMsgHdr *hdr = (netPlayMsgHdr*)msgBuf;

	const uint32_t msgId = netPlayByteSwap(hdr->msgId);

	switch (msgId)
	{
		case NETPLAY_ERROR_MSG:
		{
			auto *msg = static_cast<netPlayTextMsg<256>*>(msgBuf);
			msg->toHostByteOrder();
			FCEU_printf("NetPlay Error: 0x%X  %s\n", msg->code, msg->getBuffer());

			if (msg->isFlagSet(netPlayTextMsgFlags::DISCONNECT))
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
			FILE *fp;
			std::string filepath = QDir::tempPath().toLocal8Bit().constData(); 
			netPlayLoadRomReq *msg = static_cast<netPlayLoadRomReq*>(msgBuf);
			msg->toHostByteOrder();
			const char *romData = &static_cast<const char*>(msgBuf)[ sizeof(netPlayLoadRomReq) ];

			filepath.append( "/" );
			filepath.append( msg->fileName );

			FCEU_printf("Load ROM Request Received: %s\n", filepath.c_str());

			//printf("Dumping Temp Rom to: %s\n", filepath.c_str());
			fp = ::fopen( filepath.c_str(), "w");

			if (fp == nullptr)
			{
				return;
			}
			::fwrite( romData, 1, msgSize, fp );
			::fclose(fp);

			FCEU_WRAPPER_LOCK();
			LoadGame( filepath.c_str(), true, true );
			FCEUI_SetEmulationPaused(EMULATIONPAUSED_PAUSED);
			FCEU_WRAPPER_UNLOCK();
		}
		break;
		case NETPLAY_SYNC_STATE_RESP:
		{
			char *stateData = &static_cast<char*>(msgBuf)[ sizeof(netPlayMsgHdr) ];

			FCEU_printf("Sync state Request Received\n");

			EMUFILE_MEMORY em( stateData, msgSize );

			FCEU_WRAPPER_LOCK();
			FCEUSS_LoadFP( &em, SSLOADPARAM_NOBACKUP );
			FCEU_WRAPPER_UNLOCK();

			opsCrc32 = 0;
			netPlayFrameData.reset();
			inputClear();
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
		default:
			printf("Unknown Msg: %08X\n", msgId);
		break;
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

	// Misc Settings 
	grid = new QGridLayout();
	settingsGroupBox->setLayout(grid);

	lbl = new QLabel( tr("Max Frame Lead:") );
	frameLeadSpinBox = new QSpinBox();
	frameLeadSpinBox->setRange(5,60);
	frameLeadSpinBox->setValue(30);
	grid->addWidget( lbl, 0, 0, 1, 1 );
	grid->addWidget( frameLeadSpinBox, 0, 1, 1, 1 );

	allowClientRomReqCBox = new QCheckBox(tr("Allow Client ROM Load Requests"));
	grid->addWidget( allowClientRomReqCBox, 1, 0, 1, 2 );

	allowClientStateReqCBox = new QCheckBox(tr("Allow Client State Load Requests"));
	grid->addWidget( allowClientStateReqCBox, 2, 0, 1, 2 );

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
	server->sessionPasswd = passwordEntry->text();
	server->setMaxLeadFrames( frameLeadSpinBox->value() );
	server->setAllowClientRomLoadRequest( allowClientRomReqCBox->isChecked() );
	server->setAllowClientStateLoadRequest( allowClientStateReqCBox->isChecked() );

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
//--- NetPlayJoinDialog
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

		setText( 0, client->userName );
		setText( 1, QObject::tr(roleString) );
		setText( 2, state);
		setText( 3, QString::number(client->currentFrame) );
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
	auto* serverTopLvlItem = new NetPlayClientTreeItem();

	serverTopLvlItem->server = server;
	serverTopLvlItem->updateData();

	clientTree->addTopLevelItem( serverTopLvlItem );

	auto& clientList = server->getClientList();

	for (auto& client : clientList)
	{
		auto* clientTopLvlItem = new NetPlayClientTreeItem();

		clientTopLvlItem->client = client;
		clientTopLvlItem->updateData();

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
	//openSingletonDialog<NetPlayHostStatusDialog>(parent);
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
	uint32_t crc = 0;
	uint8_t ram[256];

	for (int i=0; i<256; i++)
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

	//printf("Frame: %u   Ops:%08X  \n", data.frameNum, data.opsCrc32 );
}
//----------------------------------------------------------------------------
