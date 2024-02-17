// QtScriptManager.h
//

#pragma once

#ifdef __FCEU_QSCRIPT_ENABLE__

#include <stdio.h>
#include <stdarg.h>

#include <QColor>
#include <QWidget>
#include <QDialog>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QGroupBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QTreeView>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QJSEngine>
#include <QThread>
#include <QTemporaryFile>
#include <QMenu>
#include <QMenuBar>
#include <QAction>

#include "Qt/main.h"
#include "utils/mutex.h"
#include "utils/timeStamp.h"

class QScriptDialog_t;
class QtScriptInstance;

namespace FCEU
{
	class JSEngine : public QJSEngine
	{
		Q_OBJECT
	public:
		JSEngine(QObject* parent = nullptr);
		~JSEngine();

		QScriptDialog_t* getDialog(){ return dialog; }
		QtScriptInstance* getScript(){ return script; }

		void setDialog(QScriptDialog_t* _dialog){ dialog = _dialog; }
		void setScript(QtScriptInstance* _script){ script = _script; }

		enum logLevel
		{
			FATAL = 0,
			CRITICAL,
			WARNING,
			INFO,
			DEBUG,
		};

		void acquireThreadContext();
		void releaseThreadContext();
		void setLogLevel(enum logLevel lvl){ _logLevel = lvl; }
		void logMessage(int lvl, const QString& msg);

		static JSEngine* getCurrent();
	private:
		QScriptDialog_t* dialog = nullptr;
		QtScriptInstance* script = nullptr;

		int _logLevel = WARNING;
		JSEngine*  prevContext = nullptr;

	};
} // FCEU

namespace JS
{

class ColorScriptObject: public QObject
{
	Q_OBJECT
	Q_PROPERTY(int red READ getRed WRITE setRed)
	Q_PROPERTY(int green READ getGreen WRITE setGreen)
	Q_PROPERTY(int blue READ getBlue WRITE setBlue)
	Q_PROPERTY(int palette READ getPalette WRITE setPalette)
public:
	Q_INVOKABLE ColorScriptObject(int r=0, int g=0, int b=0);
	~ColorScriptObject();

private:
	QColor color;
	int    _palette;
	static int numInstances;

public slots:
	Q_INVOKABLE  int getRed(){ return color.red(); }
	Q_INVOKABLE  int getGreen(){ return color.green(); }
	Q_INVOKABLE  int getBlue(){ return color.blue(); }
	Q_INVOKABLE  void setRed(int r){ color.setRed(r); }
	Q_INVOKABLE  void setGreen(int g){ color.setGreen(g); }
	Q_INVOKABLE  void setBlue(int b){ color.setBlue(b); }
	Q_INVOKABLE  int getPalette(){ return _palette; }
	Q_INVOKABLE  void setPalette(int p){ _palette = p; }
	Q_INVOKABLE  int toRGB8(){ return color.value(); }
	Q_INVOKABLE  QString name(){ return color.name(QColor::HexRgb); }
};

class JoypadScriptObject: public QObject
{
	Q_OBJECT
	Q_PROPERTY(bool up READ getUp)
	Q_PROPERTY(bool down READ getDown)
	Q_PROPERTY(bool left READ getLeft)
	Q_PROPERTY(bool right READ getRight)
	Q_PROPERTY(bool select READ getSelect)
	Q_PROPERTY(bool start READ getStart)
	Q_PROPERTY(bool a READ getA)
	Q_PROPERTY(bool b READ getB)
	Q_PROPERTY(int  player READ getPlayer WRITE setPlayer)
public:
	Q_INVOKABLE JoypadScriptObject(int playerIdx = 0, bool immediate = false);
	~JoypadScriptObject();

	static constexpr int MAX_JOYPAD_PLAYERS = 4;

