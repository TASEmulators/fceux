// NetPlay.h
//

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <list>

#include <QWidget>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QFrame>
#include <QGroupBox>
#include <QCloseEvent>

#include <QTcpSocket>
#include <QTcpServer>

#include "utils/mutex.h"

class NetPlayClient;

struct NetPlayFrameInput
{
	static constexpr uint32_t ownsData = 0x01;

	NetPlayFrameInput(void)
	{
		flags = 0; frameCounter = 0; 
		ctrl[0] = ctrl[1] = ctrl[2] = ctrl[3] = 0;
		data = nullptr;
	}

	~NetPlayFrameInput()
	{
		if (data)
		{
			if (flags & ownsData)
			{
				::free(data);
				data = nullptr;
			}
		}
	}

	uint32_t  flags;
	uint32_t  frameCounter;
	uint8_t   ctrl[4];
	uint8_t  *data;
};

class NetPlayServer : public QTcpServer
{
	Q_OBJECT

	public:
		NetPlayServer(QObject *parent = 0);
		~NetPlayServer(void);

		static NetPlayServer *GetInstance(void){ return instance; };

		static int Create(QObject *parent = 0);

		bool removeClient(NetPlayClient *client, bool markForDelete = false);

		int closeAllConnections(void);

		void update(void);

		size_t inputAvailable(void)
		{
			FCEU::autoScopedLock alock(inputMtx);
		       	return input.size();
		};

		void pushBackInput( NetPlayFrameInput &in )
		{
			FCEU::autoScopedLock alock(inputMtx);
			input.push_back(in);
		};

		NetPlayFrameInput getNextInput(void)
		{
			NetPlayFrameInput in;
			FCEU::autoScopedLock alock(inputMtx);
			if (!input.empty())
			{
				in = input.front();
				input.pop_front();
			}
			return in;
		};

		uint32_t inputFrameBack()
		{
			uint32_t frame = 0;
			FCEU::autoScopedLock alock(inputMtx);
			if (!input.empty())
			{
				NetPlayFrameInput &in = input.back();
				frame = in.frameCounter;
			}
			return frame;
		}

		int  sendMsg( NetPlayClient *client, void *msg, size_t msgSize);
		int  sendRomLoadReq( NetPlayClient *client );
		int  sendStateSyncReq( NetPlayClient *client );
		void setRole(int _role);
		bool claimRole(NetPlayClient* client, int _role);

		void serverProcessMessage( NetPlayClient *client, void *msgBuf, size_t msgSize );

	private:
		static NetPlayServer *instance;

		void processPendingConnections(void);

		std::list <NetPlayClient*> clientList;
		std::list <NetPlayFrameInput> input;
		FCEU::mutex inputMtx;
		int role = -1;
		int roleMask = 0;
		NetPlayClient* clientPlayer[4] = { nullptr };

	public slots:
		void newConnectionRdy(void);
};

class NetPlayClient : public QObject
{
	Q_OBJECT

	public:
		NetPlayClient(QObject *parent = 0, bool outGoing = false);
		~NetPlayClient(void);

		static NetPlayClient *GetInstance(void){ return instance; };

		static int Create(QObject *parent = 0);

		int connectToHost( const QString host, int port );

		bool isConnected(void);
		void disconnect();

		void setSocket(QTcpSocket *s);
		QTcpSocket* getSocket(void){ return sock; };

		void update(void);
		int  readMessages( void (*msgCallback)( void *userData, void *msgBuf, size_t msgSize ), void *userData );
		void clientProcessMessage( void *msgBuf, size_t msgSize );

		size_t inputAvailable(void)
		{
			FCEU::autoScopedLock alock(inputMtx);
		       	return !input.empty();
		};

		void pushBackInput( NetPlayFrameInput &in )
		{
			FCEU::autoScopedLock alock(inputMtx);
			input.push_back(in);
		};

		NetPlayFrameInput getNextInput(void)
		{
			NetPlayFrameInput in;
			FCEU::autoScopedLock alock(inputMtx);
			if (!input.empty())
			{
				in = input.front();
				input.pop_front();
			}
			return in;
		};

		uint32_t inputFrameBack()
		{
			uint32_t frame = 0;
			FCEU::autoScopedLock alock(inputMtx);
			if (!input.empty())
			{
				NetPlayFrameInput &in = input.back();
				frame = in.frameCounter;
			}
			return frame;
		}


		QString userName;
		int     role;
		int     state;
		unsigned int currentFrame;

	private:
		int createSocket(void);

		static NetPlayClient *instance;

		QTcpSocket *sock;
		int     recvMsgId;
		int     recvMsgSize;
		int     recvMsgBytesLeft;
		int     recvMsgByteIndex;
		char   *recvMsgBuf;

		std::list <NetPlayFrameInput> input;
		FCEU::mutex inputMtx;

		static constexpr size_t recvMsgBufSize = 2 * 1024 * 1024;

	public slots:
		void onConnect(void);
		void onDisconnect(void);
};


class NetPlayHostDialog : public QDialog
{
	Q_OBJECT

public:
	NetPlayHostDialog(QWidget *parent = 0);
	~NetPlayHostDialog(void);

protected:
	void closeEvent(QCloseEvent *event);

	QLineEdit  *serverNameEntry;
	QSpinBox   *portEntry;
	QComboBox  *playerRoleBox;

public slots:
	void closeWindow(void);
	void onStartClicked(void);

};

class NetPlayJoinDialog : public QDialog
{
	Q_OBJECT

public:
	NetPlayJoinDialog(QWidget *parent = 0);
	~NetPlayJoinDialog(void);

protected:
	void closeEvent(QCloseEvent *event);

	QLineEdit  *hostEntry;
	QSpinBox   *portEntry;
	QComboBox  *playerRoleBox;

public slots:
	void closeWindow(void);
	void onJoinClicked(void);

};

bool NetPlayActive(void);
void NetPlayPeriodicUpdate(void);
bool NetPlaySkipWait(void);
int NetPlayFrameWait(void);
void NetPlayReadInputFrame(uint8_t* joy);
