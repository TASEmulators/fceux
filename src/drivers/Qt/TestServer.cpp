#include "Qt/TestServer.h"

#ifdef __FCEU_TEST_SERVER_ENABLE__

#include <cmath>

#include <QBuffer>
#include <QCryptographicHash>
#include <QFileInfo>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTcpSocket>
#include <QUuid>

#include "Qt/ConsoleWindow.h"
#include "Qt/fceuWrapper.h"
#include "Qt/input.h"
#include "../../debug.h"
#include "../../driver.h"
#include "../../git.h"
#include "../../video.h"

namespace
{
	constexpr int MaxRequestBytes = 64 * 1024;
	constexpr int MaxMemoryReadBytes = 4096;

	QJsonObject makeError(const QJsonValue &id, int code, const QString &message)
	{
		QJsonObject error;
		error.insert(QStringLiteral("code"), code);
		error.insert(QStringLiteral("message"), message);

		QJsonObject response;
		response.insert(QStringLiteral("id"), id);
		response.insert(QStringLiteral("error"), error);
		return response;
	}

	bool getInteger(const QJsonObject &object, const QString &key, int minimum, int maximum, int &value)
	{
		const QJsonValue jsonValue = object.value(key);
		if (!jsonValue.isDouble())
		{
			return false;
		}
		const double number = jsonValue.toDouble();
		if (!std::isfinite(number) || std::floor(number) != number || number < minimum || number > maximum)
		{
			return false;
		}
		value = static_cast<int>(number);
		return true;
	}

	bool parseButtons(const QJsonValue &value, quint8 &buttons)
	{
		if (!value.isArray())
		{
			return false;
		}

		static const QHash<QString, quint8> buttonMasks = {
			{QStringLiteral("A"), 0x01},
			{QStringLiteral("B"), 0x02},
			{QStringLiteral("Select"), 0x04},
			{QStringLiteral("Start"), 0x08},
			{QStringLiteral("Up"), 0x10},
			{QStringLiteral("Down"), 0x20},
			{QStringLiteral("Left"), 0x40},
			{QStringLiteral("Right"), 0x80},
		};

		buttons = 0;
		for (const QJsonValue &item : value.toArray())
		{
			if (!item.isString() || !buttonMasks.contains(item.toString()))
			{
				return false;
			}
			buttons |= buttonMasks.value(item.toString());
		}
		return true;
	}

	QJsonArray buttonNames(quint8 buttons)
	{
		static const std::array<std::pair<const char *, quint8>, 8> buttonsByMask = {{
			{"A", 0x01},
			{"B", 0x02},
			{"Select", 0x04},
			{"Start", 0x08},
			{"Up", 0x10},
			{"Down", 0x20},
			{"Left", 0x40},
			{"Right", 0x80},
		}};

		QJsonArray names;
		for (const auto &button : buttonsByMask)
		{
			if (buttons & button.second)
			{
				names.append(QString::fromLatin1(button.first));
			}
		}
		return names;
	}
}

TestServer::TestServer(QObject *parent)
	: QObject(parent)
	, server(this)
	, token(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
	connect(&server, &QTcpServer::newConnection, this, [this]() { acceptConnections(); });
}

bool TestServer::listen(quint16 port, QString *errorMessage)
{
	if (!server.listen(QHostAddress::LocalHost, port))
	{
		if (errorMessage != nullptr)
		{
			*errorMessage = server.errorString();
		}
		return false;
	}
	return true;
}

quint16 TestServer::serverPort(void) const
{
	return server.serverPort();
}

QString TestServer::authenticationToken(void) const
{
	return token;
}

void TestServer::acceptConnections(void)
{
	while (server.hasPendingConnections())
	{
		QTcpSocket *socket = server.nextPendingConnection();
		if (socket == nullptr)
		{
			continue;
		}
		clientBuffers.insert(socket, QByteArray());
		connect(socket, &QTcpSocket::readyRead, this, [this, socket]() { readClient(socket); });
		connect(socket, &QTcpSocket::disconnected, this, [this, socket]()
		{
			clientBuffers.remove(socket);
			socket->deleteLater();
		});
	}
}

void TestServer::readClient(QTcpSocket *socket)
{
	QByteArray &buffer = clientBuffers[socket];
	buffer.append(socket->readAll());
	if (buffer.size() > MaxRequestBytes && !buffer.contains('\n'))
	{
		socket->disconnectFromHost();
		return;
	}

	int newline = -1;
	while ((newline = buffer.indexOf('\n')) >= 0)
	{
		const QByteArray line = buffer.left(newline).trimmed();
		buffer.remove(0, newline + 1);
		if (line.size() > MaxRequestBytes)
		{
			socket->disconnectFromHost();
			return;
		}
		handleRequest(socket, line);
	}
}