	enum Button
	{
		FIRST_BUTTON = 0,
		A_BUTTON = FIRST_BUTTON,
		B_BUTTON,
		SELECT_BUTTON,
		START_BUTTON,
		UP_BUTTON,
		DOWN_BUTTON,
		LEFT_BUTTON,
		RIGHT_BUTTON,
		LAST_BUTTON = RIGHT_BUTTON,
		END_BUTTON
	};
	Q_ENUM(Button);

	// Joypad Override Function
	static uint8_t readOverride(int which, uint8_t joyl);

private:
	// Joypad Override Bit Masks
	static uint8_t jsOverrideMask1[MAX_JOYPAD_PLAYERS];
	static uint8_t jsOverrideMask2[MAX_JOYPAD_PLAYERS];

	struct buttonState
	{
		uint32_t  buttonMask = 0;
		bool   _immediate = false;
	};
	buttonState  current;
	buttonState  prev;

	int    player = 0;
	static int numInstances;

	static constexpr uint32_t ButtonMaskA      = 0x01;
	static constexpr uint32_t ButtonMaskB      = 0x02;
	static constexpr uint32_t ButtonMaskSelect = 0x04;
	static constexpr uint32_t ButtonMaskStart  = 0x08;
	static constexpr uint32_t ButtonMaskUp     = 0x10;
	static constexpr uint32_t ButtonMaskDown   = 0x20;
	static constexpr uint32_t ButtonMaskLeft   = 0x40;
	static constexpr uint32_t ButtonMaskRight  = 0x80;

	template <uint32_t mask> bool isBitSet(uint32_t& byte)
	{
		return (byte & mask) ? true : false;
	}

	template <uint32_t mask> bool isChanged()
	{
		return ((current.buttonMask ^ prev.buttonMask) & mask) ? true : false;
	}

	template <uint32_t mask> void setButtonOverride(bool value)
	{
		if (value)
		{
			jsOverrideMask1[player] |= mask;
			jsOverrideMask2[player] |= mask;
		}
		else
		{
			jsOverrideMask1[player] &= ~mask;
			jsOverrideMask2[player] &= ~mask;
		}
	}

	template <uint32_t mask> void clearButtonOverride()
	{
		jsOverrideMask1[player] |=  mask;
		jsOverrideMask2[player] &= ~mask;
	}

	template <uint32_t mask> void invertButtonOverride()
	{
		jsOverrideMask1[player] &= ~mask;
		jsOverrideMask2[player] |=  mask;
	}

public slots:
	Q_INVOKABLE  void refreshData(bool immediate = false);

	Q_INVOKABLE  bool getUp(){ return isBitSet<ButtonMaskUp>(current.buttonMask); }
	Q_INVOKABLE  bool getDown(){ return isBitSet<ButtonMaskDown>(current.buttonMask); }
	Q_INVOKABLE  bool getLeft(){ return isBitSet<ButtonMaskLeft>(current.buttonMask); }
	Q_INVOKABLE  bool getRight(){ return isBitSet<ButtonMaskRight>(current.buttonMask); }
	Q_INVOKABLE  bool getSelect(){ return isBitSet<ButtonMaskSelect>(current.buttonMask); }
	Q_INVOKABLE  bool getStart(){ return isBitSet<ButtonMaskStart>(current.buttonMask); }
	Q_INVOKABLE  bool getA(){ return isBitSet<ButtonMaskA>(current.buttonMask); }
	Q_INVOKABLE  bool getB(){ return isBitSet<ButtonMaskB>(current.buttonMask); }

	Q_INVOKABLE  bool upChanged(){ return isChanged<ButtonMaskUp>(); }
	Q_INVOKABLE  bool downChanged(){ return isChanged<ButtonMaskDown>(); }
	Q_INVOKABLE  bool leftChanged(){ return isChanged<ButtonMaskLeft>(); }
	Q_INVOKABLE  bool rightChanged(){ return isChanged<ButtonMaskRight>(); }
	Q_INVOKABLE  bool selectChanged(){ return isChanged<ButtonMaskSelect>(); }
	Q_INVOKABLE  bool startChanged(){ return isChanged<ButtonMaskStart>(); }
	Q_INVOKABLE  bool aChanged(){ return isChanged<ButtonMaskA>(); }
	Q_INVOKABLE  bool bChanged(){ return isChanged<ButtonMaskB>(); }

