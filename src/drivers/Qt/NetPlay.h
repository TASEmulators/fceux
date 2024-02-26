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

		static constexpr int DefaultPort = 4046;
		static NetPlayServer *GetInstance(void){ return instance; };

		static int Create(QObject *parent = 0);
		static int Destroy();

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

		void inputClear()
		{
			FCEU::autoScopedLock alock(inputMtx);
			input.clear();
		}

		int  sendMsg( NetPlayClient *client, void *msg, size_t msgSize);
		int  sendRomLoadReq( NetPlayClient *client );
		int  sendStateSyncReq( NetPlayClient *client );
		void setRole(int _role);
		bool claimRole(NetPlayClient* client, int _role);

		uint32_t getMaxLeadFrames(){ return maxLeadFrames; }
		void setMaxLeadFrames(uint32_t value){ maxLeadFrames = value; }

		void serverProcessMessage( NetPlayClient *client, void *msgBuf, size_t msgSize );

		QString sessionName;
		QString sessionPasswd;
	private:
		static NetPlayServer *instance;

		void processPendingConnections(void);

		std::list <NetPlayClient*> clientList;
		std::list <NetPlayFrameInput> input;
		FCEU::mutex inputMtx;
		int role = -1;
		int roleMask = 0;
		NetPlayClient* clientPlayer[4] = { nullptr };
		int forceResyncCount = 10;
		uint32_t cycleCounter = 0;
		uint32_t maxLeadFrames = 10u;

	public slots:
		void newConnectionRdy(void);
		void onRomLoad(void);
		void onNesReset(void);
};

class NetPlayClient : public QObject
{
	Q_OBJECT

	public:
		NetPlayClient(QObject *parent = 0, bool outGoing = false);
		~NetPlayClient(void);

		static NetPlayClient *GetInstance(void){ return instance; };

		static int Create(QObject *parent = 0);
		static int Destroy();

		int connectToHost( const QString host, int port );

		bool isConnected(void){ return _connected; }
		bool disconnectRequested(){ return disconnectPending; }
		void forceDisconnect();
		bool flushData();

		QTcpSocket* createSocket(void);
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

		void inputClear()
		{
			FCEU::autoScopedLock alock(inputMtx);
			input.clear();
		}

		bool isAuthenticated();
		bool isPlayerRole();
		bool shouldDestroy(){ return needsDestroy; }
		bool isPaused(){ return paused; }
		void setPaused(bool value){ paused = value; }
		void recordPingResult( uint64_t delay_ms );
		void resetPingData(void);
		double getAvgPingDelay();

		QString userName;
		QString password;
		int     role = -1;
		int     state = 0;
		int     desyncCount = 0;
		bool    syncOk = false;
		unsigned int currentFrame = 0;
		uint8_t gpData[4];

	private:

		static NetPlayClient *instance;

		QTcpSocket *sock = nullptr;
		int     recvMsgId = 0;
		int     recvMsgSize = 0;
		int     recvMsgBytesLeft = 0;
		int     recvMsgByteIndex = 0;
		char   *recvMsgBuf = nullptr;
		bool    disconnectPending = false;
		bool    needsDestroy = false;
		bool    _connected = false;
		bool    paused = false;

		uint64_t  pingDelaySum = 0;
		uint64_t  pingDelayLast = 0;
		uint64_t  pingNumSamples = 0;

		std::list <NetPlayFrameInput> input;
		FCEU::mutex inputMtx;

		static constexpr size_t recvMsgBufSize = 2 * 1024 * 1024;

	signals:
		void connected(void);
		void errorOccurred(const QString&);

	public slots:
		void onConnect(void);
		void onDisconnect(void);
		void onSocketError(QAbstractSocket::SocketError);
};


class NetPlayHostDialog : public QDialog
{
	Q_OBJECT

public:
	NetPlayHostDialog(QWidget *parent = 0);
	~NetPlayHostDialog(void);

	static NetPlayHostDialog *GetInstance(void){ return instance; };
protected:
	void closeEvent(QCloseEvent *event);

	QLineEdit  *sessionNameEntry;
	QSpinBox   *portEntry;
	QComboBox  *playerRoleBox;
	QLineEdit  *passwordEntry;
	QCheckBox  *passwordRequiredCBox;

	static NetPlayHostDialog* instance;

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

	static NetPlayJoinDialog *GetInstance(void){ return instance; };

protected:
	void closeEvent(QCloseEvent *event);

	QLineEdit  *hostEntry;
	QSpinBox   *portEntry;
	QComboBox  *playerRoleBox;
	QLineEdit  *userNameEntry;
	QLineEdit  *passwordEntry;

	static NetPlayJoinDialog* instance;

public slots:
	void closeWindow(void);
	void onJoinClicked(void);
	void onConnect(void);
	void onSocketError(const QString& errorMsg);
};

bool NetPlayActive(void);
bool isNetPlayHost(void);
void NetPlayPeriodicUpdate(void);
bool NetPlaySkipWait(void);
int NetPlayFrameWait(void);
void NetPlayOnFrameBegin(void);
void NetPlayReadInputFrame(uint8_t* joy);
void NetPlayCloseSession(void);
void NetPlayTraceInstruction(uint8_t *opcode, int size);
void openNetPlayHostDialog(QWidget* parent = nullptr);
void openNetPlayJoinDialog(QWidget* parent = nullptr);
