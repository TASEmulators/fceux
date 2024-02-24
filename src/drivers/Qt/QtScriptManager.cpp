/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2020 thor
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
// QtScriptManager.cpp
//
#ifdef __FCEU_QSCRIPT_ENABLE__
#include <stdio.h>
#include <string.h>
#include <list>

#ifdef WIN32
#include <Windows.h>
#endif

#include <QUrl>
#include <QFile>
#include <QIODevice>
#include <QTextEdit>
#include <QFileInfo>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>
#include <QHeaderView>
#include <QDesktopServices>
#include <QJSValueIterator>

#ifdef __QT_UI_TOOLS__
#include <QUiLoader>
#endif

#include "../../fceu.h"
#include "../../movie.h"
#include "../../video.h"
#include "../../x6502.h"
#include "../../debug.h"
#include "../../state.h"
#include "../../ppu.h"

#include "common/os_utils.h"
#include "utils/xstring.h"

#include "Qt/QtScriptManager.h"
#include "Qt/main.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/keyscan.h"
#include "Qt/fceuWrapper.h"
#include "Qt/ConsoleUtilities.h"
#include "Qt/ConsoleWindow.h"

// pix format for JS graphics
#define BUILD_PIXEL_ARGB8888(A,R,G,B) (((int) (A) << 24) | ((int) (R) << 16) | ((int) (G) << 8) | (int) (B))
#define DECOMPOSE_PIXEL_ARGB8888(PIX,A,R,G,B) { (A) = ((PIX) >> 24) & 0xff; (R) = ((PIX) >> 16) & 0xff; (G) = ((PIX) >> 8) & 0xff; (B) = (PIX) & 0xff; }
#define JS_BUILD_PIXEL BUILD_PIXEL_ARGB8888
#define JS_DECOMPOSE_PIXEL DECOMPOSE_PIXEL_ARGB8888
#define JS_PIXEL_A(PIX) (((PIX) >> 24) & 0xff)
#define JS_PIXEL_R(PIX) (((PIX) >> 16) & 0xff)
#define JS_PIXEL_G(PIX) (((PIX) >> 8) & 0xff)
#define JS_PIXEL_B(PIX) ((PIX) & 0xff)

// File Base Name from Core
extern char FileBase[];
extern uint8 joy[4];

static thread_local FCEU::JSEngine* currentEngine = nullptr;