	Q_INVOKABLE  bool isImmediate(){ return current._immediate; }
	Q_INVOKABLE  bool getButton(enum Button b);
	Q_INVOKABLE  bool buttonChanged(enum Button b);
	Q_INVOKABLE  bool stateChanged(){ return prev.buttonMask != current.buttonMask; }
	Q_INVOKABLE  void setState(int mask){ prev.buttonMask = current.buttonMask; current.buttonMask = mask; }
	Q_INVOKABLE  int  getState(){ return current.buttonMask; }
	Q_INVOKABLE  int  maxPlayers(){ return MAX_JOYPAD_PLAYERS; }
	Q_INVOKABLE  int  getPlayer(){ return player; }
	Q_INVOKABLE  void setPlayer(int newPlayerIdx){ player = newPlayerIdx; }

	Q_INVOKABLE  void ovrdClearA(){ clearButtonOverride<ButtonMaskA>(); }
	Q_INVOKABLE  void ovrdClearB(){ clearButtonOverride<ButtonMaskB>(); }
	Q_INVOKABLE  void ovrdClearStart(){ clearButtonOverride<ButtonMaskStart>(); }
	Q_INVOKABLE  void ovrdClearSelect(){ clearButtonOverride<ButtonMaskSelect>(); }
	Q_INVOKABLE  void ovrdClearUp(){ clearButtonOverride<ButtonMaskUp>(); }
	Q_INVOKABLE  void ovrdClearDown(){ clearButtonOverride<ButtonMaskDown>(); }
	Q_INVOKABLE  void ovrdClearLeft(){ clearButtonOverride<ButtonMaskLeft>(); }
	Q_INVOKABLE  void ovrdClearRight(){ clearButtonOverride<ButtonMaskRight>(); }
	Q_INVOKABLE  void ovrdClear(){ ovrdReset(); }

	Q_INVOKABLE  void ovrdInvertA(){ invertButtonOverride<ButtonMaskA>(); }
	Q_INVOKABLE  void ovrdInvertB(){ invertButtonOverride<ButtonMaskB>(); }
	Q_INVOKABLE  void ovrdInvertStart(){ invertButtonOverride<ButtonMaskStart>(); }
	Q_INVOKABLE  void ovrdInvertSelect(){ invertButtonOverride<ButtonMaskSelect>(); }
	Q_INVOKABLE  void ovrdInvertUp(){ invertButtonOverride<ButtonMaskUp>(); }
	Q_INVOKABLE  void ovrdInvertDown(){ invertButtonOverride<ButtonMaskDown>(); }
	Q_INVOKABLE  void ovrdInvertLeft(){ invertButtonOverride<ButtonMaskLeft>(); }
	Q_INVOKABLE  void ovrdInvertRight(){ invertButtonOverride<ButtonMaskRight>(); }

	Q_INVOKABLE  void ovrdA(bool value){ setButtonOverride<ButtonMaskA>(value); }
	Q_INVOKABLE  void ovrdB(bool value){ setButtonOverride<ButtonMaskB>(value); }
	Q_INVOKABLE  void ovrdStart(bool value){ setButtonOverride<ButtonMaskStart>(value); }
	Q_INVOKABLE  void ovrdSelect(bool value){ setButtonOverride<ButtonMaskSelect>(value); }
	Q_INVOKABLE  void ovrdUp(bool value){ setButtonOverride<ButtonMaskUp>(value); }
	Q_INVOKABLE  void ovrdDown(bool value){ setButtonOverride<ButtonMaskDown>(value); }
	Q_INVOKABLE  void ovrdLeft(bool value){ setButtonOverride<ButtonMaskLeft>(value); }
	Q_INVOKABLE  void ovrdRight(bool value){ setButtonOverride<ButtonMaskRight>(value); }

