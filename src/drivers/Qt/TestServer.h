#pragma once

#ifdef __FCEU_TEST_SERVER_ENABLE__

#include <array>

#include <QByteArray>
#include <QHash>
#include <QObject>
#include <QString>
#include <QTcpServer>
#include <QtGlobal>

class QTcpSocket;

class TestServer : public QObject
{
public:
	explicit TestServer(QObject *parent = nullptr);

	bool listen(quint16 port, QString *errorMessage = nullptr);
	quint16 serverPort(void) const;
	QString authenticationToken(void) const;

private:
	void acceptConnections(void);
	void readClient(QTcpSocket *socket);
	void handleRequest(QTcpSocket *socket, const QByteArray &line);

	QTcpServer server;
	QHash<QTcpSocket *, QByteArray> clientBuffers;
	std::array<quint8, 2> padStates{};
	QString token;
};

#endif