namespace JS
{
//----------------------------------------------------
//----  Color Object
//----------------------------------------------------
int ColorScriptObject::numInstances = 0;

ColorScriptObject::ColorScriptObject(int r, int g, int b)
	: QObject(), color(r, g, b), _palette(0)
{
	numInstances++;
	//printf("ColorScriptObject(r,g,b) %p Constructor: %i\n", this, numInstances);

	moveToThread(QApplication::instance()->thread());
}
//----------------------------------------------------
ColorScriptObject::~ColorScriptObject()
{
	numInstances--;
	//printf("ColorScriptObject %p Destructor: %i\n", this, numInstances);
}
//----------------------------------------------------
//----  File Object
//----------------------------------------------------
int FileScriptObject::numInstances = 0;

FileScriptObject::FileScriptObject(const QString& path)
	: QObject()
{
	numInstances++;
	//printf("FileScriptObject(%s) %p Constructor: %i\n", path.toLocal8Bit().constData(), this, numInstances);

	moveToThread(QApplication::instance()->thread());

	setFilePath(path);
}
//----------------------------------------------------
FileScriptObject::~FileScriptObject()
{
	close();

	numInstances--;
	//printf("FileScriptObject %p Destructor: %i\n", this, numInstances);
}
//----------------------------------------------------
void FileScriptObject::setTemporary(bool value)
{
	tmp = value;
}
//----------------------------------------------------
void FileScriptObject::setFilePath(const QString& path)
{
	if (isOpen())
	{
		auto* engine = FCEU::JSEngine::getCurrent();
		engine->throwError(QJSValue::GenericError, "cannot change file path while open");
	}
	QFileInfo fi(path);
	filepath = fi.absoluteFilePath();
	//printf("FileScriptObject::filepath(%s)\n", filepath.toLocal8Bit().constData());
}
//----------------------------------------------------
QString FileScriptObject::fileName()
{
	QFileInfo fi(filepath);
	return fi.fileName();
}
//----------------------------------------------------
QString FileScriptObject::fileSuffix()
{
	QFileInfo fi(filepath);
	return fi.completeSuffix();
}
//----------------------------------------------------
bool FileScriptObject::open(int mode)
{
	close();

	if (filepath.isEmpty())
	{
		auto* engine = FCEU::JSEngine::getCurrent();
		engine->throwError(QJSValue::GenericError, "unspecified file path ");
		return false;
	}

	if (tmp)
	{
		file = new QTemporaryFile();
	}
	else
	{
		file = new QFile(filepath);
	}

	if (file == nullptr)
	{
		auto* engine = FCEU::JSEngine::getCurrent();
		engine->throwError(QJSValue::GenericError, "failed to create QFile object");
		return false;
	}

	QIODevice::OpenMode deviceMode = QIODevice::NotOpen;

	if (mode & ReadOnly)
	{
		deviceMode |= QIODevice::ReadOnly;
	}
	if (mode & WriteOnly)
	{
		deviceMode |= QIODevice::WriteOnly;
	}
	if (mode & Append)
	{
		deviceMode |= QIODevice::Append;
	}
	if (mode & Truncate)
	{
		deviceMode |= QIODevice::Truncate;
	}

	bool success = file->open(deviceMode);

	//printf("FileOpen: %i\n", success);

	return success;
}
//----------------------------------------------------
bool FileScriptObject::isOpen()
{
	bool flag = false;

	if (file != nullptr)
	{
		flag = file->isOpen();
	}
	return flag;
}
//----------------------------------------------------
bool FileScriptObject::isReadable()
{
	bool flag = false;

	if (file != nullptr)
	{
		flag = file->isReadable();
	}
	return flag;
}
//----------------------------------------------------
bool FileScriptObject::isWritable()
{
	bool flag = false;

	if (file != nullptr)
	{
		flag = file->isWritable();
	}
	return flag;
}
//----------------------------------------------------
bool FileScriptObject::atEnd()
{
	bool retval = false;

	if (file != nullptr)
	{
		retval = file->atEnd();
	}
	return retval;
}
//----------------------------------------------------
bool FileScriptObject::truncate()
{
	bool retval = false;

	if (file != nullptr)
	{
		retval = file->resize(0);
	}
	return retval;
}
//----------------------------------------------------
bool FileScriptObject::resize(int64_t size)
{
	bool retval = false;

	if (file != nullptr)
	{
		retval = file->resize(size);
	}
	return retval;
}
//----------------------------------------------------
int64_t FileScriptObject::skip(int64_t size)
{
	int64_t retval = 0;

	if (file != nullptr)
	{
		retval = file->skip(size);
	}
	return retval;
}
//----------------------------------------------------
bool FileScriptObject::seek(int64_t pos)
{
	bool retval = false;

	if (file != nullptr)
	{
		retval = file->seek(pos);
	}
	return retval;
}
//----------------------------------------------------
int64_t FileScriptObject::pos()
{
	int64_t retval = 0;

	if (file != nullptr)
	{
		retval = file->pos();
	}
	return retval;
}
//----------------------------------------------------
int64_t FileScriptObject::bytesAvailable()
{
	int64_t retval = 0;

	if (file != nullptr)
	{
		retval = file->bytesAvailable();
	}
	return retval;
}
//----------------------------------------------------
bool FileScriptObject::flush()
{
	bool retval = false;

	if (file != nullptr)
	{
		retval = file->flush();
	}
	return retval;
}
//----------------------------------------------------
void FileScriptObject::close()
{
	if (file != nullptr)
	{
		if (file->isOpen())
		{
			file->close();
		}
		if (tmp)
		{
			file->remove();
		}
		delete file;
		file = nullptr;
	}
}
//----------------------------------------------------
int  FileScriptObject::writeString(const QString& s)
{
	if ( (file == nullptr) || !file->isOpen())
	{
		auto* engine = FCEU::JSEngine::getCurrent();
		engine->throwError(QJSValue::GenericError, "file is not open ");
		return -1;
	}
	int bytesWritten = file->write( s.toLocal8Bit() );

	return bytesWritten;
}
//----------------------------------------------------
int  FileScriptObject::writeData(const QByteArray& s)
{
	if ( (file == nullptr) || !file->isOpen())
	{
		auto* engine = FCEU::JSEngine::getCurrent();
		engine->throwError(QJSValue::GenericError, "file is not open ");
		return -1;
	}
	int bytesWritten = file->write( s );

	return bytesWritten;
}
//----------------------------------------------------
QString FileScriptObject::readLine()
{
	QString line;

	if ( (file == nullptr) || !file->isOpen())
	{
		auto* engine = FCEU::JSEngine::getCurrent();
		engine->throwError(QJSValue::GenericError, "file is not open ");
		return line;
	}
	QByteArray byteArray = file->readLine();

	line = QString::fromLocal8Bit(byteArray);

	//printf("ReadLine: %s\n", line.toLocal8Bit().constData());

	return line;
}
//----------------------------------------------------
QByteArray FileScriptObject::readData(unsigned int maxBytes)
{
	QJSValue arrayBuffer;
	QByteArray byteArray;

	auto* engine = FCEU::JSEngine::getCurrent();

	if ( (file == nullptr) || !file->isOpen())
	{
		engine->throwError(QJSValue::GenericError, "file is not open ");
		return byteArray;
	}
	if (maxBytes > 0)
	{
		byteArray = file->read(maxBytes);
	}
	else
	{
		byteArray = file->readAll();
	}

	//arrayBuffer = engine->toScriptValue(byteArray);

	return byteArray;
}
//----------------------------------------------------
QByteArray FileScriptObject::peekData(unsigned int maxBytes)
{
	QJSValue arrayBuffer;
	QByteArray byteArray;

	auto* engine = FCEU::JSEngine::getCurrent();

	if ( (file == nullptr) || !file->isOpen())
	{
		engine->throwError(QJSValue::GenericError, "file is not open ");
		return byteArray;
	}
	byteArray = file->peek(maxBytes);

	//arrayBuffer = engine->toScriptValue(byteArray);

	return byteArray;
}
//----------------------------------------------------
bool FileScriptObject::putChar(char c)
{
	if ( (file == nullptr) || !file->isOpen())
	{
		auto* engine = FCEU::JSEngine::getCurrent();
		engine->throwError(QJSValue::GenericError, "file is not open ");
		return false;
	}
	bool success = file->putChar(c);

	return success;
}
//----------------------------------------------------
char FileScriptObject::getChar()
{
	if ( (file == nullptr) || !file->isOpen())
	{
		auto* engine = FCEU::JSEngine::getCurrent();
		engine->throwError(QJSValue::GenericError, "file is not open ");
		return -1;
	}
	char c = -1;
	bool success = file->getChar(&c);

	if (!success)
	{
		c = -1;
	}
	return c;
}
//----------------------------------------------------
//----  Joypad Object
//----------------------------------------------------
int JoypadScriptObject::numInstances = 0;
/* Joypad Override Logic True Table
	11 - true		01 - pass-through (default)
	00 - false		10 - invert		*/
uint8_t JoypadScriptObject::jsOverrideMask1[MAX_JOYPAD_PLAYERS]= { 0xFF, 0xFF, 0xFF, 0xFF };
uint8_t JoypadScriptObject::jsOverrideMask2[MAX_JOYPAD_PLAYERS]= { 0x00, 0x00, 0x00, 0x00 };

JoypadScriptObject::JoypadScriptObject(int playerIdx, bool immediate)
	: QObject()
{
	numInstances++;
	//printf("JoypadScriptObject %p Constructor: %i\n", this, numInstances);

	moveToThread(QApplication::instance()->thread());

	if ( (playerIdx < 0) || (playerIdx >= MAX_JOYPAD_PLAYERS) )
	{
		QString msg = "Error: Joypad player index (" + QString::number(playerIdx) + ") is out of bounds!\n";
		auto* engine = FCEU::JSEngine::getCurrent();
		if (engine != nullptr)
		{
			engine->throwError(QJSValue::RangeError, msg);
		}
		playerIdx = 0;
	}

	player = playerIdx;

	refreshData(immediate);
}
//----------------------------------------------------
JoypadScriptObject::~JoypadScriptObject()
{
	numInstances--;
	//printf("JoypadScriptObject %p Destructor: %i\n", this, numInstances);
}
//----------------------------------------------------
uint8_t JoypadScriptObject::readOverride(int which, uint8_t joyl)
{
	joyl = (joyl & jsOverrideMask1[which]) | (~joyl & jsOverrideMask2[which]);
	jsOverrideMask1[which] = 0xFF;
	jsOverrideMask2[which] = 0x00;
	return joyl;

}
//----------------------------------------------------
void JoypadScriptObject::refreshData(bool immediate)
{
	uint8_t buttons = 0;
	if (immediate)
	{
		uint32_t gpData = GetGamepadPressedImmediate();
		buttons = gpData >> (player * 8);
	}
	else
	{
		buttons = joy[player];
	}
	prev = current;

	current.buttonMask = buttons;
	current._immediate = immediate;
}
//----------------------------------------------------
bool JoypadScriptObject::getButton(enum Button b)
{
	bool isPressed = false;
	uint8_t mask = 0x01 << b;

	//printf("mask=%08x  buttons=%08x\n", mask, current.buttonMask);
	isPressed = (current.buttonMask & mask) ? true : false;
	return isPressed;
}
//----------------------------------------------------
bool JoypadScriptObject::buttonChanged(enum Button b)
{
	bool hasChanged = false;
	uint8_t mask = 0x01 << b;

	//printf("mask=%08x  buttons=%08x\n", mask, current.buttonMask);
	hasChanged = ((current.buttonMask ^ prev.buttonMask) & mask) ? true : false;
	return hasChanged;
}
//----------------------------------------------------
Q_INVOKABLE void JoypadScriptObject::ovrdResetAll()
{
	for (int i=0; i<MAX_JOYPAD_PLAYERS; i++)
	{
		jsOverrideMask1[i] = 0xFF;
		jsOverrideMask2[i] = 0x00;
	}
}
//----------------------------------------------------
//----  EMU State Object
//----------------------------------------------------
int EmuStateScriptObject::numInstances = 0;

//----------------------------------------------------
EmuStateScriptObject::EmuStateScriptObject(const QJSValue& jsArg1, const QJSValue& jsArg2)
{
	numInstances++;
	//printf("EmuStateScriptObject %p JS Constructor: %i\n", this, numInstances);

	moveToThread(QApplication::instance()->thread());
	QJSValueList args = { jsArg1, jsArg2 };

	for (auto& jsVal : args)
	{
		if (jsVal.isObject())
		{
			//printf("EmuStateScriptObject %p JS Constructor(Obj): %i\n", this, numInstances);
			auto obj = qobject_cast<EmuStateScriptObject*>(jsVal.toQObject());

			if (obj != nullptr)
			{
				*this = *obj;
			}
		}
		else if (jsVal.isNumber())
		{
			//printf("EmuStateScriptObject %p JS Constructor(int): %i\n", this, numInstances);
			setSlot(jsVal.toInt());

			if ( (slot >= 0) && saveFileExists())
			{
				loadFromFile(filename);
			}
		}
	}
}
//----------------------------------------------------
EmuStateScriptObject::~EmuStateScriptObject()
{
	if (data != nullptr)
	{
		if (persist)
		{
			saveToFile(filename);
		}
		delete data;
		data = nullptr;
	}
	numInstances--;
	//printf("EmuStateScriptObject %p Destructor: %i\n", this, numInstances);
}
//----------------------------------------------------
EmuStateScriptObject& EmuStateScriptObject::operator= (const EmuStateScriptObject& obj)
{
	setSlot( obj.slot );
	persist = obj.persist;
	compression = obj.compression;
	filename = obj.filename;

	//printf("EmuStateScriptObject Copy Assignment: %p\n", this);

	if (obj.data != nullptr)
	{
		data = new EMUFILE_MEMORY(obj.data->size());
		memcpy( data->buf(), obj.data->buf(), obj.data->size());
	}
	return *this;
}
//----------------------------------------------------
void EmuStateScriptObject::setSlot(int value)
{
	slot = value;

	if (slot >= 0)
	{
		slot = slot % 10;

		std::string fileString = FCEU_MakeFName(FCEUMKF_STATE, slot, 0);

		filename = QString::fromStdString(fileString);
	}
}
//----------------------------------------------------
bool EmuStateScriptObject::save()
{
	if (data != nullptr)
	{
		delete data;
		data = nullptr;
	}
	data = new EMUFILE_MEMORY();

	FCEU_WRAPPER_LOCK();
	FCEUSS_SaveMS( data, compression);
	data->fseek(0,SEEK_SET);
	FCEU_WRAPPER_UNLOCK();

	if (persist)
	{
		saveToFile(filename);
	}
	return true;
}
//----------------------------------------------------
bool EmuStateScriptObject::load()
{
	bool loaded = false;
	if (data != nullptr)
	{
		FCEU_WRAPPER_LOCK();
		if (FCEUSS_LoadFP( data, SSLOADPARAM_NOBACKUP))
		{
			data->fseek(0,SEEK_SET);
			loaded = true;
		}
		FCEU_WRAPPER_UNLOCK();
	}
	return loaded;
}
//----------------------------------------------------
bool EmuStateScriptObject::saveToFile(const QString& filepath)
{
	if (filepath.isEmpty())
	{
		return false;
	}
	if (data == nullptr)
	{
		return false;
	}
	FILE* outf = fopen(filepath.toLocal8Bit().constData(),"wb");
	if (outf == nullptr)
	{
		return false;
	}
	fwrite( data->buf(), 1, data->size(), outf);
	fclose(outf);
	return true;
}
//----------------------------------------------------
bool EmuStateScriptObject::loadFromFile(const QString& filepath)
{
	if (filepath.isEmpty())
	{
		return false;
	}
	if (data != nullptr)
	{
		delete data;
		data = nullptr;
	}
	FILE* inf = fopen(filepath.toLocal8Bit().constData(),"rb");
	if (inf == nullptr)
	{
		QString msg = "JS EmuState::loadFromFile failed to open file: " + filepath;
		logMessage(FCEU::JSEngine::WARNING, msg);
		return false;
	}
	fseek(inf,0,SEEK_END);
	long int len = ftell(inf);
	fseek(inf,0,SEEK_SET);
	data = new EMUFILE_MEMORY(len);
	if ( fread(data->buf(),1,len,inf) != static_cast<size_t>(len) )
	{
		QString msg = "JS EmuState::loadFromFile failed to load full buffer.";
		logMessage(FCEU::JSEngine::WARNING, msg);
		delete data;
		data = nullptr;
	}
	fclose(inf);
	return true;
}
//----------------------------------------------------
void EmuStateScriptObject::logMessage(int lvl, QString& msg)
{
	auto* engine = FCEU::JSEngine::getCurrent();

	if (engine != nullptr)
	{
		engine->logMessage(lvl, msg);
	}
}
//----------------------------------------------------
bool EmuStateScriptObject::saveFileExists()
{
	bool exists = false;
	QFileInfo fileInfo(filename);

	if (fileInfo.exists() && fileInfo.isFile())
	{
		exists = true;
	}
	return exists;
}
//----------------------------------------------------
QJSValue EmuStateScriptObject::copy()
{
	QJSValue jsVal;
	auto* engine = FCEU::JSEngine::getCurrent();

	if (engine != nullptr)
	{
		EmuStateScriptObject* emuStateObj = new EmuStateScriptObject();

		if (emuStateObj != nullptr)
		{
			*emuStateObj = *this;

			jsVal = engine->newQObject(emuStateObj);

#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
			QJSEngine::setObjectOwnership( emuStateObj, QJSEngine::JavaScriptOwnership);
#endif
		}
	}
	return jsVal;
}
//----------------------------------------------------
//----  EMU Script Object
//----------------------------------------------------
EmuScriptObject::EmuScriptObject(QObject* parent)
	: QObject(parent)
{
	script = qobject_cast<QtScriptInstance*>(parent);
}
//----------------------------------------------------
EmuScriptObject::~EmuScriptObject()
{
}
//----------------------------------------------------
void EmuScriptObject::print(const QString& msg)
{
	if (dialog != nullptr)
	{
		dialog->logOutput(msg);
	}
}
//----------------------------------------------------
void EmuScriptObject::powerOn()
{
	fceuWrapperHardReset();
}
//----------------------------------------------------
void EmuScriptObject::softReset()
{
	fceuWrapperSoftReset();
}
//----------------------------------------------------
void EmuScriptObject::pause()
{
	FCEUI_SetEmulationPaused( EMULATIONPAUSED_PAUSED );
}
//----------------------------------------------------
void EmuScriptObject::unpause()
{
	FCEUI_SetEmulationPaused(0);
}
//----------------------------------------------------
bool EmuScriptObject::paused()
{
	return FCEUI_EmulationPaused() != 0;
}
//----------------------------------------------------
void EmuScriptObject::frameAdvance()
{
	script->frameAdvance();
}
//----------------------------------------------------
int EmuScriptObject::frameCount()
{
	return FCEUMOV_GetFrame();
}
//----------------------------------------------------
int EmuScriptObject::lagCount()
{
	return FCEUI_GetLagCount();
}
//----------------------------------------------------
bool EmuScriptObject::lagged()
{
	return FCEUI_GetLagged();
}
//----------------------------------------------------
void EmuScriptObject::setLagFlag(bool flag)
{
	FCEUI_SetLagFlag(flag);
}
//----------------------------------------------------
bool EmuScriptObject::emulating()
{
	return (GameInfo != nullptr);
}
//----------------------------------------------------
bool EmuScriptObject::isReadOnly()
{
	return FCEUI_GetMovieToggleReadOnly();
}
//----------------------------------------------------
void EmuScriptObject::setReadOnly(bool flag)
{
	FCEUI_SetMovieToggleReadOnly(flag);
}
//----------------------------------------------------
void EmuScriptObject::setRenderPlanes(bool sprites, bool background)
{
	FCEUI_SetRenderPlanes(sprites, background);
}
//----------------------------------------------------
void EmuScriptObject::exit()
{
	fceuWrapperRequestAppExit();
}
//----------------------------------------------------
void EmuScriptObject::message(const QString& msg)
{
	FCEU_DispMessage("%s",0, msg.toLocal8Bit().constData());
}
//----------------------------------------------------
void EmuScriptObject::speedMode(const QString& mode)
{
	int speed = EMUSPEED_NORMAL;
	bool useTurbo = false;

	if (mode.contains("normal", Qt::CaseInsensitive))
	{
		speed = EMUSPEED_NORMAL;
	}
	else if (mode.contains("nothrottle", Qt::CaseInsensitive))
	{
		useTurbo = true;
	}
	else if (mode.contains("turbo", Qt::CaseInsensitive))
	{
		useTurbo = true;
	}
	else if (mode.contains("maximum", Qt::CaseInsensitive))
	{
		speed = EMUSPEED_FASTEST;
	}
	else
	{
		QString msg = "Invalid mode argument \"" + mode + "\" to emu.speedmode\n";
		script->throwError(QJSValue::TypeError, msg);
		return;
	}

	if (useTurbo)
	{
		FCEUD_TurboOn();
	}
	else
	{
		FCEUD_TurboOff();
	}
	FCEUD_SetEmulationSpeed(speed);
}
//----------------------------------------------------
bool EmuScriptObject::addGameGenie(const QString& code)
{
	// Add a Game Genie code if it hasn't already been added
	int GGaddr, GGcomp, GGval;
	int i=0;

	uint32 Caddr;
	uint8 Cval;
	int Ccompare, Ctype;

	if (!FCEUI_DecodeGG(code.toLocal8Bit().constData(), &GGaddr, &GGval, &GGcomp))
	{
		print("Failed to decode game genie code");
		return false;
	}

	while (FCEUI_GetCheat(i,NULL,&Caddr,&Cval,&Ccompare,NULL,&Ctype))
	{
		if ((static_cast<uint32>(GGaddr) == Caddr) && (GGval == static_cast<int>(Cval)) && (GGcomp == Ccompare) && (Ctype == 1))
		{
			// Already Added, so consider it a success
			return true;
		}
		i = i + 1;
	}

	if (FCEUI_AddCheat(code.toLocal8Bit().constData(),GGaddr,GGval,GGcomp,1))
	{
		// Code was added
		// Can't manage the display update the way I want, so I won't bother with it
		// UpdateCheatsAdded();
		return true;
	}
	else
	{
		// Code didn't get added
	}
	return false;
}
//----------------------------------------------------
bool EmuScriptObject::delGameGenie(const QString& code)
{
	// Remove a Game Genie code. Very restrictive about deleted code.
	int GGaddr, GGcomp, GGval;
	uint32 i=0;

	std::string Cname;
	uint32 Caddr;
	uint8 Cval;
	int Ccompare, Ctype;

	if (!FCEUI_DecodeGG(code.toLocal8Bit().constData(), &GGaddr, &GGval, &GGcomp))
	{
		print("Failed to decode game genie code");
		return false;
	}

	while (FCEUI_GetCheat(i,&Cname,&Caddr,&Cval,&Ccompare,NULL,&Ctype))
	{
		QString name = QString::fromStdString(Cname);

		if ((code == name) && (static_cast<uint32>(GGaddr) == Caddr) && (GGval == static_cast<int>(Cval)) && (GGcomp == Ccompare) && (Ctype == 1))
		{
			// Delete cheat code
			if (FCEUI_DelCheat(i))
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		i = i + 1;
	}
	// Cheat didn't exist, so it's not an error
	return true;
}
//----------------------------------------------------
void EmuScriptObject::registerBeforeFrame(const QJSValue& func)
{
	script->registerBeforeEmuFrame(func);
}
//----------------------------------------------------
void EmuScriptObject::registerAfterFrame(const QJSValue& func)
{
	script->registerAfterEmuFrame(func);
}
//----------------------------------------------------
void EmuScriptObject::registerStop(const QJSValue& func)
{
	script->registerStop(func);
}
//----------------------------------------------------
bool EmuScriptObject::loadRom(const QString& romPath)
{
	int ret = 0;

	if (!romPath.isEmpty())
	{
		ret = LoadGame(romPath.toLocal8Bit().constData());
	}
	return ret != 0;
}
//----------------------------------------------------
bool EmuScriptObject::onEmulationThread()
{
	bool isEmuThread = (consoleWindow != nullptr) && 
		(QThread::currentThread() == consoleWindow->emulatorThread);
	return isEmuThread;
}
//----------------------------------------------------
QString EmuScriptObject::getDir()
{
	return QString(fceuExecutablePath());
}
//----------------------------------------------------
QJSValue EmuScriptObject::getScreenPixel(int x, int y, bool useBackup)
{
	int r,g,b,p;
	uint32_t pixel = GetScreenPixel(x,y,useBackup);
	r = JS_PIXEL_R(pixel);
	g = JS_PIXEL_G(pixel);
	b = JS_PIXEL_B(pixel);

	p = GetScreenPixelPalette(x,y,useBackup);

	ColorScriptObject* pixelObj = new ColorScriptObject(r, g, b);

	pixelObj->setPalette(p);

	QJSValue jsVal = engine->newQObject(pixelObj);

#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
	QJSEngine::setObjectOwnership( pixelObj, QJSEngine::JavaScriptOwnership);
#endif

	return jsVal;
}
//----------------------------------------------------
QJSValue EmuScriptObject::createState(int slot)
{
	QJSValue jsVal;
	EmuStateScriptObject* emuStateObj = new EmuStateScriptObject(slot);

	if (emuStateObj != nullptr)
	{
		jsVal = engine->newQObject(emuStateObj);

#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
		QJSEngine::setObjectOwnership( emuStateObj, QJSEngine::JavaScriptOwnership);
#endif
	}
	return jsVal;
}
//----------------------------------------------------
//----  ROM Script Object
//----------------------------------------------------
//----------------------------------------------------
RomScriptObject::RomScriptObject(QObject* parent)
	: QObject(parent)
{
	script = qobject_cast<QtScriptInstance*>(parent);
}
//----------------------------------------------------
RomScriptObject::~RomScriptObject()
{
}
//----------------------------------------------------
bool RomScriptObject::isLoaded()
{
	return (GameInfo != nullptr);
}
//----------------------------------------------------
QString RomScriptObject::getFileName()
{
	QString baseName;

	if (GameInfo != nullptr)
	{
		baseName = FileBase;
	}
	return baseName;
}
//----------------------------------------------------
QString RomScriptObject::getHash(const QString& type)
{
	QString hash;

	if (GameInfo != nullptr)
	{
		MD5DATA md5hash = GameInfo->MD5;

		if (type.compare("md5", Qt::CaseInsensitive) == 0)
		{
			hash = md5_asciistr(md5hash);
		}
		else if (type.compare("base64", Qt::CaseInsensitive) == 0)
		{
			hash = BytesToString(md5hash.data, MD5DATA::size).c_str();
		}
	}
	return hash;
}
//----------------------------------------------------
uint8_t RomScriptObject::readByte(int address)
{
	return FCEU_ReadRomByte(address);
}
//----------------------------------------------------
uint8_t RomScriptObject::readByteUnsigned(int address)
{
	return FCEU_ReadRomByte(address);
}
//----------------------------------------------------
int8_t RomScriptObject::readByteSigned(int address)
{
	return static_cast<int8_t>(FCEU_ReadRomByte(address));
}
//----------------------------------------------------
QJSValue RomScriptObject::readByteRange(int start, int end)
{
	QJSValue array;
	int size = end - start + 1;

	if (size > 0)
	{
		array = engine->newArray(size);

		for (int i=0; i<size; i++)
		{
			int byte = FCEU_ReadRomByte(start + i);

			QJSValue element = byte;

			array.setProperty(i, element);
		}
	}
	return array;
}
//----------------------------------------------------
void RomScriptObject::writeByte(int address, int value)
{
	if (address < 16)
	{
		script->print("rom.writebyte() can't edit the ROM header.");
	}
	else
	{
		FCEU_WriteRomByte(address, value);
	}
}
//----------------------------------------------------
//----  PPU Script Object
//----------------------------------------------------
//----------------------------------------------------
PpuScriptObject::PpuScriptObject(QObject* parent)
	: QObject(parent)
{
	script = qobject_cast<QtScriptInstance*>(parent);
}
//----------------------------------------------------
PpuScriptObject::~PpuScriptObject()
{
}
//----------------------------------------------------
uint8_t PpuScriptObject::readByte(int address)
{
	uint8_t byte = 0;
	if (FFCEUX_PPURead != nullptr)
	{
		byte = FFCEUX_PPURead(address);
	}
	return byte;
}
//----------------------------------------------------
uint8_t PpuScriptObject::readByteUnsigned(int address)
{
	uint8_t byte = 0;
	if (FFCEUX_PPURead != nullptr)
	{
		byte = FFCEUX_PPURead(address);
	}
	return byte;
}
//----------------------------------------------------
int8_t PpuScriptObject::readByteSigned(int address)
{
	int8_t byte = 0;
	if (FFCEUX_PPURead != nullptr)
	{
		byte = static_cast<int8_t>(FFCEUX_PPURead(address));
	}
	return byte;
}
//----------------------------------------------------
QJSValue PpuScriptObject::readByteRange(int start, int end)
{
	QJSValue array;
	int size = end - start + 1;

	if (FFCEUX_PPURead == nullptr)
	{
		return array;
	}

	if (size > 0)
	{
		array = engine->newArray(size);

		for (int i=0; i<size; i++)
		{
			int byte = FFCEUX_PPURead(start + i);

			QJSValue element = byte;

			array.setProperty(i, element);
		}
	}
	return array;
}
//----------------------------------------------------
void PpuScriptObject::writeByte(int address, int value)
{
	if (FFCEUX_PPUWrite != nullptr)
	{
		FFCEUX_PPUWrite(address, value);
	}
}
//----------------------------------------------------
//----  Movie Script Object
//----------------------------------------------------
//----------------------------------------------------
MovieScriptObject::MovieScriptObject(QObject* parent)
	: QObject(parent)
{
	script = qobject_cast<QtScriptInstance*>(parent);
	engine = script->getEngine();
}
//----------------------------------------------------
MovieScriptObject::~MovieScriptObject()
{
}
//----------------------------------------------------
bool MovieScriptObject::active()
{
	bool movieActive = (FCEUMOV_IsRecording() || FCEUMOV_IsPlaying());
	return movieActive;
}
//----------------------------------------------------
bool MovieScriptObject::isPlaying()
{
	bool playing = FCEUMOV_IsPlaying();
	return playing;
}
//----------------------------------------------------
bool MovieScriptObject::isRecording()
{
	bool recording = FCEUMOV_IsRecording();
	return recording;
}
//----------------------------------------------------
bool MovieScriptObject::isPowerOn()
{
	bool flag = false;
	if (FCEUMOV_IsRecording() || FCEUMOV_IsPlaying())
	{
		flag = FCEUMOV_FromPoweron();
	}
	return flag;
}
//----------------------------------------------------
bool MovieScriptObject::isFromSaveState()
{
	bool flag = false;
	if (FCEUMOV_IsRecording() || FCEUMOV_IsPlaying())
	{
		flag = !FCEUMOV_FromPoweron();
	}
	return flag;
}
//----------------------------------------------------
void MovieScriptObject::replay()
{
	FCEUI_MoviePlayFromBeginning();
}
//----------------------------------------------------
bool MovieScriptObject::getReadOnly()
{
	return FCEUI_GetMovieToggleReadOnly();
}
//----------------------------------------------------
void MovieScriptObject::setReadOnly(bool which)
{
	FCEUI_SetMovieToggleReadOnly(which);
}
//----------------------------------------------------
int MovieScriptObject::mode()
{
	int _mode = IDLE;
	if (FCEUMOV_Mode(MOVIEMODE_TASEDITOR))
	{
		_mode = TAS_EDITOR;
	}
	else if (FCEUMOV_IsRecording())
	{
		_mode = RECORD;
	}
	else if (FCEUMOV_IsFinished())
	{
		_mode = FINISHED; //Note: this comes before playback since playback checks for finished as well
	}
	else if (FCEUMOV_IsPlaying())
	{
		_mode = PLAYBACK;
	}
	else
	{
		_mode = IDLE;
	}
	return _mode;
}
//----------------------------------------------------
void MovieScriptObject::stop()
{
	FCEUI_StopMovie();
}
//----------------------------------------------------
int MovieScriptObject::frameCount()
{
	return FCEUMOV_GetFrame();
}
//----------------------------------------------------
int MovieScriptObject::length()
{
	if (!FCEUMOV_IsRecording() && !FCEUMOV_IsPlaying() && !FCEUMOV_Mode(MOVIEMODE_TASEDITOR))
	{
		script->throwError(QJSValue::GenericError, "No movie loaded.");
	}
	return FCEUI_GetMovieLength();
}
//----------------------------------------------------
int MovieScriptObject::rerecordCount()
{
	if (!FCEUMOV_IsRecording() && !FCEUMOV_IsPlaying() && !FCEUMOV_Mode(MOVIEMODE_TASEDITOR))
	{
		script->throwError(QJSValue::GenericError, "No movie loaded.");
	}
	return FCEUI_GetMovieRerecordCount();
}
//----------------------------------------------------
QString MovieScriptObject::getFilepath()
{
	if (!FCEUMOV_IsRecording() && !FCEUMOV_IsPlaying() && !FCEUMOV_Mode(MOVIEMODE_TASEDITOR))
	{
		script->throwError(QJSValue::GenericError, "No movie loaded.");
	}
	return QString::fromStdString(FCEUI_GetMovieName());
}
//----------------------------------------------------
QString MovieScriptObject::getFilename()
{
	if (!FCEUMOV_IsRecording() && !FCEUMOV_IsPlaying() && !FCEUMOV_Mode(MOVIEMODE_TASEDITOR))
	{
		script->throwError(QJSValue::GenericError, "No movie loaded.");
	}
	QFileInfo fi( QString::fromStdString(FCEUI_GetMovieName()) );
	return fi.fileName();
}
//----------------------------------------------------
bool MovieScriptObject::skipRerecords = false;
//----------------------------------------------------
void MovieScriptObject::rerecordCounting(bool counting)
{
	skipRerecords = counting;
}
//----------------------------------------------------
bool MovieScriptObject::play(const QString& filename, bool readOnly, int pauseFrame)
{
	if (pauseFrame < 0) pauseFrame = 0;

	// Load it!
	bool loaded = FCEUI_LoadMovie(filename.toLocal8Bit().constData(), readOnly, pauseFrame);

	return loaded;
}
//----------------------------------------------------
bool MovieScriptObject::record(const QString& filename, int saveType, const QString author)
{
	if (filename.isEmpty())
	{
		script->throwError(QJSValue::GenericError, "movie.record(): Filename required");
		return false;
	}

	// No need to use the full functionality of the enum
	EMOVIE_FLAG flags;
	if      (saveType == FROM_SAVESTATE) flags = MOVIE_FLAG_NONE;  // from savestate
	else if (saveType == FROM_SAVERAM  ) flags = MOVIE_FLAG_FROM_SAVERAM;
	else                                 flags = MOVIE_FLAG_FROM_POWERON;

	// Save it!
	FCEUI_SaveMovie( filename.toLocal8Bit().constData(), flags, author.toStdWString());

	return true;
}
//----------------------------------------------------
//----  Input Script Object
//----------------------------------------------------
//----------------------------------------------------
InputScriptObject::InputScriptObject(QObject* parent)
	: QObject(parent)
{
	script = qobject_cast<QtScriptInstance*>(parent);
	engine = script->getEngine();
}
//----------------------------------------------------
InputScriptObject::~InputScriptObject()
{
}
//----------------------------------------------------
QJSValue InputScriptObject::readJoypad(int player, bool immediate)
{
	JoypadScriptObject* joypadObj = new JoypadScriptObject(player, immediate);

	QJSValue jsObj = engine->newQObject( joypadObj );

#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
	QJSEngine::setObjectOwnership( joypadObj, QJSEngine::JavaScriptOwnership);
#endif

	return jsObj;
}
//----------------------------------------------------
//----  Memory Script Object
//----------------------------------------------------
//----------------------------------------------------
static void addressReadCallback(unsigned int address, unsigned int value, void *userData)
{
	MemoryScriptObject* mem = static_cast<MemoryScriptObject*>(userData);

	if (mem != nullptr)
	{
		QJSValue* func = mem->getReadFunc(address);

		if (func != nullptr)
		{
			QJSValueList args = { address, value };

			mem->getScript()->runFunc( *func, args);
		}
	}
}
//----------------------------------------------------
static void addressWriteCallback(unsigned int address, unsigned int value, void *userData)
{
	MemoryScriptObject* mem = static_cast<MemoryScriptObject*>(userData);

	if (mem != nullptr)
	{
		QJSValue* func = mem->getWriteFunc(address);

		if (func != nullptr)
		{
			QJSValueList args = { address, value };

			mem->getScript()->runFunc( *func, args);
		}
	}
}
//----------------------------------------------------
static void addressExecCallback(unsigned int address, unsigned int value, void *userData)
{
	MemoryScriptObject* mem = static_cast<MemoryScriptObject*>(userData);

	if (mem != nullptr)
	{
		QJSValue* func = mem->getExecFunc(address);

		if (func != nullptr)
		{
			QJSValueList args = { address, value };

			mem->getScript()->runFunc( *func, args);
		}
	}
}
//----------------------------------------------------
MemoryScriptObject::MemoryScriptObject(QObject* parent)
	: QObject(parent)
{
	script = qobject_cast<QtScriptInstance*>(parent);
}
//----------------------------------------------------
MemoryScriptObject::~MemoryScriptObject()
{
	unregisterAll();
}
//----------------------------------------------------
void MemoryScriptObject::reset()
{
	unregisterAll();
}
//----------------------------------------------------
uint8_t MemoryScriptObject::readByte(int address)
{
	return GetMem(address);
}
//----------------------------------------------------
uint8_t MemoryScriptObject::readByteUnsigned(int address)
{
	return GetMem(address);
}
//----------------------------------------------------
int8_t MemoryScriptObject::readByteSigned(int address)
{
	return static_cast<int8_t>(GetMem(address));
}
//----------------------------------------------------
uint16_t MemoryScriptObject::readWord(int addressLow, int addressHigh)
{
	// little endian, unless the high byte address is specified as a 2nd parameter
	if (addressHigh < 0)
	{
		addressHigh = addressLow + 1;
	}
	uint16_t result = GetMem(addressLow) | (GetMem(addressHigh) << 8);
	return result;
}
//----------------------------------------------------
uint16_t MemoryScriptObject::readWordUnsigned(int addressLow, int addressHigh)
{
	// little endian, unless the high byte address is specified as a 2nd parameter
	if (addressHigh < 0)
	{
		addressHigh = addressLow + 1;
	}
	uint16_t result = GetMem(addressLow) | (GetMem(addressHigh) << 8);
	return result;
}
//----------------------------------------------------
int16_t MemoryScriptObject::readWordSigned(int addressLow, int addressHigh)
{
	// little endian, unless the high byte address is specified as a 2nd parameter
	if (addressHigh < 0)
	{
		addressHigh = addressLow + 1;
	}
	uint16_t result = GetMem(addressLow) | (GetMem(addressHigh) << 8);
	return static_cast<int16_t>(result);
}
//----------------------------------------------------
void MemoryScriptObject::writeByte(int address, int value)
{
	uint32_t A = address;
	uint8_t  V = value;

	if (A < 0x10000)
	{
		BWrite[A](A, V);
	}
}
//----------------------------------------------------
uint16_t MemoryScriptObject::getRegisterPC()
{
	return X.PC;
}
//----------------------------------------------------
uint8_t MemoryScriptObject::getRegisterA()
{
	return X.A;
}
//----------------------------------------------------
uint8_t MemoryScriptObject::getRegisterX()
{
	return X.X;
}
//----------------------------------------------------
uint8_t MemoryScriptObject::getRegisterY()
{
	return X.Y;
}
//----------------------------------------------------
uint8_t MemoryScriptObject::getRegisterS()
{
	return X.S;
}
//----------------------------------------------------
uint8_t MemoryScriptObject::getRegisterP()
{
	return X.P;
}
//----------------------------------------------------
void MemoryScriptObject::setRegisterPC(uint16_t v)
{
	X.PC = v;
}
//----------------------------------------------------
void MemoryScriptObject::setRegisterA(uint8_t v)
{
	X.A = v;
}
//----------------------------------------------------
void MemoryScriptObject::setRegisterX(uint8_t v)
{
	X.X = v;
}
//----------------------------------------------------
void MemoryScriptObject::setRegisterY(uint8_t v)
{
	X.Y = v;
}
//----------------------------------------------------
void MemoryScriptObject::setRegisterS(uint8_t v)
{
	X.S = v;
}
//----------------------------------------------------
void MemoryScriptObject::setRegisterP(uint8_t v)
{
	X.P = v;
}
//----------------------------------------------------
void MemoryScriptObject::registerCallback(int type, const QJSValue& func, int address, int size)
{
	int n=0;
	int *numFuncsRegistered = nullptr;
	QJSValue** funcArray = nullptr;

	switch (type)
	{
		default:
		case X6502_MemHook::Read:
			funcArray = readFunc;
			numFuncsRegistered = &numReadFuncsRegistered;
			X6502_MemHook::Add(X6502_MemHook::Read, addressReadCallback, this);
		break;
		case X6502_MemHook::Write:
			funcArray = writeFunc;
			numFuncsRegistered = &numWriteFuncsRegistered;
			X6502_MemHook::Add(X6502_MemHook::Write, addressWriteCallback, this);
		break;
		case X6502_MemHook::Exec:
			funcArray = execFunc;
			numFuncsRegistered = &numExecFuncsRegistered;
			X6502_MemHook::Add(X6502_MemHook::Exec, addressExecCallback, this);
		break;
	}
	n = *numFuncsRegistered;

	if (func.isCallable())
	{
		for (int i=0; i<size; i++)
		{
			int addr = address + i;

			if ( (addr >= 0) && (addr < AddressRange) )
			{
				if (funcArray[addr] != nullptr)
				{
					n--;
					delete funcArray[addr];
				}
				funcArray[addr] = new QJSValue(func);
				n++;
			}
		}
	}
	else
	{
		for (int i=0; i<size; i++)
		{
			int addr = address + i;

			if ( (addr >= 0) && (addr < AddressRange) )
			{
				if (funcArray[addr] != nullptr)
				{
					n--;
					delete funcArray[addr];
				}
				funcArray[addr] = nullptr;
			}
		}
	}
	*numFuncsRegistered = n;
}
//----------------------------------------------------
void MemoryScriptObject::registerRead(const QJSValue& func, int address, int size)
{
	registerCallback(X6502_MemHook::Read, func, address, size);
}
//----------------------------------------------------
void MemoryScriptObject::registerWrite(const QJSValue& func, int address, int size)
{
	registerCallback(X6502_MemHook::Write, func, address, size);
}
//----------------------------------------------------
void MemoryScriptObject::registerExec(const QJSValue& func, int address, int size)
{
	registerCallback(X6502_MemHook::Exec, func, address, size);
}
//----------------------------------------------------
void MemoryScriptObject::unregisterCallback(int type, const QJSValue& func, int address, int size)
{
	int n=0;
	int *numFuncsRegistered = nullptr;
	QJSValue** funcArray = nullptr;

	switch (type)
	{
		default:
		case X6502_MemHook::Read:
			funcArray = readFunc;
			numFuncsRegistered = &numReadFuncsRegistered;
		break;
		case X6502_MemHook::Write:
			funcArray = writeFunc;
			numFuncsRegistered = &numWriteFuncsRegistered;
		break;
		case X6502_MemHook::Exec:
			funcArray = execFunc;
			numFuncsRegistered = &numExecFuncsRegistered;
		break;
	}
	n = *numFuncsRegistered;

	if (func.isCallable())
	{
		for (int i=0; i<size; i++)
		{
			int addr = address + i;

			if ( (addr >= 0) && (addr < AddressRange) )
			{
				if (funcArray[addr] != nullptr)
				{
					n--;
					delete funcArray[addr];
				}
				funcArray[addr] = nullptr;
			}
		}
	}
	else
	{
		for (int i=0; i<size; i++)
		{
			int addr = address + i;

			if ( (addr >= 0) && (addr < AddressRange) )
			{
				if (funcArray[addr] != nullptr)
				{
					n--;
					delete funcArray[addr];
				}
				funcArray[addr] = nullptr;
			}
		}
	}
	*numFuncsRegistered = n;

	if (0 <= numReadFuncsRegistered)
	{
		X6502_MemHook::Remove(X6502_MemHook::Read, addressReadCallback, this);
	}
	if (0 <= numWriteFuncsRegistered)
	{
		X6502_MemHook::Remove(X6502_MemHook::Write, addressWriteCallback, this);
	}
	if (0 <= numExecFuncsRegistered)
	{
		X6502_MemHook::Remove(X6502_MemHook::Exec, addressExecCallback, this);
	}
}
//----------------------------------------------------
void MemoryScriptObject::unregisterRead(const QJSValue& func, int address, int size)
{
	unregisterCallback(X6502_MemHook::Read, func, address, size);
}
//----------------------------------------------------
void MemoryScriptObject::unregisterWrite(const QJSValue& func, int address, int size)
{
	unregisterCallback(X6502_MemHook::Write, func, address, size);
}
//----------------------------------------------------
void MemoryScriptObject::unregisterExec(const QJSValue& func, int address, int size)
{
	unregisterCallback(X6502_MemHook::Exec, func, address, size);
}
//----------------------------------------------------
void MemoryScriptObject::unregisterAll()
{
	X6502_MemHook::Remove(X6502_MemHook::Read, addressReadCallback, this);
	X6502_MemHook::Remove(X6502_MemHook::Write, addressWriteCallback, this);
	X6502_MemHook::Remove(X6502_MemHook::Exec, addressExecCallback, this);

	for (int i=0; i<AddressRange; i++)
	{
		if (execFunc[i] != nullptr)
		{
			numExecFuncsRegistered--;
			delete execFunc[i];
		}
		if (readFunc[i] != nullptr)
		{
			numReadFuncsRegistered--;
			delete readFunc[i];
		}
		if (writeFunc[i] != nullptr)
		{
			numWriteFuncsRegistered--;
			delete writeFunc[i];
		}
		execFunc[i] = nullptr;
		readFunc[i] = nullptr;
		writeFunc[i] = nullptr;
	}
	numReadFuncsRegistered = 0;
	numWriteFuncsRegistered = 0;
	numExecFuncsRegistered = 0;
}

//----------------------------------------------------
//----  Module Loader Object
//----------------------------------------------------
//----------------------------------------------------
ModuleLoaderObject::ModuleLoaderObject(QObject* parent)
	: QObject(parent)
{
	script = qobject_cast<QtScriptInstance*>(parent);
	engine = script->getEngine();
	scopeCounter = 0;
}
//----------------------------------------------------
ModuleLoaderObject::~ModuleLoaderObject()
{
}
//----------------------------------------------------
void ModuleLoaderObject::GlobalImport(const QString& ns, const QString& file)
{
	const QString& srcFile = script->getSrcFile();
	QFileInfo srcFileInfo(srcFile);
	QString srcPath = srcFileInfo.canonicalPath();
	QString cwd = QDir::currentPath();

	scopeCounter++;

	QDir::setCurrent(srcPath);
	QFileInfo moduleFileInfo(file);
	QString modulePath = moduleFileInfo.canonicalFilePath();

	//printf("%i Namespace: %s     File: %s\n", scopeCounter, ns.toLocal8Bit().constData(), modulePath.toLocal8Bit().constData() );

	QJSValue newModule = engine->importModule( modulePath );
	QDir::setCurrent(cwd);

	scopeCounter--;

	if (newModule.isError())
	{
		QString errMsg = QString("Failed to load module: ") + file + "\n" +
		newModule.toString() + "\n" +
		newModule.property("fileName").toString() + ":" +
		newModule.property("lineNumber").toString() + " : " +
		newModule.toString() + "\nStack:\n" +
		newModule.property("stack").toString() + "\n";

		script->throwError(QJSValue::GenericError, errMsg);
		return;
	}

	engine->globalObject().setProperty(ns, newModule);

	QString msg = QString("Global import * as '") + ns + QString("' from \"") + file + "\";\n";
	script->print(msg);
}
//----------------------------------------------------

} // JS
//----------------------------------------------------
//----  FCEU JSEngine
//----------------------------------------------------
namespace FCEU
{
	JSEngine::JSEngine(QObject* parent)
		: QJSEngine(parent)
	{
	}