	Q_INVOKABLE  void ovrdReset()
	{
		jsOverrideMask1[player] = 0xFF;
		jsOverrideMask2[player] = 0x00;
	}

	Q_INVOKABLE  static void ovrdResetAll();
};

class EmuStateScriptObject: public QObject
{
	Q_OBJECT
public:
	Q_INVOKABLE EmuStateScriptObject(const QJSValue& jsArg1 = QJSValue(), const QJSValue& jsArg2 = QJSValue());
	~EmuStateScriptObject();

	Q_PROPERTY(bool persist READ isPersistent WRITE setPersistent)
	Q_PROPERTY(int slot READ getSlot WRITE setSlot)
	Q_PROPERTY(int compressionLevel READ getCompressionLevel WRITE setCompressionLevel)

	EmuStateScriptObject& operator= (const EmuStateScriptObject& obj);

private:
	QString filename;
	EMUFILE_MEMORY *data = nullptr;
	int compression = 0;
	int slot = -1;
	bool persist = false;

	static int numInstances;

	void logMessage(int lvl, QString& msg);

public slots:
	Q_INVOKABLE  bool  save();
	Q_INVOKABLE  bool  load();
	Q_INVOKABLE  bool  isValid(){ return (data != nullptr); }
	Q_INVOKABLE  bool  isPersistent(){ return persist; }
	Q_INVOKABLE  void  setPersistent(bool value){ persist = value; }
	Q_INVOKABLE  int   getSlot(){ return slot; }
	Q_INVOKABLE  void  setSlot(int value);
	Q_INVOKABLE  int   getCompressionLevel(){ return compression; }
	Q_INVOKABLE  void  setCompressionLevel(int value){ compression = value; }
	Q_INVOKABLE  void  setFilename(const QString& name){ filename = name; }
	Q_INVOKABLE  bool  saveToFile(const QString& filepath);
	Q_INVOKABLE  bool  loadFromFile(const QString& filepath);
	Q_INVOKABLE  bool  saveFileExists();
	Q_INVOKABLE  QJSValue  copy();
};

class EmuScriptObject: public QObject
{
	Q_OBJECT
public:
	EmuScriptObject(QObject* parent = nullptr);
	~EmuScriptObject();

	void setEngine(FCEU::JSEngine* _engine){ engine = _engine; }
	void setDialog(QScriptDialog_t* _dialog){ dialog = _dialog; }

private:
	FCEU::JSEngine* engine = nullptr;
	QScriptDialog_t* dialog = nullptr;
	QtScriptInstance* script = nullptr;

public slots:
	Q_INVOKABLE  void print(const QString& msg);
	Q_INVOKABLE  void powerOn();
	Q_INVOKABLE  void softReset();
	Q_INVOKABLE  void pause();
	Q_INVOKABLE  void unpause();
	Q_INVOKABLE  bool paused();
	Q_INVOKABLE  void frameAdvance();
	Q_INVOKABLE  int  frameCount();
	Q_INVOKABLE  int  lagCount();
	Q_INVOKABLE  bool lagged();
	Q_INVOKABLE  void setLagFlag(bool flag);
	Q_INVOKABLE  bool emulating();
	Q_INVOKABLE  bool isReadOnly();
	Q_INVOKABLE  void setReadOnly(bool flag);
	Q_INVOKABLE  void setRenderPlanes(bool sprites, bool background);
	Q_INVOKABLE  void registerBeforeFrame(const QJSValue& func);
	Q_INVOKABLE  void registerAfterFrame(const QJSValue& func);
	Q_INVOKABLE  void registerStop(const QJSValue& func);
	Q_INVOKABLE  void message(const QString& msg);
	Q_INVOKABLE  void speedMode(const QString& mode);
	Q_INVOKABLE  bool loadRom(const QString& romPath);
	Q_INVOKABLE  bool onEmulationThread();
	Q_INVOKABLE  bool addGameGenie(const QString& code);
	Q_INVOKABLE  bool delGameGenie(const QString& code);
	Q_INVOKABLE  void exit();
	Q_INVOKABLE  QString getDir();
	Q_INVOKABLE  QJSValue getScreenPixel(int x, int y, bool useBackup = false);
	Q_INVOKABLE  QJSValue createState(int slot = -1);

};

class RomScriptObject: public QObject
{
	Q_OBJECT
public:
	RomScriptObject(QObject* parent = nullptr);
	~RomScriptObject();

