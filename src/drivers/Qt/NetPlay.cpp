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

#include "../../fceu.h"
#include "../../state.h"
#include "../../movie.h"
#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/input.h"
#include "Qt/NetPlay.h"
#include "Qt/fceuWrapper.h"
#include "Qt/ConsoleWindow.h"
#include "Qt/ConsoleUtilities.h"
#include "Qt/NetPlayMsgDef.h"

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

void NetPlayServer::newConnectionRdy(void)
{
	printf("New Connection Ready!\n");

	processPendingConnections();
}

void NetPlayServer::processPendingConnections(void)
{
	QTcpSocket *newSock;

	newSock = nextPendingConnection();

	while (newSock)
	{
		NetPlayClient *client = new NetPlayClient(this);

		client->setSocket(newSock);

		clientList.push_back(client);

		newSock = nextPendingConnection();

		printf("Added Client: %p   %zu\n", client, clientList.size() );

		netPlayAuthReq msg;

		sendMsg( client, &msg, sizeof(msg) );
	}
}

bool NetPlayServer::removeClient(NetPlayClient *client, bool markForDelete)
{
	bool removed = false;
	std::list <NetPlayClient*>::iterator it;

	it = clientList.begin();
       
	while (it != clientList.end())
	{
		if (client == *it)
		{
			if (markForDelete)
			{
				client->deleteLater();
			}

			it = clientList.erase(it);
		}
		else
		{
			it++;
		}
	}

	for (int i=0; i<4; i++)
	{
		if (clientPlayer[i] == client)
		{
			clientPlayer[i] = nullptr;	
		}
	}
	return removed;
}

