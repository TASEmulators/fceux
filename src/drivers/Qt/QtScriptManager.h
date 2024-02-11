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

class QScriptDialog_t : public QDialog
{
	Q_OBJECT

public:
	QScriptDialog_t(QWidget *parent = nullptr);
	~QScriptDialog_t(void);

	void refreshState(void);
	void logOutput(const QString& text);

protected:
	void closeEvent(QCloseEvent *bar);
	void openJSKillMessageBox(void);
	void clearPropertyTree();
	void loadPropertyTree(QJSValue& val, JsPropertyItem* parentItem = nullptr);

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

private:
public slots:
	void closeWindow(void);
private slots:
	void updatePeriodic(void);
	void openScriptFile(void);
	void startScript(void);
	void stopScript(void);
};

// Formatted print
//int LuaPrintfToWindowConsole( __FCEU_PRINTF_FORMAT const char *format, ...) __FCEU_PRINTF_ATTRIBUTE( 1, 2 );

//void PrintToWindowConsole(intptr_t hDlgAsInt, const char *str);

//int LuaKillMessageBox(void);

#endif // __FCEU_QSCRIPT_ENABLE__