	void setEngine(FCEU::JSEngine* _engine){ engine = _engine; }
	void setDialog(QScriptDialog_t* _dialog){ dialog = _dialog; }
private:
	FCEU::JSEngine* engine = nullptr;
	QScriptDialog_t* dialog = nullptr;
	QtScriptInstance* script = nullptr;

public slots:
	Q_INVOKABLE  bool    isLoaded();
	Q_INVOKABLE QString  getFileName();
	Q_INVOKABLE QString  getHash(const QString& type);
	Q_INVOKABLE  uint8_t readByte(int address);
	Q_INVOKABLE   int8_t readByteSigned(int address);
	Q_INVOKABLE  uint8_t readByteUnsigned(int address);
	Q_INVOKABLE QJSValue readByteRange(int start, int end);
	Q_INVOKABLE  void    writeByte(int address, int value);
};

class MemoryScriptObject: public QObject
{
	Q_OBJECT
public:
	MemoryScriptObject(QObject* parent = nullptr);
	~MemoryScriptObject();

	void setEngine(FCEU::JSEngine* _engine){ engine = _engine; }
	void setDialog(QScriptDialog_t* _dialog){ dialog = _dialog; }
	void reset();

	QtScriptInstance* getScript(){ return script; }
	QJSValue* getReadFunc(int address) { return readFunc[address]; }
	QJSValue* getWriteFunc(int address) { return writeFunc[address]; }
	QJSValue* getExecFunc(int address) { return execFunc[address]; }

private:
	static constexpr int AddressRange = 0x10000;
	FCEU::JSEngine* engine = nullptr;
	QScriptDialog_t* dialog = nullptr;
	QtScriptInstance* script = nullptr;
	QJSValue* readFunc[AddressRange] = { nullptr };
	QJSValue* writeFunc[AddressRange] = { nullptr };
	QJSValue* execFunc[AddressRange] = { nullptr };
	int numReadFuncsRegistered = 0;
	int numWriteFuncsRegistered = 0;
	int numExecFuncsRegistered = 0;

	void registerCallback(int type, const QJSValue& func, int address, int size = 1);
	void unregisterCallback(int type, const QJSValue& func, int address, int size = 1);

public slots:
	Q_INVOKABLE  uint8_t readByte(int address);
	Q_INVOKABLE   int8_t readByteSigned(int address);
	Q_INVOKABLE  uint8_t readByteUnsigned(int address);
	Q_INVOKABLE uint16_t readWord(int addressLow, int addressHigh = -1);
	Q_INVOKABLE  int16_t readWordSigned(int addressLow, int addressHigh = -1);
	Q_INVOKABLE uint16_t readWordUnsigned(int addressLow, int addressHigh = -1);
	Q_INVOKABLE     void writeByte(int address, int value);
	Q_INVOKABLE uint16_t getRegisterPC();
	Q_INVOKABLE  uint8_t getRegisterA();
	Q_INVOKABLE  uint8_t getRegisterX();
	Q_INVOKABLE  uint8_t getRegisterY();
	Q_INVOKABLE  uint8_t getRegisterS();
	Q_INVOKABLE  uint8_t getRegisterP();
	Q_INVOKABLE     void setRegisterPC(uint16_t v);
	Q_INVOKABLE     void setRegisterA(uint8_t v);
	Q_INVOKABLE     void setRegisterX(uint8_t v);
	Q_INVOKABLE     void setRegisterY(uint8_t v);
	Q_INVOKABLE     void setRegisterS(uint8_t v);
	Q_INVOKABLE     void setRegisterP(uint8_t v);
	Q_INVOKABLE     void registerRead(const QJSValue& func, int address, int size = 1);
	Q_INVOKABLE     void registerWrite(const QJSValue& func, int address, int size = 1);
	Q_INVOKABLE     void registerExec(const QJSValue& func, int address, int size = 1);
	Q_INVOKABLE     void unregisterRead(const QJSValue& func, int address, int size = 1);
	Q_INVOKABLE     void unregisterWrite(const QJSValue& func, int address, int size = 1);
	Q_INVOKABLE     void unregisterExec(const QJSValue& func, int address, int size = 1);
	Q_INVOKABLE     void unregisterAll();
};

class PpuScriptObject: public QObject
{
	Q_OBJECT
public:
	PpuScriptObject(QObject* parent = nullptr);
	~PpuScriptObject();

