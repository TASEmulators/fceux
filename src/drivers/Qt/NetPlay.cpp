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
		if (bufHead == 0)
		{
			i = numFrames - 1;
		}
		else
		{
			i = bufHead - 1;
		}
		out = data[i];

		return 0;
	}

	int find( uint32_t frame, NetPlayFrameData& out )
	{
		int i, retval = -1;

		if (bufHead == 0)
		{
			i = numFrames - 1;
		}
		else
		{
			i = bufHead - 1;
		}

		while (i != bufHead)
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
//--- NetPlayServer
//-----------------------------------------------------------------------------
NetPlayServer *NetPlayServer::instance = nullptr;

NetPlayServer::NetPlayServer(QObject *parent)
	: QTcpServer(parent)
{
	instance = this;

	connect(this, SIGNAL(newConnection(void)), this, SLOT(newConnectionRdy(void)));
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

		msg.toNetworkByteOrder();
		sendMsg( client, &msg, sizeof(msg) );
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

	return 0;
}

//-----------------------------------------------------------------------------
int NetPlayServer::sendMsg( NetPlayClient *client, void *msg, size_t msgSize)
{
	QTcpSocket* sock;

	sock = client->getSocket();

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

	msg.toNetworkByteOrder();
	sendMsg( client, &msg, sizeof(netPlayLoadRomReq) );

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

	hdr.toNetworkByteOrder();
	sendMsg( client, &hdr, sizeof(netPlayMsgHdr) );
	sendMsg( client, em.buf(), em.size() );

	opsCrc32 = 0;

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
		}
	}
	return success;
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
					netPlayErrorMsg<128>  errorMsg;
					errorMsg.setDisconnectFlag();
					errorMsg.printf("Invalid Password");
					errorMsg.toNetworkByteOrder();
					sendMsg( client, &errorMsg, errorMsg.hdr.msgSize );
					client->flushData();
				}
			}

			if (authentication_passed)
			{
				if ( claimRole(client, msg->playerId) )
				{
					client->userName = msg->userName;
					sendRomLoadReq( client );
					sendStateSyncReq( client );
					client->state = 1;
					FCEU_DispMessage("%s Joined",0, client->userName.toLocal8Bit().constData());
				}
				else
				{
					netPlayErrorMsg<128>  errorMsg;
					errorMsg.setDisconnectFlag();
					errorMsg.printf("Player %i role is not available", msg->playerId+1);
					errorMsg.toNetworkByteOrder();
					sendMsg( client, &errorMsg, errorMsg.hdr.msgSize );
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
			client->gpData[0] = msg->ctrlState[0];
			client->gpData[1] = msg->ctrlState[1];
			client->gpData[2] = msg->ctrlState[2];
			client->gpData[3] = msg->ctrlState[3];

			client->setPaused( (msg->flags & netPlayClientState::PAUSE_FLAG) ? true : false );

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
						sendStateSyncReq( client );

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
		default:
			printf("Unknown Msg: %08X\n", msgId);
		break;
	}

}
//-----------------------------------------------------------------------------
void NetPlayServer::update(void)
{
	bool shouldRunFrame = false;
	unsigned int clientMinFrame = 0xFFFFFFFF;
	unsigned int clientMaxFrame = 0;
	const uint32_t maxLead = 5u;
	const uint32_t currFrame = static_cast<uint32_t>(currFrameCounter);
	const uint32_t leadFrame = currFrame + maxLead;
	const uint32_t lastFrame = inputFrameBack();
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
			delete client;
		}
		else
		{
			it++;
		}
	}

	shouldRunFrame = (clientMinFrame != 0xFFFFFFFF) && 
		(clientMinFrame >= lagFrame ) &&
		(clientMaxFrame  < leadFrame) &&
		(currFrame > lastFrame) && (numClientsPaused == 0);

	//printf("Client Frame: Min:%u  Max:%u\n", clientMinFrame, clientMaxFrame);

	if (shouldRunFrame)
	{
		// Output Processing
		NetPlayFrameInput  inputFrame;
		netPlayRunFrameReq  runFrameReq;

		inputFrame.frameCounter = static_cast<uint32_t>(currFrameCounter) + 1;
		inputFrame.ctrl[0] = gpData[0];
		inputFrame.ctrl[1] = gpData[1];
		inputFrame.ctrl[2] = gpData[2];
		inputFrame.ctrl[3] = gpData[3];

		runFrameReq.frameNum     = inputFrame.frameCounter;
		runFrameReq.ctrlState[0] = inputFrame.ctrl[0];
		runFrameReq.ctrlState[1] = inputFrame.ctrl[1];
		runFrameReq.ctrlState[2] = inputFrame.ctrl[2];
		runFrameReq.ctrlState[3] = inputFrame.ctrl[3];

		pushBackInput( inputFrame );

		runFrameReq.toNetworkByteOrder();

		for (auto it = clientList.begin(); it != clientList.end(); it++)
		{
			NetPlayClient *client = *it;

			if (client->state > 0)
			{
				sendMsg( client, &runFrameReq, sizeof(runFrameReq) );
			}
		}
	}
	
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
	FCEU_DispMessage("Socket Error",0);

	QString errorMsg = sock->errorString();
	printf("Error: %s\n", errorMsg.toLocal8Bit().constData());

	FCEU_DispMessage("%s", 0, errorMsg.toLocal8Bit().constData());

	emit errorOccurred(errorMsg);
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
	}
}
//-----------------------------------------------------------------------------
int NetPlayClient::readMessages( void (*msgCallback)( void *userData, void *msgBuf, size_t msgSize ), void *userData )
{
	if (sock)
	{
		bool readReq;
		netPlayMsgHdr *hdr;
		const int netPlayMsgHdrSize = sizeof(netPlayMsgHdr);

		readReq = sock->bytesAvailable() > 0;

		while (readReq)
		{
			if (recvMsgBytesLeft > 0)
			{
				readReq = (sock->bytesAvailable() >= recvMsgBytesLeft);

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
						readReq = (sock->bytesAvailable() >= recvMsgBytesLeft);
					}
					else
					{
						msgCallback( userData, recvMsgBuf, recvMsgSize );
						readReq = (sock->bytesAvailable() > 0);
					}
				}
			}
			else if (sock->bytesAvailable() >= netPlayMsgHdrSize)
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
				readReq = (sock->bytesAvailable() >= recvMsgSize);
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
			auto *msg = static_cast<netPlayErrorMsg<256>*>(msgBuf);
			msg->toHostByteOrder();
			printf("Error: 0x%X  %s\n", msg->code, msg->getBuffer());

			if (msg->isDisconnectFlagSet())
			{
				sock->disconnectFromHost();
			}
			FCEU_DispMessage("Host connect failed",0);
		}
		break;
		case NETPLAY_AUTH_REQ:
		{
			netPlayAuthResp msg;
			msg.playerId = role;
			Strlcpy( msg.userName, userName.toLocal8Bit().constData(), sizeof(msg.userName));
			Strlcpy( msg.pswd, password.toLocal8Bit().constData(), sizeof(msg.pswd) );

			printf("Authentication Request Received\n");
			msg.toNetworkByteOrder();
			sock->write( (const char*)&msg, sizeof(netPlayAuthResp) );
		}
		break;
		case NETPLAY_LOAD_ROM_REQ:
		{
			FILE *fp;
			std::string filepath = QDir::tempPath().toStdString(); 
			netPlayLoadRomReq *msg = static_cast<netPlayLoadRomReq*>(msgBuf);
			msg->toHostByteOrder();
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
			LoadGame( filepath.c_str(), true );
			FCEUI_SetEmulationPaused(EMULATIONPAUSED_PAUSED);
			FCEU_WRAPPER_UNLOCK();
		}
		break;
		case NETPLAY_SYNC_STATE_RESP:
		{
			char *stateData = &static_cast<char*>(msgBuf)[ sizeof(netPlayMsgHdr) ];

			printf("Sync state Request Received\n");

			EMUFILE_MEMORY em( stateData, msgSize );

			FCEU_WRAPPER_LOCK();
			FCEUSS_LoadFP( &em, SSLOADPARAM_NOBACKUP );
			FCEU_WRAPPER_UNLOCK();

			opsCrc32 = 0;
			netPlayFrameData.reset();
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

			if (inputFrame.frameCounter > inputFrameBack())
			{
				pushBackInput( inputFrame );
			}
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
	QPushButton *cancelButton, *startButton;
	QLabel *lbl;

	instance = this;

	setWindowTitle("NetPlay Host Game");

	mainLayout = new QVBoxLayout();
	grid = new QGridLayout();

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

	mainLayout->addLayout(grid);

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
		skip = client->inputAvailable() > 1;
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
void openNetPlayHostDialog(QWidget* parent)
{
	NetPlayHostDialog* win = NetPlayHostDialog::GetInstance();

	if (win == nullptr)
	{
		win = new NetPlayHostDialog(parent);

		win->show();
	}
	else
	{
		win->activateWindow();
		win->raise();
	}
}
//----------------------------------------------------------------------------
void openNetPlayJoinDialog(QWidget* parent)
{
	NetPlayJoinDialog* win = NetPlayJoinDialog::GetInstance();

	if (win == nullptr)
	{
		win = new NetPlayJoinDialog(parent);

		win->show();
	}
	else
	{
		win->activateWindow();
		win->raise();
	}
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