int NetPlayServer::closeAllConnections(void)
{
	std::list <NetPlayClient*>::iterator it;

	for (it = clientList.begin(); it != clientList.end(); it++)
	{
		delete *it;
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
	printf("Prep ROM Load Request: %s \n", filepath );

	if (filepath == nullptr)
	{
		return -1;
	}
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
	strncpy( msg.fileName, GameInfo->filename, sizeof(msg.fileName) );

	printf("Sending ROM Load Request: %s  %lu\n", filepath, fileSize );
	FCEUI_SetEmulationPaused(EMULATIONPAUSED_PAUSED);

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
	netPlayMsgHdr hdr(NETPLAY_SYNC_STATE);

	if ( GameInfo == nullptr )
	{
		return -1;
	}
	FCEUSS_SaveMS( &em, compressionLevel );

	hdr.msgSize += em.size();

	printf("Sending ROM Sync Request\n");
	FCEUI_SetEmulationPaused(EMULATIONPAUSED_PAUSED);

	sendMsg( client, &hdr, sizeof(netPlayMsgHdr) );
	sendMsg( client, em.buf(), em.size() );

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

	//printf("Server Received Message: ID: %i   Bytes:%zu\n", hdr->msgId, msgSize );

	switch (hdr->msgId)
	{
		case NETPLAY_AUTH_RESP:
		{
			netPlayAuthResp *msg = static_cast<netPlayAuthResp*>(msgBuf);
			printf("Authorize: Player: %i   Passwd: %s\n", msg->playerId, msg->pswd);

			if ( claimRole(client, msg->playerId) )
			{
				sendRomLoadReq( client );
				sendStateSyncReq( client );
				client->state = 1;
			}
			else
			{
				netPlayErrorMsg<128>  errorMsg;
				errorMsg.printf("Player %i role is not available", msg->playerId+1);
				sendMsg( client, &errorMsg, errorMsg.hdr.msgSize );
				//client->disconnect();
			}
		}
		break;
		case NETPLAY_RUN_FRAME_RESP:
		{
			netPlayRunFrameResp *msg = static_cast<netPlayRunFrameResp*>(msgBuf);

			client->currentFrame = msg->frameRun;
		}
		break;
		case NETPLAY_CLIENT_STATE:
		{
			netPlayClientState *msg = static_cast<netPlayClientState*>(msgBuf);

			client->currentFrame = msg->frameRun;
		}
		break;
		default:

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

	if (currFrame > maxLead)
	{
		lagFrame = currFrame - maxLead;
	}

	// Input Processing
	for (auto it = clientList.begin(); it != clientList.end(); it++)
	{
		NetPlayClient *client = *it;

		client->readMessages( serverMessageCallback, client );

		if (client->currentFrame < clientMinFrame)
		{
			clientMinFrame = client->currentFrame;
		}
		if (client->currentFrame > clientMaxFrame)
		{
			clientMaxFrame = client->currentFrame;
		}
	}

	shouldRunFrame = (clientMinFrame != 0xFFFFFFFF) && 
		(clientMinFrame >= lagFrame ) &&
		(clientMaxFrame  < leadFrame) &&
		(currFrame > lastFrame);

	//printf("Client Frame: Min:%u  Max:%u\n", clientMinFrame, clientMaxFrame);

	if (shouldRunFrame)
	{
		// Output Processing
		NetPlayFrameInput  inputFrame;
		netPlayRunFrameReq  runFrameReq;

		uint32_t ctlrData = GetGamepadPressedImmediate();

		inputFrame.frameCounter = static_cast<uint32_t>(currFrameCounter) + 1;
		inputFrame.ctrl[0] = (ctlrData      ) & 0x000000ff;
		inputFrame.ctrl[1] = (ctlrData >>  8) & 0x000000ff;
		inputFrame.ctrl[2] = (ctlrData >> 16) & 0x000000ff;
		inputFrame.ctrl[3] = (ctlrData >> 24) & 0x000000ff;

		runFrameReq.frameNum     = inputFrame.frameCounter;
		runFrameReq.ctrlState[0] = inputFrame.ctrl[0];
		runFrameReq.ctrlState[1] = inputFrame.ctrl[1];
		runFrameReq.ctrlState[2] = inputFrame.ctrl[2];
		runFrameReq.ctrlState[3] = inputFrame.ctrl[3];

		pushBackInput( inputFrame );

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
//-----------------------------------------------------------------------------
void NetPlayClient::disconnect()
{
	sock->close();
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
int NetPlayClient::createSocket(void)
{
	if (sock == nullptr)
	{
		sock = new QTcpSocket(this);
	}

	connect(sock, SIGNAL(connected(void))   , this, SLOT(onConnect(void)));
	connect(sock, SIGNAL(disconnected(void)), this, SLOT(onDisconnect(void)));
	
	return 0;
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
}
//-----------------------------------------------------------------------------
void NetPlayClient::onDisconnect(void)
{
	printf("Client Disconnected!!!\n");

	NetPlayServer *server = NetPlayServer::GetInstance();

	if (server)
	{
		if (server->removeClient(this))
		{
			deleteLater();
		}
	}
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

	uint32_t ctlrData = GetGamepadPressedImmediate();
	uint32_t currFrame = static_cast<uint32_t>(currFrameCounter);

	netPlayClientState  statusMsg;
	statusMsg.flags    = 0;
	statusMsg.frameRdy = inputFrameBack();
	statusMsg.frameRun = currFrame;
	statusMsg.ctrlState[0] = (ctlrData      ) & 0x000000ff;
	statusMsg.ctrlState[1] = (ctlrData >>  8) & 0x000000ff;
	statusMsg.ctrlState[2] = (ctlrData >> 16) & 0x000000ff;
	statusMsg.ctrlState[3] = (ctlrData >> 24) & 0x000000ff;

	sock->write( reinterpret_cast<const char*>(&statusMsg), sizeof(statusMsg) );

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

				recvMsgId   = hdr->msgId;
				recvMsgSize = hdr->msgSize - sizeof(netPlayMsgHdr);
				recvMsgBytesLeft = recvMsgSize;

				if (hdr->msgSize > recvMsgBufSize)
				{
					printf("Error: Message size too large\n");
				}
				//printf("HDR: Id: %u   Size: %u\n", hdr->msgId, hdr->msgSize );

				recvMsgByteIndex = sizeof(netPlayMsgHdr);

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

	switch (hdr->msgId)
	{
		case NETPLAY_AUTH_REQ:
		{
			netPlayAuthResp msg;
			msg.playerId = role;
			strncpy( msg.pswd, "TODO: Dummy Password", sizeof(msg.pswd) );

			sock->write( (const char*)&msg, sizeof(netPlayAuthResp) );
		}
		break;
		case NETPLAY_LOAD_ROM_REQ:
		{
			FILE *fp;
			std::string filepath = QDir::tempPath().toStdString(); 
			netPlayLoadRomReq *msg = static_cast<netPlayLoadRomReq*>(msgBuf);
			const char *romData = &static_cast<const char*>(msgBuf)[ sizeof(netPlayLoadRomReq) ];

			filepath.append( "/" );
			filepath.append( msg->fileName );

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
		case NETPLAY_SYNC_STATE:
		{
			char *stateData = &static_cast<char*>(msgBuf)[ sizeof(netPlayMsgHdr) ];

			EMUFILE_MEMORY em( stateData, msgSize );

			FCEUSS_LoadFP( &em, SSLOADPARAM_NOBACKUP );
		}
		break;
		case NETPLAY_RUN_FRAME_REQ:
		{
			NetPlayFrameInput  inputFrame;
			netPlayRunFrameReq *msg = static_cast<netPlayRunFrameReq*>(msgBuf);

			uint32_t currFrame = static_cast<uint32_t>(currFrameCounter);

			inputFrame.frameCounter = msg->frameNum;
			inputFrame.ctrl[0] = msg->ctrlState[0];
			inputFrame.ctrl[1] = msg->ctrlState[1];
			inputFrame.ctrl[2] = msg->ctrlState[2];
			inputFrame.ctrl[3] = msg->ctrlState[3];

			if (inputFrame.frameCounter > inputFrameBack())
			{
				pushBackInput( inputFrame );
			}

			netPlayRunFrameResp  resp;
			resp.flags    = msg->flags;
			resp.frameNum = msg->frameNum;
			resp.frameRun = currFrame;
			resp.ctrlState[0] = msg->ctrlState[0];
			resp.ctrlState[1] = msg->ctrlState[1];
			resp.ctrlState[2] = msg->ctrlState[2];
			resp.ctrlState[3] = msg->ctrlState[3];

			sock->write( reinterpret_cast<const char*>(&resp), sizeof(resp) );
		}
		break;
		default:

		break;
	}
}
//-----------------------------------------------------------------------------
//--- NetPlayHostDialog
//-----------------------------------------------------------------------------
NetPlayHostDialog::NetPlayHostDialog(QWidget *parent)
	: QDialog(parent)
{
	QVBoxLayout *mainLayout;
	QHBoxLayout *hbox;
	QGridLayout *grid;
	QPushButton *cancelButton, *startButton;
	QLabel *lbl;

	setWindowTitle("NetPlay Host Game");

	mainLayout = new QVBoxLayout();
	grid = new QGridLayout();

	lbl = new QLabel( tr("Server Name:") );
	grid->addWidget( lbl, 0, 0 );

	serverNameEntry = new QLineEdit();
	grid->addWidget( serverNameEntry, 0, 1 );

	lbl = new QLabel( tr("Port:") );
	grid->addWidget( lbl, 1, 0 );

	portEntry = new QSpinBox();
	portEntry->setRange(0,65535);
	portEntry->setValue(5050);
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

	server = NetPlayServer::GetInstance();
	server->setRole( playerRoleBox->currentData().toInt() );

	if (server->listen( QHostAddress::Any, portEntry->value() ) == false)
	{
		printf("Error: TCP server failed to listen\n");
	}

	done(0);
	deleteLater();
}
//-----------------------------------------------------------------------------
//--- NetPlayJoinDialog
//-----------------------------------------------------------------------------
NetPlayJoinDialog::NetPlayJoinDialog(QWidget *parent)
	: QDialog(parent)
{
	QVBoxLayout *mainLayout;
	QHBoxLayout *hbox;
	QGridLayout *grid;
	QPushButton *cancelButton, *startButton;
	QLabel *lbl;

	setWindowTitle("NetPlay Host Game");

	mainLayout = new QVBoxLayout();
	grid = new QGridLayout();

	lbl = new QLabel( tr("Host:") );
	grid->addWidget( lbl, 0, 0 );

	hostEntry = new QLineEdit();
	hostEntry->setText("localhost");
	grid->addWidget( hostEntry, 0, 1 );

	lbl = new QLabel( tr("Port:") );
	grid->addWidget( lbl, 1, 0 );

	portEntry = new QSpinBox();
	portEntry->setRange(0,65535);
	portEntry->setValue(5050);
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

	client = NetPlayClient::GetInstance();
	client->role = playerRoleBox->currentData().toInt();

	if (client->connectToHost( hostEntry->text(), portEntry->value() ))
	{
		printf("Failed to connect to Host\n");
	}

	//printf("Close Window\n");
	done(0);
	deleteLater();
}
//----------------------------------------------------------------------------
//---- Global Functions
//----------------------------------------------------------------------------
bool NetPlayActive(void)
{
	return (NetPlayClient::GetInstance() != nullptr) || (NetPlayServer::GetInstance() != nullptr);
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
}
//----------------------------------------------------------------------------