	void setEngine(FCEU::JSEngine* _engine){ engine = _engine; }
	void setDialog(QScriptDialog_t* _dialog){ dialog = _dialog; }
private:
	FCEU::JSEngine* engine = nullptr;
	QScriptDialog_t* dialog = nullptr;
	QtScriptInstance* script = nullptr;

public slots:
	Q_INVOKABLE  uint8_t readByte(int address);
	Q_INVOKABLE   int8_t readByteSigned(int address);
	Q_INVOKABLE  uint8_t readByteUnsigned(int address);
	Q_INVOKABLE QJSValue readByteRange(int start, int end);
	Q_INVOKABLE  void    writeByte(int address, int value);
};

class MovieScriptObject: public QObject
{
	Q_OBJECT
public:
	MovieScriptObject(QObject* parent = nullptr);
	~MovieScriptObject();

	void setEngine(FCEU::JSEngine* _engine){ engine = _engine; }

	enum Mode
	{
		IDLE = 0,
		RECORD,
		PLAYBACK,
		FINISHED,
		TAS_EDITOR
	};
	Q_ENUM(Mode);

	enum SaveType
	{
		FROM_POWERON = 0,
		FROM_SAVESTATE,
		FROM_SAVERAM,
	};
	Q_ENUM(SaveType);

	static bool skipRerecords;
private:
	FCEU::JSEngine* engine = nullptr;
	QtScriptInstance* script = nullptr;

public slots:
	Q_INVOKABLE  bool active();
	Q_INVOKABLE  bool isPlaying();
	Q_INVOKABLE  bool isRecording();
	Q_INVOKABLE  bool isPowerOn();
	Q_INVOKABLE  bool isFromSaveState();
	Q_INVOKABLE  void replay();
	Q_INVOKABLE  bool getReadOnly();
	Q_INVOKABLE  void setReadOnly(bool which);
	Q_INVOKABLE  bool play(const QString& filename, bool readOnly = false, int pauseFrame = 0);
	Q_INVOKABLE  bool record(const QString& filename, int saveType = FROM_POWERON, const QString author = QString());
	Q_INVOKABLE  int  frameCount();
	Q_INVOKABLE  int  mode();
	Q_INVOKABLE  void stop();
	Q_INVOKABLE  int  length();
	Q_INVOKABLE  int  rerecordCount();
	Q_INVOKABLE  void rerecordCounting(bool counting);
	Q_INVOKABLE  QString  getFilename();
	Q_INVOKABLE  QString  getFilepath();