	JSEngine::~JSEngine()
	{
	}

	void JSEngine::logMessage(int lvl, const QString& msg)
	{
		if (dialog != nullptr)
		{
			if (lvl <= _logLevel)
			{
				const char *prefix = "Warning: ";
				switch (lvl)
				{
					case FCEU::JSEngine::DEBUG:
						prefix = "Debug: ";
					break;
					case FCEU::JSEngine::INFO:
						prefix = "Info: ";
					break;
					case FCEU::JSEngine::WARNING:
						prefix = "Warning: ";
					break;
					case FCEU::JSEngine::CRITICAL:
						prefix = "Critical: ";
					break;
					case FCEU::JSEngine::FATAL:
						prefix = "Fatal: ";
					break;
				}
				QString fullMsg = prefix + msg.trimmed() + "\n";
				dialog->logOutput(fullMsg);
			}
		}
	}

	void JSEngine::acquireThreadContext()
	{
		prevContext = currentEngine;
		currentEngine = this;
	}

	void JSEngine::releaseThreadContext()
	{
		currentEngine = prevContext;
		prevContext = nullptr;
	}

	JSEngine* JSEngine::getCurrent()
	{
		return currentEngine;
	}
}
//----------------------------------------------------
//----  Qt Script Instance
//----------------------------------------------------
QtScriptInstance::QtScriptInstance(QObject* parent)
	: QObject(parent)
{
	QScriptDialog_t* win = qobject_cast<QScriptDialog_t*>(parent);

	if (win != nullptr)
	{
		dialog = win;
	}

	FCEU_WRAPPER_LOCK();

	initEngine();

	QtScriptManager::getInstance()->addScriptInstance(this);

	FCEU_WRAPPER_UNLOCK();
}
//----------------------------------------------------
QtScriptInstance::~QtScriptInstance()
{
	FCEU_WRAPPER_LOCK();

	QtScriptManager::getInstance()->removeScriptInstance(this);

	shutdownEngine();

	FCEU_WRAPPER_UNLOCK();

	//printf("QtScriptInstance Destroyed\n");
}
//----------------------------------------------------
void QtScriptInstance::shutdownEngine()
{
	running = false;

	if (onFrameBeginCallback != nullptr)
	{
		delete onFrameBeginCallback;
		onFrameBeginCallback = nullptr;
	}
	if (onFrameFinishCallback != nullptr)
	{
		delete onFrameFinishCallback;
		onFrameFinishCallback = nullptr;
	}
	if (onScriptStopCallback != nullptr)
	{
		delete onScriptStopCallback;
		onScriptStopCallback = nullptr;
	}
	if (onGuiUpdateCallback != nullptr)
	{
		delete onGuiUpdateCallback;
		onGuiUpdateCallback = nullptr;
	}

	if (engine != nullptr)
	{
		engine->collectGarbage();
		//engine->deleteLater();
		delete engine;
		engine = nullptr;
	}

	if (emu != nullptr)
	{
		delete emu;
		emu = nullptr;
	}
	if (rom != nullptr)
	{
		delete rom;
		rom = nullptr;
	}
	if (ppu != nullptr)
	{
		delete ppu;
		ppu = nullptr;
	}
	if (mem != nullptr)
	{
		mem->reset();
		delete mem;
		mem = nullptr;
	}
	if (input != nullptr)
	{
		delete input;
		input = nullptr;
	}
	if (movie != nullptr)
	{
		delete movie;
		movie = nullptr;
	}
	if (moduleLoader != nullptr)
	{
		delete moduleLoader;
		moduleLoader = nullptr;
	}

	if (ui_rootWidget != nullptr)
	{
		ui_rootWidget->hide();
		ui_rootWidget->deleteLater();
		ui_rootWidget = nullptr;
	}
}
//----------------------------------------------------
void QtScriptInstance::resetEngine()
{
	shutdownEngine();
	initEngine();
}
//----------------------------------------------------
int QtScriptInstance::initEngine()
{
	engine = new FCEU::JSEngine(this);

	engine->setScript(this);
	engine->setDialog(dialog);

	emu = new JS::EmuScriptObject(this);
	rom = new JS::RomScriptObject(this);
	ppu = new JS::PpuScriptObject(this);
	mem = new JS::MemoryScriptObject(this);
	input = new JS::InputScriptObject(this);
	movie = new JS::MovieScriptObject(this);
	moduleLoader = new JS::ModuleLoaderObject(this);

	emu->setDialog(dialog);
	rom->setDialog(dialog);
	mem->setDialog(dialog);

	engine->installExtensions(QJSEngine::ConsoleExtension);

	emu->setEngine(engine);
	rom->setEngine(engine);
	mem->setEngine(engine);

	// emu
	QJSValue emuObject = engine->newQObject(emu);

	engine->globalObject().setProperty("emu", emuObject);

	// rom
	QJSValue romObject = engine->newQObject(rom);

	engine->globalObject().setProperty("rom", romObject);

	// ppu
	QJSValue ppuObject = engine->newQObject(ppu);

	engine->globalObject().setProperty("ppu", ppuObject);

	// memory
	QJSValue memObject = engine->newQObject(mem);

	engine->globalObject().setProperty("memory", memObject);

	// input
	QJSValue inputObject = engine->newQObject(input);

	engine->globalObject().setProperty("input", inputObject);

	// movie
	QJSValue movieObject = engine->newQObject(movie);

	engine->globalObject().setProperty("movie", movieObject);

	// gui
	QJSValue guiObject = engine->newQObject(this);

	engine->globalObject().setProperty("gui", guiObject);

	// module
	QJSValue moduleLoaderObject = engine->newQObject(moduleLoader);

	engine->globalObject().setProperty("Module", moduleLoaderObject);

	// Class Type Definitions for Script Use
	QJSValue jsColorMetaObject = engine->newQMetaObject(&JS::ColorScriptObject::staticMetaObject);
	engine->globalObject().setProperty("Color", jsColorMetaObject);

	QJSValue jsFileMetaObject = engine->newQMetaObject(&JS::FileScriptObject::staticMetaObject);
	engine->globalObject().setProperty("File", jsFileMetaObject);

	QJSValue jsJoypadMetaObject = engine->newQMetaObject(&JS::JoypadScriptObject::staticMetaObject);
	engine->globalObject().setProperty("Joypad", jsJoypadMetaObject);

	QJSValue jsEmuStateMetaObject = engine->newQMetaObject(&JS::EmuStateScriptObject::staticMetaObject);
	engine->globalObject().setProperty("EmuState", jsEmuStateMetaObject);

	return 0;
}
//----------------------------------------------------
int QtScriptInstance::loadScriptFile( QString filepath )
{
	QFile scriptFile(filepath);

	running = false;

	if (!scriptFile.open(QIODevice::ReadOnly))
	{
		return -1;
	}
	QTextStream stream(&scriptFile);
	QString fileText = stream.readAll();
	scriptFile.close();

	srcFile = filepath;

	FCEU_WRAPPER_LOCK();
	engine->acquireThreadContext();
	QJSValue evalResult = engine->evaluate(fileText, filepath);
	engine->releaseThreadContext();
	FCEU_WRAPPER_UNLOCK();

	if (evalResult.isError())
	{
		QString msg = "Uncaught exception at: " +
			evalResult.property("fileName").toString() + ":" +
			evalResult.property("lineNumber").toString() + " : " +
			evalResult.toString() + "\nStack:\n" +
			evalResult.property("stack").toString() + "\n";
		print(msg);
		emit errorNotify();
		return -1;
	}
	else
	{
		running = true;
		//printf("Script Evaluation Success!\n");
	}

	return 0;
}
//----------------------------------------------------
void QtScriptInstance::loadObjectChildren(QJSValue& jsObject, QObject* obj)
{
	const QObjectList& childList = obj->children();

	for (auto& child : childList)
	{
		QString name = child->objectName();

		if (!name.isEmpty())
		{
			//printf("Object: %s.%s\n", obj->objectName().toLocal8Bit().constData(), child->objectName().toLocal8Bit().constData());

			QJSValue newJsObj = engine->newQObject(child);

			jsObject.setProperty(child->objectName(), newJsObj);

			loadObjectChildren( newJsObj, child);
		}
	}
}
//----------------------------------------------------
void QtScriptInstance::loadUI(const QString& uiFilePath)
{
#ifdef __QT_UI_TOOLS__
	QFile uiFile(uiFilePath);
	QUiLoader  uiLoader;

	ui_rootWidget = uiLoader.load(&uiFile, dialog);

	if (ui_rootWidget == nullptr)
	{
		return;
	}
	
	QJSValue uiObject = engine->newQObject(ui_rootWidget);

	engine->globalObject().setProperty("ui", uiObject);

	loadObjectChildren( uiObject, ui_rootWidget);

	ui_rootWidget->show();
#else
	throwError(QJSValue::GenericError, "Application was not linked against Qt UI Tools");
#endif
}
//----------------------------------------------------
void QtScriptInstance::frameAdvance()
{
	frameAdvanceCount++;
}
//----------------------------------------------------
void QtScriptInstance::registerBeforeEmuFrame(const QJSValue& func)
{
	if (onFrameBeginCallback != nullptr)
	{
		delete onFrameBeginCallback;
	}
	onFrameBeginCallback = new QJSValue(func);
}
//----------------------------------------------------
void QtScriptInstance::registerAfterEmuFrame(const QJSValue& func)
{
	if (onFrameFinishCallback != nullptr)
	{
		delete onFrameFinishCallback;
	}
	onFrameFinishCallback = new QJSValue(func);
}
//----------------------------------------------------
void QtScriptInstance::registerStop(const QJSValue& func)
{
	if (onScriptStopCallback != nullptr)
	{
		delete onScriptStopCallback;
	}
	onScriptStopCallback = new QJSValue(func);
}
//----------------------------------------------------
void QtScriptInstance::registerGuiUpdate(const QJSValue& func)
{
	if (onGuiUpdateCallback != nullptr)
	{
		delete onGuiUpdateCallback;
	}
	onGuiUpdateCallback = new QJSValue(func);
}
//----------------------------------------------------
void QtScriptInstance::print(const QString& msg)
{
	if (dialog)
	{
		dialog->logOutput(msg);
	}
}
//----------------------------------------------------
bool QtScriptInstance::onEmulationThread()
{
	bool isEmuThread = (consoleWindow != nullptr) && 
		(QThread::currentThread() == consoleWindow->emulatorThread);
	return isEmuThread;
}
//----------------------------------------------------
bool QtScriptInstance::onGuiThread()
{
	bool isGuiThread = (QThread::currentThread() == QApplication::instance()->thread());
	return isGuiThread;
}
//----------------------------------------------------
int QtScriptInstance::throwError(QJSValue::ErrorType errorType, const QString &message)
{
	running = false;
	mem->reset();
	engine->throwError(errorType, message);
	return 0;
}
//----------------------------------------------------
void QtScriptInstance::printSymbols(QJSValue& val, int iter)
{
	int i=0;
	if (iter > 10)
	{
		return;
	}
	QJSValueIterator it(val);
	while (it.hasNext()) 
	{
		it.next();
		QJSValue child = it.value();
		qDebug() << iter << ":" << i << "  " << it.name() << ": " << child.toString();

		bool isPrototype = it.name() == "prototype";

		if (!isPrototype)
		{
			printSymbols(child, iter + 1);
		}
		i++;
	}
}
//----------------------------------------------------
int  QtScriptInstance::runFunc(QJSValue &func, const QJSValueList& args)
{
	int retval = 0;
	auto state = getExecutionState();

	state->start();

	engine->acquireThreadContext();

	QJSValue callResult = func.call(args);

	engine->releaseThreadContext();

	state->stop();

	if (callResult.isError())
	{
		retval = -1;
		running = false;
		QString msg = "Uncaught exception at: " +
			callResult.property("fileName").toString() + ":" +
			callResult.property("lineNumber").toString() + " : " +
			callResult.toString() + "\nStack:\n" +
			callResult.property("stack").toString() + "\n";
		print(msg);

		emit errorNotify();
	}
	return retval;
}
//----------------------------------------------------
int  QtScriptInstance::call(const QString& funcName, const QJSValueList& args)
{
	if (engine == nullptr)
	{
		return -1;
	}
	if (!engine->globalObject().hasProperty(funcName))
	{
		print(QString("No function exists: ") + funcName);
		emit errorNotify();
		return -1;
	}
	QJSValue func = engine->globalObject().property(funcName);

	FCEU_WRAPPER_LOCK();
	int retval = runFunc(func, args);
	FCEU_WRAPPER_UNLOCK();

	return retval;
}
//----------------------------------------------------
void QtScriptInstance::stopRunning()
{
	FCEU_WRAPPER_LOCK();
	if (running)
	{
		if (onScriptStopCallback != nullptr && onScriptStopCallback->isCallable())
		{
			runFunc( *onScriptStopCallback );
		}
		running = false;

		mem->reset();
	}
	FCEU_WRAPPER_UNLOCK();
}
//----------------------------------------------------
void QtScriptInstance::onFrameBegin()
{
	if (running)
	{
		if (onFrameBeginCallback != nullptr && onFrameBeginCallback->isCallable())
		{
			runFunc( *onFrameBeginCallback );
		}
		if (frameAdvanceCount > 0)
		{
			if (frameAdvanceState == 0)
			{
				FCEUI_FrameAdvance();
				frameAdvanceState = 1;
				frameAdvanceCount--;
			}
		}
	}
}
//----------------------------------------------------
void QtScriptInstance::onFrameFinish()
{
	if (running)
	{
		if (onFrameFinishCallback != nullptr && onFrameFinishCallback->isCallable())
		{
			runFunc( *onFrameFinishCallback );
		}
		if (frameAdvanceState == 1)
		{
			FCEUI_FrameAdvanceEnd();
			frameAdvanceState = 0;
		}
	}
}
//----------------------------------------------------
void QtScriptInstance::flushLog()
{
	if (dialog != nullptr)
	{
		dialog->flushLog();
	}
}
//----------------------------------------------------
void QtScriptInstance::onGuiUpdate()
{
	if (running && onGuiUpdateCallback != nullptr && onGuiUpdateCallback->isCallable())
	{
		runFunc( *onGuiUpdateCallback );
	}
}
//----------------------------------------------------
ScriptExecutionState* QtScriptInstance::getExecutionState()
{
	ScriptExecutionState* state;

	if (onEmulationThread())
	{
		state = &emuFuncState;
	}
	else
	{
		state = &guiFuncState;
	}
	return state;
}
//----------------------------------------------------
void QtScriptInstance::checkForHang()
{
	static constexpr unsigned int funcTimeoutMs = 1000;

	if ( guiFuncState.isRunning() )
	{
		unsigned int timeRunningMs = guiFuncState.timeCheck();

		if (timeRunningMs > funcTimeoutMs)
		{
			printf("Interrupted GUI Thread Script Function\n");
			engine->setInterrupted(true);
		}
	}

	if ( emuFuncState.isRunning() )
	{
		unsigned int timeRunningMs = emuFuncState.timeCheck();

		if (timeRunningMs > funcTimeoutMs)
		{
			printf("Interrupted Emulation Thread Script Function\n");
			engine->setInterrupted(true);
		}
	}

}
//----------------------------------------------------
QString QtScriptInstance::openFileBrowser(const QString& initialPath)
{
	QString selectedFile;
	QFileDialog  dialog(this->dialog, tr("Open File") );
	QList<QUrl> urls;
	bool useNativeFileDialogVal = false;

	g_config->getOption("SDL.UseNativeFileDialog", &useNativeFileDialogVal);

	const QStringList filters({
           "Any files (*)"
         });

	urls << QUrl::fromLocalFile( QDir::rootPath() );
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first());
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::DesktopLocation).first());
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::DownloadLocation).first());
	urls << QUrl::fromLocalFile( QDir( FCEUI_GetBaseDirectory() ).absolutePath() );

	dialog.setFileMode(QFileDialog::ExistingFile);

	dialog.setNameFilters( filters );

	dialog.setViewMode(QFileDialog::List);
	dialog.setFilter( QDir::AllEntries | QDir::AllDirs | QDir::Hidden );
	dialog.setLabelText( QFileDialog::Accept, tr("Open") );

	if (!initialPath.isEmpty() )
	{
		dialog.setDirectory( initialPath );
	}

	dialog.setOption(QFileDialog::DontUseNativeDialog, !useNativeFileDialogVal);
	dialog.setSidebarUrls(urls);

	int ret = dialog.exec();

	if ( ret )
	{
		QStringList fileList;
		fileList = dialog.selectedFiles();

		if ( fileList.size() > 0 )
		{
			selectedFile = fileList[0];
		}
	}
	return selectedFile;
}
//----------------------------------------------------
//----  Qt Script Manager
//----------------------------------------------------
QtScriptManager* QtScriptManager::_instance = nullptr;