void TestServer::handleRequest(QTcpSocket *socket, const QByteArray &line)
{
	QJsonParseError parseError;
	const QJsonDocument document = QJsonDocument::fromJson(line, &parseError);
	if (parseError.error != QJsonParseError::NoError || !document.isObject())
	{
		socket->write(QJsonDocument(makeError(QJsonValue(), -32700,
			QStringLiteral("Invalid JSON request"))).toJson(QJsonDocument::Compact) + '\n');
		return;
	}

	const QJsonObject request = document.object();
	const QJsonValue id = request.value(QStringLiteral("id"));
	if ((!id.isString() && !id.isDouble()) ||
		request.value(QStringLiteral("token")).toString() != token)
	{
		socket->write(QJsonDocument(makeError(id, -32001,
			QStringLiteral("Missing or invalid request id or authentication token")))
			.toJson(QJsonDocument::Compact) + '\n');
		return;
	}

	const QString method = request.value(QStringLiteral("method")).toString();
	const QJsonObject params = request.value(QStringLiteral("params")).toObject();
	QJsonValue result;
	QString errorMessage;
	int errorCode = -32602;
	bool quitRequested = false;

	if (method == QStringLiteral("app.quit"))
	{
		if (consoleWindow == nullptr)
		{
			errorCode = -32016;
			errorMessage = QStringLiteral("Main window is not available");
		}
		else
		{
			quitRequested = true;
			QJsonObject quitting;
			quitting.insert(QStringLiteral("quitting"), true);
			result = quitting;
		}
	}
	else if (method == QStringLiteral("rom.load"))
	{
		const QString requestedPath = params.value(QStringLiteral("path")).toString();
		const QFileInfo fileInfo(requestedPath);
		if (requestedPath.isEmpty() || !fileInfo.exists() || !fileInfo.isFile())
		{
			errorMessage = QStringLiteral("ROM path does not name an existing file");
		}
		else if (LoadGame(fileInfo.canonicalFilePath().toLocal8Bit().constData(), true, false) != 1)
		{
			errorCode = -32010;
			errorMessage = QStringLiteral("ROM load failed");
		}
		else
		{
			padStates = {0, 0};
			FCEUI_SetTestPadState(0, 0);
			FCEUI_SetTestPadState(1, 0);
			fceuWrapperTestPause();
			QJsonObject loaded;
			loaded.insert(QStringLiteral("path"), fileInfo.canonicalFilePath());
			result = loaded;
		}
	}
	else if (method == QStringLiteral("pad.set") ||
		method == QStringLiteral("pad.press") ||
		method == QStringLiteral("pad.release"))
	{
		int port = 0;
		quint8 buttons = 0;
		if (!getInteger(params, QStringLiteral("port"), 1, 2, port) ||
			!parseButtons(params.value(QStringLiteral("buttons")), buttons))
		{
			errorMessage = QStringLiteral("port must be 1 or 2 and buttons must be an array of valid button names");
		}
		else if (!fceuWrapperGameLoaded())
		{
			errorCode = -32011;
			errorMessage = QStringLiteral("No ROM is loaded");
		}
		else
		{
			int inputType = SI_UNSET;
			getInputSelection(port - 1, &inputType, nullptr);
			if (inputType != SI_GAMEPAD)
			{
				errorCode = -32014;
				errorMessage = QStringLiteral("Requested port is not configured as a gamepad");
			}
			else
			{
				quint8 &state = padStates[static_cast<size_t>(port - 1)];
				if (method == QStringLiteral("pad.set"))
				{
					state = buttons;
				}
				else if (method == QStringLiteral("pad.press"))
				{
					state |= buttons;
				}
				else
				{
					state &= static_cast<quint8>(~buttons);
				}
				FCEUI_SetTestPadState(port - 1, state);

				QJsonObject pad;
				pad.insert(QStringLiteral("port"), port);
				pad.insert(QStringLiteral("buttons"), buttonNames(state));
				result = pad;
			}
		}
	}
	else if (method == QStringLiteral("emu.frameAdvance"))
	{
		int frames = 0;
		int timeoutMs = 30000;
		if (!getInteger(params, QStringLiteral("frames"), 1, 100000, frames) ||
			(params.contains(QStringLiteral("timeoutMs")) &&
			 !getInteger(params, QStringLiteral("timeoutMs"), 1, 120000, timeoutMs)))
		{
			errorMessage = QStringLiteral("frames must be 1..100000 and timeoutMs must be 1..120000");
		}
		else
		{
			const int advanceResult = fceuWrapperTestFrameAdvance(frames, timeoutMs);
			if (advanceResult == 1)
			{
				QJsonObject advanced;
				advanced.insert(QStringLiteral("frames"), frames);
				result = advanced;
			}
			else
			{
				errorCode = advanceResult < 0 ? -32011 : -32012;
				errorMessage = advanceResult < 0
					? QStringLiteral("No ROM is loaded")
					: QStringLiteral("Timed out while advancing frames");
			}
		}
	}
	else if (method == QStringLiteral("emu.reset"))
	{
		if (!fceuWrapperGameLoaded())
		{
			errorCode = -32011;
			errorMessage = QStringLiteral("No ROM is loaded");
		}
		else
		{
			FCEU_WRAPPER_SCOPED_LOCK();
			fceuWrapperSoftReset();
			QJsonObject reset;
			reset.insert(QStringLiteral("reset"), true);
			result = reset;
		}
	}
	else if (method == QStringLiteral("mem.read"))
	{
		int address = 0;
		int length = 0;
		if (!getInteger(params, QStringLiteral("address"), 0, 0xFFFF, address) ||
			!getInteger(params, QStringLiteral("length"), 1, MaxMemoryReadBytes, length) ||
			address + length > 0x10000)
		{
			errorMessage = QStringLiteral("address or length is out of range");
		}
		else if (!fceuWrapperGameLoaded())
		{
			errorCode = -32011;
			errorMessage = QStringLiteral("No ROM is loaded");
		}
		else
		{
			QByteArray bytes;
			bytes.resize(length);
			FCEU_WRAPPER_SCOPED_LOCK();
			for (int index = 0; index < length; index++)
			{
				bytes[index] = static_cast<char>(GetMem(static_cast<uint16>(address + index)));
			}
			QJsonObject memory;
			memory.insert(QStringLiteral("address"), address);
			memory.insert(QStringLiteral("bytes"), QString::fromLatin1(bytes.toHex()));
			result = memory;
		}
	}
	else if (method == QStringLiteral("frame.hash"))
	{
		if (!fceuWrapperGameLoaded())
		{
			errorCode = -32011;
			errorMessage = QStringLiteral("No ROM is loaded");
		}
		else
		{
			FCEU_WRAPPER_SCOPED_LOCK();
			if (XBuf == nullptr)
			{
				errorCode = -32013;
				errorMessage = QStringLiteral("Video frame is not available");
			}
			else
			{
				const QByteArray frame(reinterpret_cast<const char *>(XBuf), 256 * 240);
				QJsonObject hash;
				hash.insert(QStringLiteral("algorithm"), QStringLiteral("sha256"));
				hash.insert(QStringLiteral("format"), QStringLiteral("palette-index-256x240"));
				hash.insert(QStringLiteral("value"),
					QString::fromLatin1(QCryptographicHash::hash(frame, QCryptographicHash::Sha256).toHex()));
				result = hash;
			}
		}
	}
	else if (method == QStringLiteral("frame.screenshot"))
	{
		if (!fceuWrapperGameLoaded())
		{
			errorCode = -32011;
			errorMessage = QStringLiteral("No ROM is loaded");
		}
		else
		{
			QImage image(256, 240, QImage::Format_RGB32);
			bool frameAvailable = false;
			{
				FCEU_WRAPPER_SCOPED_LOCK();
				if (XBuf != nullptr)
				{
					for (int y = 0; y < image.height(); y++)
					{
						auto *row = reinterpret_cast<QRgb *>(image.scanLine(y));
						for (int x = 0; x < image.width(); x++)
						{
							quint8 red = 0;
							quint8 green = 0;
							quint8 blue = 0;
							FCEUD_GetPalette(XBuf[y * image.width() + x], &red, &green, &blue);
							row[x] = qRgb(red, green, blue);
						}
					}
					frameAvailable = true;
				}
			}

			if (!frameAvailable)
			{
				errorCode = -32013;
				errorMessage = QStringLiteral("Video frame is not available");
			}
			else
			{
				QByteArray pngData;
				QBuffer pngBuffer(&pngData);
				if (!pngBuffer.open(QIODevice::WriteOnly) || !image.save(&pngBuffer, "PNG"))
				{
					errorCode = -32015;
					errorMessage = QStringLiteral("Could not encode screenshot as PNG");
				}
				else
				{
					QJsonObject screenshot;
					screenshot.insert(QStringLiteral("format"), QStringLiteral("png"));
					screenshot.insert(QStringLiteral("width"), image.width());
					screenshot.insert(QStringLiteral("height"), image.height());
					screenshot.insert(QStringLiteral("data"), QString::fromLatin1(pngData.toBase64()));
					result = screenshot;
				}
			}
		}
	}
	else
	{
		errorCode = -32601;
		errorMessage = QStringLiteral("Unknown method");
	}

	QJsonObject response;
	response.insert(QStringLiteral("id"), id);
	if (errorMessage.isEmpty())
	{
		response.insert(QStringLiteral("result"), result);
	}
	else
	{
		QJsonObject error;
		error.insert(QStringLiteral("code"), errorCode);
		error.insert(QStringLiteral("message"), errorMessage);
		response.insert(QStringLiteral("error"), error);
	}
	socket->write(QJsonDocument(response).toJson(QJsonDocument::Compact) + '\n');

	if (quitRequested)
	{
		socket->flush();
		// Queued so the reply is sent before the normal File > Quit shutdown path runs.
		QMetaObject::invokeMethod(consoleWindow, "closeApp", Qt::QueuedConnection);
	}
}

#endif