	Q_INVOKABLE  void close(){ stop(); }
	Q_INVOKABLE  bool readOnly(){ return getReadOnly(); }
	Q_INVOKABLE  void playBeginning(){ replay(); }
	Q_INVOKABLE  bool playback(const QString& filename, bool readOnly = false, int pauseFrame = 0){ return play(filename, readOnly, pauseFrame); }
	Q_INVOKABLE  bool load(const QString& filename, bool readOnly = false, int pauseFrame = 0){ return play(filename, readOnly, pauseFrame); }
	Q_INVOKABLE  bool save(const QString& filename, int saveType = FROM_POWERON, const QString author = QString()){ return record(filename, saveType, author); }
};

class InputScriptObject: public QObject
{
	Q_OBJECT
public:
	InputScriptObject(QObject* parent = nullptr);
	~InputScriptObject();

	void setEngine(FCEU::JSEngine* _engine){ engine = _engine; }

private:
	FCEU::JSEngine* engine = nullptr;
	QtScriptInstance* script = nullptr;

public slots:
	Q_INVOKABLE QJSValue readJoypad(int player = 0, bool immediate = false);
	Q_INVOKABLE int  maxJoypadPlayers(){ return JoypadScriptObject::MAX_JOYPAD_PLAYERS; }
};

} // JS

class ScriptExecutionState
{
	public:
		void start()
		{
			timeMs = 0;
			executing = true;
		}
		void stop()
		{
			executing = false;
			timeMs = 0;
		}
		bool isRunning(){ return executing; }

		unsigned int timeCheck()
		{
			unsigned int retval = timeMs;
			timeMs += checkPeriod;
			return retval;
		}

		static constexpr unsigned int checkPeriod = 100;

	private:
		bool executing = false;
		unsigned int timeMs = 0;
};

class QtScriptInstance : public QObject
{
	Q_OBJECT
public:
	QtScriptInstance(QObject* parent = nullptr);
	~QtScriptInstance();

	void resetEngine();
	int loadScriptFile(QString filepath);

	bool isRunning(){ return running; };
	void stopRunning();

	int  call(const QString& funcName, const QJSValueList& args = QJSValueList());
	void frameAdvance();
	void onFrameBegin();
	void onFrameFinish();
	void onGuiUpdate();
	void checkForHang();
	void flushLog();
	int  runFunc(QJSValue &func, const QJSValueList& args = QJSValueList());

	int  throwError(QJSValue::ErrorType errorType, const QString &message = QString());

	FCEU::JSEngine* getEngine(){ return engine; };
private:

	int  initEngine();
	void shutdownEngine();
	void printSymbols(QJSValue& val, int iter = 0);
	void loadObjectChildren(QJSValue& jsObject, QObject* obj);

	ScriptExecutionState* getExecutionState();

	FCEU::JSEngine* engine = nullptr;
	QScriptDialog_t* dialog = nullptr;
	JS::EmuScriptObject* emu = nullptr;
	JS::RomScriptObject* rom = nullptr;
	JS::PpuScriptObject* ppu = nullptr;
	JS::MemoryScriptObject* mem = nullptr;
	JS::InputScriptObject* input = nullptr;
	JS::MovieScriptObject* movie = nullptr;
	QWidget* ui_rootWidget = nullptr;
	QJSValue *onFrameBeginCallback = nullptr;
	QJSValue *onFrameFinishCallback = nullptr;
	QJSValue *onScriptStopCallback = nullptr;
	QJSValue *onGuiUpdateCallback = nullptr;
	ScriptExecutionState guiFuncState;
	ScriptExecutionState emuFuncState;
	int frameAdvanceCount = 0;
	int frameAdvanceState = 0;
	bool running = false;

signals:
	void errorNotify();

public slots:
	Q_INVOKABLE  void print(const QString& msg);
	Q_INVOKABLE  void loadUI(const QString& uiFilePath);
	Q_INVOKABLE  QString openFileBrowser(const QString& initialPath = QString());
	Q_INVOKABLE  void registerBeforeEmuFrame(const QJSValue& func);
	Q_INVOKABLE  void registerAfterEmuFrame(const QJSValue& func);
	Q_INVOKABLE  void registerStop(const QJSValue& func);
	Q_INVOKABLE  void registerGuiUpdate(const QJSValue& func);
	Q_INVOKABLE  bool onGuiThread();
	Q_INVOKABLE  bool onEmulationThread();
};