QtScriptManager::QtScriptManager(QObject* parent)
	: QObject(parent)
{
	_instance = this;
	monitorThread = new ScriptMonitorThread_t(this);
	monitorThread->start();

	periodicUpdateTimer = new QTimer(this);
	connect( periodicUpdateTimer, &QTimer::timeout, this, &QtScriptManager::guiUpdate );
	periodicUpdateTimer->start(50); // ~20hz
}
//----------------------------------------------------
QtScriptManager::~QtScriptManager()
{
	monitorThread->requestInterruption();
	monitorThread->wait();

	_instance = nullptr;
	//printf("QtScriptManager destroyed\n");
}
//----------------------------------------------------
QtScriptManager* QtScriptManager::create(QObject* parent)
{
	QtScriptManager* mgr = new QtScriptManager(parent);

	//printf("QtScriptManager created\n");
	
	return mgr;
}
//----------------------------------------------------
void QtScriptManager::destroy(void)
{
	if (_instance != nullptr)
	{
		delete _instance;
	}
}
//----------------------------------------------------
void QtScriptManager::logMessageQt(QtMsgType type, const QString &msg)
{
	auto* engine = FCEU::JSEngine::getCurrent();

	if (engine != nullptr)
	{
		int logLevel = FCEU::JSEngine::WARNING;

		switch (type)
		{
			case QtDebugMsg:
				logLevel = FCEU::JSEngine::DEBUG;
			break;
			case QtInfoMsg:
				logLevel = FCEU::JSEngine::INFO;
			break;
			case QtWarningMsg:
				logLevel = FCEU::JSEngine::WARNING;
			break;
			case QtCriticalMsg:
				logLevel = FCEU::JSEngine::CRITICAL;
			break;
			case QtFatalMsg:
				logLevel = FCEU::JSEngine::FATAL;
			break;
		}
		engine->logMessage( logLevel, msg );
	}
}
//----------------------------------------------------
void QtScriptManager::addScriptInstance(QtScriptInstance* script)
{
	FCEU::autoScopedLock autoLock(scriptListMutex);
	scriptList.push_back(script);
}
//----------------------------------------------------
void QtScriptManager::removeScriptInstance(QtScriptInstance* script)
{
	FCEU::autoScopedLock autoLock(scriptListMutex);
	auto it = scriptList.begin();

	while (it != scriptList.end())
	{
		if (*it == script)
		{
			it = scriptList.erase(it);
		}
		else
		{
			it++;
		}
	}

	// If no scripts are loaded, reset globals
	if (scriptList.size() == 0)
	{
		JS::MovieScriptObject::skipRerecords = false;
		JS::JoypadScriptObject::ovrdResetAll();
	}
}
//----------------------------------------------------
void QtScriptManager::frameBeginUpdate()
{
	FCEU_WRAPPER_LOCK();
	for (auto script : scriptList)
	{
		script->onFrameBegin();
	}
	FCEU_WRAPPER_UNLOCK();
}
//----------------------------------------------------
void QtScriptManager::frameFinishedUpdate()
{
	FCEU_WRAPPER_LOCK();
	for (auto script : scriptList)
	{
		script->onFrameFinish();
	}
	FCEU_WRAPPER_UNLOCK();
}
//----------------------------------------------------
void QtScriptManager::guiUpdate()
{
	FCEU_WRAPPER_LOCK();
	for (auto script : scriptList)
	{
		script->onGuiUpdate();
	}
	FCEU_WRAPPER_UNLOCK();

	//for (auto script : scriptList)
	//{
	//	script->flushLog();
	//}
}
//----------------------------------------------------
//---- Qt Script Monitor Thread
//----------------------------------------------------
ScriptMonitorThread_t::ScriptMonitorThread_t(QObject *parent)
	: QThread(parent)
{
}
//----------------------------------------------------
void ScriptMonitorThread_t::run()
{
	//printf("Script Monitor Thread is Running...\n");
	QtScriptManager* manager = QtScriptManager::getInstance();

	while (!isInterruptionRequested())
	{
		manager->scriptListMutex.lock();
		for (auto script : manager->scriptList)
		{
			script->checkForHang();
		}
		manager->scriptListMutex.unlock();
		msleep(ScriptExecutionState::checkPeriod);
	}
	//printf("Script Monitor Thread is Stopping...\n");
}
//----------------------------------------------------
//---- Qt Script Dialog Window
//----------------------------------------------------
QScriptDialog_t::QScriptDialog_t(QWidget *parent)
	: QDialog(parent, Qt::Window)
{
	QVBoxLayout *mainLayout;
	QHBoxLayout *hbox;
	QPushButton *closeButton;
	QLabel *lbl;
	std::string filename;
	QSettings settings;

	resize(512, 512);

	setWindowTitle(tr("JavaScript Control"));

	menuBar = buildMenuBar();
	mainLayout = new QVBoxLayout();
	mainLayout->setMenuBar( menuBar );

	lbl = new QLabel(tr("Script File:"));

	scriptPath = new QLineEdit();
	scriptArgs = new QLineEdit();
	browseButton = new QPushButton(tr("Browse"));

	hbox = new QHBoxLayout();
	hbox->addWidget(lbl);
	hbox->addWidget(scriptPath);
	hbox->addWidget(browseButton);
	mainLayout->addLayout(hbox);

	hbox = new QHBoxLayout();
	lbl = new QLabel(tr("Arguments:"));
	hbox->addWidget(lbl);
	hbox->addWidget(scriptArgs);
	mainLayout->addLayout(hbox);

	g_config->getOption("SDL.LastLoadJs", &filename);

	scriptPath->setText( tr(filename.c_str()) );
	scriptPath->setClearButtonEnabled(true);
	scriptArgs->setClearButtonEnabled(true);

	jsOutput = new QTextEdit();
	jsOutput->setReadOnly(true);

	stopButton = new QPushButton(tr("Stop"));

	scriptInstance = new QtScriptInstance(this);

	if (scriptInstance->isRunning())
	{
		startButton = new QPushButton(tr("Restart"));
	}
	else
	{
		startButton = new QPushButton(tr("Start"));
	}

	stopButton->setEnabled(scriptInstance->isRunning());

	connect(browseButton, SIGNAL(clicked()), this, SLOT(openScriptFile(void)));
	connect(stopButton, SIGNAL(clicked()), this, SLOT(stopScript(void)));
	connect(startButton, SIGNAL(clicked()), this, SLOT(startScript(void)));

	hbox = new QHBoxLayout();
	hbox->addWidget(stopButton);
	hbox->addWidget(startButton);
	mainLayout->addLayout(hbox);

	tabWidget = new QTabWidget();
	mainLayout->addWidget(tabWidget);

	tabWidget->addTab(jsOutput, tr("Output Console"));

	propTree = new JsPropertyTree();

	propTree->setColumnCount(3);
	propTree->setSelectionMode( QAbstractItemView::SingleSelection );
	propTree->setAlternatingRowColors(true);

	auto* item = new QTreeWidgetItem();
	item->setText(0, QString::fromStdString("Name"));
	item->setText(1, QString::fromStdString("Type"));
	item->setText(2, QString::fromStdString("Value"));
	item->setTextAlignment(0, Qt::AlignLeft);
	item->setTextAlignment(1, Qt::AlignLeft);
	item->setTextAlignment(2, Qt::AlignLeft);

	propTree->setHeaderItem(item);
	propTree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

	tabWidget->addTab(propTree, tr("Global Properties"));

	logFilepathLbl = new QLabel( tr("Logging to:") );
	logFilepath = new QLabel();
	logFilepath->setTextInteractionFlags(Qt::TextBrowserInteraction);
	connect( logFilepath, SIGNAL(linkActivated(const QString&)), this, SLOT(onLogLinkClicked(const QString&)) );
	closeButton = new QPushButton( tr("Close") );
	closeButton->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
	connect(closeButton, SIGNAL(clicked(void)), this, SLOT(closeWindow(void)));

	hbox = new QHBoxLayout();
	hbox->addWidget( logFilepathLbl, 1 );
	hbox->addWidget( logFilepath, 10 );
	hbox->addWidget( closeButton, 1 );
	mainLayout->addLayout( hbox );

	setLayout(mainLayout);

	emuThreadText.reserve(4096);

	//winList.push_back(this);

	periodicTimer = new QTimer(this);

	connect(periodicTimer, &QTimer::timeout, this, &QScriptDialog_t::updatePeriodic);

	periodicTimer->start(200); // 5hz

	restoreGeometry(settings.value("QScriptWindow/geometry").toByteArray());

	connect(scriptInstance, SIGNAL(errorNotify()), this, SLOT(onScriptError(void)));
}
//----------------------------------------------------
QScriptDialog_t::~QScriptDialog_t(void)
{
	QSettings settings;

	//printf("Destroy JS Control Window\n");

	periodicTimer->stop();

	clearPropertyTree();

	scriptInstance->stopRunning();
	delete scriptInstance;

	settings.setValue("QScriptWindow/geometry", saveGeometry());
}
//----------------------------------------------------
void QScriptDialog_t::closeEvent(QCloseEvent *event)
{
	scriptInstance->stopRunning();

	//printf("JS Control Close Window Event\n");
	done(0);
	deleteLater();
	event->accept();
}
//----------------------------------------------------
void QScriptDialog_t::closeWindow(void)
{
	scriptInstance->stopRunning();

	//printf("JS Control Close Window\n");
	done(0);
	deleteLater();
}
//----------------------------------------------------
QMenuBar *QScriptDialog_t::buildMenuBar(void)
{
	QMenu       *fileMenu;
	//QActionGroup *actGroup;
	QAction     *act;
	int useNativeMenuBar=0;

	QMenuBar *menuBar = new QMenuBar(this);

	// This is needed for menu bar to show up on MacOS
	g_config->getOption( "SDL.UseNativeMenuBar", &useNativeMenuBar );

	menuBar->setNativeMenuBar( useNativeMenuBar ? true : false );

	//-----------------------------------------------------------------------
	// Menu Start
	//-----------------------------------------------------------------------
	// File
	fileMenu = menuBar->addMenu(tr("&File"));

	// File -> Open Script
	act = new QAction(tr("&Open Script"), this);
	act->setShortcut(QKeySequence::Open);
	act->setStatusTip(tr("Open Script"));
	connect(act, SIGNAL(triggered()), this, SLOT(openScriptFile(void)) );

	fileMenu->addAction(act);

	// File -> Save Log
	act = new QAction(tr("&Save Log"), this);
	//act->setShortcut(QKeySequence::Close);
	act->setStatusTip(tr("Save Log"));
	connect(act, &QAction::triggered, [ this ] { saveLog(false); } );

	fileMenu->addAction(act);

	// File -> Save Log As
	act = new QAction(tr("Save Log &As"), this);
	//act->setShortcut(QKeySequence::Close);
	act->setStatusTip(tr("Save Log As"));
	connect(act, &QAction::triggered, [ this ] { saveLog(true); } );

	fileMenu->addAction(act);

	// File -> Flush Log
	act = new QAction(tr("Flush &Log"), this);
	//act->setShortcut(QKeySequence::Close);
	act->setStatusTip(tr("Flush Log to Disk"));
	connect(act, SIGNAL(triggered()), this, SLOT(flushLog(void)) );

	fileMenu->addAction(act);

	// File -> Close
	act = new QAction(tr("&Close"), this);
	act->setShortcut(QKeySequence::Close);
	act->setStatusTip(tr("Close Window"));
	connect(act, SIGNAL(triggered()), this, SLOT(closeWindow(void)) );

	fileMenu->addAction(act);

	return menuBar;
}
//----------------------------------------------------
void QScriptDialog_t::saveLog(bool openFileBrowser)
{
	if (logFile != nullptr)
	{
	       	if (logSavePath.isEmpty() || openFileBrowser)
		{
			QString initialPath;
			QFileDialog  dialog(this, tr("Save Log File") );
			QList<QUrl> urls;
			bool useNativeFileDialogVal = false;

			g_config->getOption("SDL.UseNativeFileDialog", &useNativeFileDialogVal);

			const QStringList filters({
        		   "Any files (*)"
        		 });

			urls << QUrl::fromLocalFile( QDir::rootPath() );
			urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first());
			urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::DesktopLocation).first());
			urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::DownloadLocation).first());
			urls << QUrl::fromLocalFile( QDir( FCEUI_GetBaseDirectory() ).absolutePath() );

			dialog.setFileMode(QFileDialog::AnyFile);

			dialog.setNameFilters( filters );

			dialog.setViewMode(QFileDialog::List);
			dialog.setFilter( QDir::AllEntries | QDir::AllDirs | QDir::Hidden );
			dialog.setLabelText( QFileDialog::Accept, tr("Save") );

			if (!initialPath.isEmpty() )
			{
				dialog.setDirectory( initialPath );
			}

			dialog.setOption(QFileDialog::DontUseNativeDialog, !useNativeFileDialogVal);
			dialog.setSidebarUrls(urls);

			int ret = dialog.exec();

			if ( ret != QDialog::Rejected )
			{
				QStringList fileList;
				fileList = dialog.selectedFiles();

				if ( fileList.size() > 0 )
				{
					logSavePath = fileList[0];
				}
			}
			else
			{
				return;
			}
		}

		if (QFile::exists(logSavePath))
		{
			QFile::remove(logSavePath);
		}
		//printf("Saving Log File: %s\n", logSavePath.toLocal8Bit().constData());
		FCEU_WRAPPER_LOCK();
		{
			char buffer[4096];
			flushLog();
			QFile saveFile( logSavePath );
			if (saveFile.open(QIODevice::ReadWrite))
			{
				logFile->seek(0);
				qint64 bytesRead = logFile->read(buffer, sizeof(buffer));
				while (bytesRead > 0)
				{
					saveFile.write(buffer, bytesRead);

					bytesRead = logFile->read(buffer, sizeof(buffer));
				}
			}
		}
		FCEU_WRAPPER_UNLOCK();
	}
}
//----------------------------------------------------
void QScriptDialog_t::flushLog()
{
	if (logFile != nullptr)
	{
		logFile->flush();
	}
}
//----------------------------------------------------
void QScriptDialog_t::onScriptError()
{
	//printf("QScriptDialog_t::onScriptError\n");

	flushLog();
}
//----------------------------------------------------
void QScriptDialog_t::onLogLinkClicked(const QString& link)
{
	QUrl url = QUrl::fromUserInput(link);

	if( url.isValid() )
	{
		flushLog();

		QDesktopServices::openUrl(url);
	}
}
//----------------------------------------------------
void QScriptDialog_t::clearPropertyTree()
{
	propTree->childMap.clear();
	propTree->clear();
}
//----------------------------------------------------
void QScriptDialog_t::loadPropertyTree(QJSValue& object, JsPropertyItem* parentItem)
{
	const QMetaObject* objMeta = nullptr;

	if (object.isObject())
	{
		auto* qobjPtr = object.toQObject();
		if (qobjPtr != nullptr)
		{
			objMeta = qobjPtr->metaObject();
		}
	}

	QJSValueIterator it(object);

	while (it.hasNext())
	{
		it.next();
		QJSValue child = it.value();

		bool isPrototype = it.name() == "prototype";
		//printf("ProtoType: %s :: %s\n", object.toString().toLocal8Bit().constData(), child.toString().toLocal8Bit().constData());

		if (!isPrototype)
		{
			JsPropertyItem* item = nullptr;
			QString name = it.name();
			QString value;
			const char *type = "unknown";
			bool itemIsNew = false;

			if (parentItem == nullptr)
			{
				auto it = propTree->childMap.find(name);

				if (it != propTree->childMap.end())
				{
					item = it.value();
				}
			}
			else
			{
				auto it = parentItem->childMap.find(name);

				if (it != parentItem->childMap.end())
				{
					item = it.value();
				}
			}
			if (item == nullptr)
			{
				item = new JsPropertyItem();
				itemIsNew = true;
			}

			if (child.isArray())
			{
				type = "array";
			}
			else if (child.isBool())
			{
				type = "bool";
				value = QVariant(child.toBool()).toString();
			}
			else if (child.isCallable())
			{
				type = "function";
				value = "";

				if (objMeta != nullptr)
				{
					//printf("Function: %s::%s\n", objMeta->className(), name.toLocal8Bit().constData());
					for (int i=0; i<objMeta->methodCount(); i++)
					{
						QMetaMethod m = objMeta->method(i);

						//printf("Method: %s  %s %s\n", 
						//		m.name().constData(),
						//		m.typeName(),
						//		m.methodSignature().constData());

						if (name == m.name())
						{
							value = QString("   ") + QString(m.typeName()) + " " +
								QString::fromLocal8Bit(m.methodSignature());
						}
					}
				}
			}
			else if (child.isDate())
			{
				type = "date";
				value = child.toDateTime().toString();
			}
			else if (child.isError())
			{
				type = "error";
				value = child.toString();
			}
			else if (child.isNull())
			{
				type = "null";
				value = "null";
			}
			else if (child.isNumber())
			{
				type = "number";
				value = QVariant(child.toNumber()).toString();
			}
			else if (child.isRegExp())
			{
				type = "RegExp";
				value.clear();
			}
			else if (child.isQMetaObject())
			{
				type = "meta";
				value.clear();

				auto* meta = child.toQMetaObject();
				if (meta != nullptr)
				{
					value = meta->className();
				}
			}
			else if (child.isObject())
			{
				type = "object";
				value.clear();

				auto* obj = child.toQObject();
				if (obj != nullptr)
				{
					auto* meta = obj->metaObject();
					if (meta != nullptr)
					{
						value = meta->className();
					}
				}
			}
			else if (child.isString())
			{
				type = "string";
				value = child.toString();
			}


			if (itemIsNew)
			{
				item->setText(0, name);
				item->setText(1, type);
				item->setText(2, value);
			}
			else
			{
				bool itemHasChanged = !item->jsValue.strictlyEquals(child);

				if (itemHasChanged)
				{
					item->setText(2, value);
				}
			}
			item->jsValue = child;

			if (itemIsNew)
			{
				if (parentItem == nullptr)
				{
					propTree->addTopLevelItem(item);
					propTree->childMap[name] = item;
				}
				else
				{
					parentItem->addChild(item);
					parentItem->childMap[name] = item;
				}
			}

			if (child.isObject())
			{
				loadPropertyTree(child, item);
			}
		}
	}
}
//----------------------------------------------------
void QScriptDialog_t::reloadGlobalTree(void)
{
	if (scriptInstance != nullptr)
	{
		auto* engine = scriptInstance->getEngine();

		if (engine)
		{
			QJSValue globals = engine->globalObject();

			loadPropertyTree(globals);
		}
	}
}
//----------------------------------------------------
void QScriptDialog_t::updatePeriodic(void)
{
	//printf("Update JS\n");
	FCEU_WRAPPER_LOCK();
	if (!emuThreadText.isEmpty())
	{
		auto* vbar = jsOutput->verticalScrollBar();
		int vbarValue = vbar->value();
		bool vbarAtMax = vbarValue >= vbar->maximum();

		jsOutput->insertPlainText(emuThreadText);

		if (vbarAtMax)
		{
			vbar->setValue( vbar->maximum() );
		}
		emuThreadText.clear();
	}

	if ((scriptInstance != nullptr) && scriptInstance->isRunning())
	{
		reloadGlobalTree();
	}

	refreshState();
	FCEU_WRAPPER_UNLOCK();
}
//----------------------------------------------------
void QScriptDialog_t::openJSKillMessageBox(void)
{
	int ret;
	QMessageBox msgBox(this);

	msgBox.setIcon(QMessageBox::Warning);
	msgBox.setText(tr("The JS script running has been running a long time.\nIt may have gone crazy. Kill it? (I won't ask again if you say No)\n"));
	msgBox.setStandardButtons(QMessageBox::Yes);
	msgBox.addButton(QMessageBox::No);
	msgBox.setDefaultButton(QMessageBox::No);

	ret = msgBox.exec();

	if (ret == QMessageBox::Yes)
	{
	}
}
//----------------------------------------------------
void QScriptDialog_t::openScriptFile(void)
{
	int ret, useNativeFileDialogVal;
	QString filename;
	std::string last;
	std::string dir;
	const char *exePath = nullptr;
	const char *jsPath = nullptr;
	QFileDialog dialog(this, tr("Open JS Script"));
	QList<QUrl> urls;
	QDir d;

	exePath = fceuExecutablePath();

	//urls = dialog.sidebarUrls();
	urls << QUrl::fromLocalFile(QDir::rootPath());
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first());
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::DesktopLocation).first());
	urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::DownloadLocation).first());
	urls << QUrl::fromLocalFile(QDir(FCEUI_GetBaseDirectory()).absolutePath());

	if (exePath[0] != 0)
	{
		d.setPath(QString(exePath) + "/../jsScripts");

		if (d.exists())
		{
			urls << QUrl::fromLocalFile(d.absolutePath());
		}
	}