class  ScriptMonitorThread_t : public QThread
{
	Q_OBJECT

	protected:
		void run( void ) override;

	public:
		ScriptMonitorThread_t( QObject *parent = 0 );
};

class QtScriptManager : public QObject
{
	Q_OBJECT

public:
	QtScriptManager(QObject* parent = nullptr);
	~QtScriptManager();

	static QtScriptManager* getInstance(){ return _instance; }
	static QtScriptManager* create(QObject* parent = nullptr);
	static void destroy();

	static void logMessageQt(QtMsgType type, const QString &msg);

	int  numScriptsLoaded(void){ return scriptList.size(); }
	void addScriptInstance(QtScriptInstance* script);
	void removeScriptInstance(QtScriptInstance* script);
private:
	static QtScriptManager* _instance;

	FCEU::mutex scriptListMutex;
	QList<QtScriptInstance*> scriptList;
	QTimer  *periodicUpdateTimer = nullptr;
	ScriptMonitorThread_t *monitorThread = nullptr;

	friend class ScriptMonitorThread_t; 

public slots:
	void frameBeginUpdate();
	void frameFinishedUpdate();
	void guiUpdate();
};

class JsPropertyItem : public QTreeWidgetItem
{
public:
	JsPropertyItem()
		: QTreeWidgetItem()
	{
	}

	virtual ~JsPropertyItem() override
	{
	}

	QJSValue  jsValue;
	QMap<QString, JsPropertyItem*> childMap;
};

class JsPropertyTree : public QTreeWidget
{
	Q_OBJECT

public:
	JsPropertyTree(QWidget *parent = nullptr)
		: QTreeWidget(parent)
	{
	}

	virtual ~JsPropertyTree() override
	{
	}

	QMap<QString, JsPropertyItem*> childMap;
};

class QScriptLogFile : public QTemporaryFile
{
	Q_OBJECT

public:
	QScriptLogFile(QObject* parent = nullptr)
		: QTemporaryFile(parent)
	{
	}

	~QScriptLogFile(void){}

	void reopen()
	{
		open(QIODeviceBase::Append | QIODeviceBase::ReadWrite);
	}
};

class QScriptDialog_t : public QDialog
{
	Q_OBJECT

public:
	QScriptDialog_t(QWidget *parent = nullptr);
	~QScriptDialog_t(void);

	void refreshState(void);
	void logOutput(const QString& text);

protected:
	void resetLog();
	void closeEvent(QCloseEvent *bar);
	void openJSKillMessageBox(void);
	void clearPropertyTree();
	void loadPropertyTree(QJSValue& val, JsPropertyItem* parentItem = nullptr);
	QMenuBar* buildMenuBar();

	QMenuBar* menuBar;
	QScriptLogFile *logFile = nullptr;
	QTimer *periodicTimer;
	QLineEdit *scriptPath;
	QLineEdit *scriptArgs;
	QPushButton *browseButton;
	QPushButton *stopButton;
	QPushButton *startButton;
	QPushButton *clearButton;
	QTabWidget *tabWidget;
	QTextEdit *jsOutput;
	JsPropertyTree *propTree;
	QtScriptInstance *scriptInstance;
	QString   emuThreadText;
	QString   logSavePath;
	QLabel *logFilepathLbl;
	QLabel *logFilepath;

private:
public slots:
	void flushLog();
	void closeWindow(void);

private slots:
	void saveLog(bool openFileBrowser = false);
	void updatePeriodic(void);
	void openScriptFile(void);
	void startScript(void);
	void stopScript(void);
	void onLogLinkClicked(const QString&);
	void onScriptError(void);
};

bool FCEU_JSRerecordCountSkip();

uint8_t FCEU_JSReadJoypad(int which, uint8_t phyState);

#endif // __FCEU_QSCRIPT_ENABLE__