#ifndef WIN32
	d.setPath("/usr/share/fceux/jsScripts");

	if (d.exists())
	{
		urls << QUrl::fromLocalFile(d.absolutePath());
	}
#endif

	jsPath = getenv("FCEU_QSCRIPT_PATH");

	// Parse LUA_PATH and add to urls
	if (jsPath)
	{
		int i, j;
		char stmp[2048];

		i = j = 0;
		while (jsPath[i] != 0)
		{
			if (jsPath[i] == ';')
			{
				stmp[j] = 0;

				if (j > 0)
				{
					d.setPath(stmp);

					if (d.exists())
					{
						urls << QUrl::fromLocalFile(d.absolutePath());
					}
				}
				j = 0;
			}
			else
			{
				stmp[j] = jsPath[i];
				j++;
			}
			i++;
		}

		stmp[j] = 0;

		if (j > 0)
		{
			d.setPath(stmp);

			if (d.exists())
			{
				urls << QUrl::fromLocalFile(d.absolutePath());
			}
		}
	}

	dialog.setFileMode(QFileDialog::ExistingFile);

	dialog.setNameFilter(tr("JS Scripts (*.js *.JS) ;; All files (*)"));

	dialog.setViewMode(QFileDialog::List);
	dialog.setFilter(QDir::AllEntries | QDir::AllDirs | QDir::Hidden);
	dialog.setLabelText(QFileDialog::Accept, tr("Load"));

	g_config->getOption("SDL.LastLoadJs", &last);

	if (last.size() == 0)
	{
#ifdef WIN32
		last.assign(FCEUI_GetBaseDirectory());
#else
		last.assign("/usr/share/fceux/jsScripts");
#endif
	}

	getDirFromFile(last.c_str(), dir);

	dialog.setDirectory(tr(dir.c_str()));

	// Check config option to use native file dialog or not
	g_config->getOption("SDL.UseNativeFileDialog", &useNativeFileDialogVal);

	dialog.setOption(QFileDialog::DontUseNativeDialog, !useNativeFileDialogVal);
	dialog.setSidebarUrls(urls);

	ret = dialog.exec();

	if (ret)
	{
		QStringList fileList;
		fileList = dialog.selectedFiles();

		if (fileList.size() > 0)
		{
			filename = fileList[0];
		}
	}

	if (filename.isNull())
	{
		return;
	}
	//qDebug() << "selected file path : " << filename.toLocal8Bit();

	g_config->setOption("SDL.LastLoadJs", filename.toLocal8Bit().constData());

	scriptPath->setText(filename);

}
//----------------------------------------------------
void QScriptDialog_t::resetLog()
{
	if (logFile != nullptr)
	{
		delete logFile;
		logFile = nullptr;
	}
	logFile = new QTemporaryFile(this);
	logFile->setAutoRemove(true);
	logFile->setFileTemplate(QDir::tempPath() + QString("/fceux_js_XXXXXX.log"));
	logFile->open();
	QString link = QString("<a href=\"file://") + 
		logFile->fileName() + QString("\">") + logFile->fileName() + QString("</a>");
	logFilepath->setText( link );
}
//----------------------------------------------------
void QScriptDialog_t::startScript(void)
{
	FCEU_WRAPPER_LOCK();
	resetLog();
	jsOutput->clear();
	clearPropertyTree();
	scriptInstance->resetEngine();
	if (scriptInstance->loadScriptFile(scriptPath->text()))
	{
		// Script parsing error
		FCEU_WRAPPER_UNLOCK();
		return;
	}
	// Pass command line arguments to script main.
	QStringList argStringList = scriptArgs->text().split(" ", Qt::SkipEmptyParts);
	QJSValue argArray = scriptInstance->getEngine()->newArray(argStringList.size());
	for (int i=0; i<argStringList.size(); i++)
	{
		argArray.setProperty(i, argStringList[i]);
	}

	QJSValueList argList = { argArray };

	scriptInstance->call("main", argList);

	refreshState();

	QJSValue globals = scriptInstance->getEngine()->globalObject();

	loadPropertyTree(globals);

	FCEU_WRAPPER_UNLOCK();
}
//----------------------------------------------------
void QScriptDialog_t::stopScript(void)
{
	FCEU_WRAPPER_LOCK();
	scriptInstance->stopRunning();
	refreshState();
	FCEU_WRAPPER_UNLOCK();
}
//----------------------------------------------------
void QScriptDialog_t::refreshState(void)
{
	if (scriptInstance->isRunning())
	{
		stopButton->setEnabled(true);
		startButton->setText(tr("Restart"));
	}
	else
	{
		stopButton->setEnabled(false);
		startButton->setText(tr("Start"));
	}
}
//----------------------------------------------------
void QScriptDialog_t::logOutput(const QString& text)
{
	if (QThread::currentThread() == consoleWindow->emulatorThread)
	{
		emuThreadText.append(text);
	}
	else
	{
		auto* vbar = jsOutput->verticalScrollBar();
		int vbarValue = vbar->value();
		bool vbarAtMax = vbarValue >= vbar->maximum();

		jsOutput->insertPlainText(text);

		if (vbarAtMax)
		{
			vbar->setValue( vbar->maximum() );
		}
	}

	if (logFile != nullptr)
	{
		logFile->write( text.toLocal8Bit() );
	}
}
//----------------------------------------------------
bool FCEU_JSRerecordCountSkip()
{
	return JS::MovieScriptObject::skipRerecords;
}
//----------------------------------------------------
uint8_t FCEU_JSReadJoypad(int which, uint8_t joyl)
{
	return JS::JoypadScriptObject::readOverride(which, joyl);
}
//----------------------------------------------------
#endif // __FCEU_QSCRIPT_ENABLE__
