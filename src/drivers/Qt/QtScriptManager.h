// QtScriptManager.h
//

#pragma once

#ifdef __FCEU_QSCRIPT_ENABLE__

#include <stdio.h>
#include <stdarg.h>

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

#include "Qt/main.h"
#include "utils/timeStamp.h"

class QScriptDialog_t;
class QtScriptInstance;

class EmuScriptObject: public QObject
{
	Q_OBJECT
public:
	EmuScriptObject(QObject* parent = nullptr);
	~EmuScriptObject();

	void setEngine(QJSEngine* _engine){ engine = _engine; }
	void setDialog(QScriptDialog_t* _dialog){ dialog = _dialog; }

private:
	QJSEngine* engine = nullptr;
	QScriptDialog_t* dialog = nullptr;
	QtScriptInstance* script = nullptr;

public slots:
	Q_INVOKABLE  void print(const QString& msg);
	Q_INVOKABLE  void powerOn();
	Q_INVOKABLE  void softReset();
	Q_INVOKABLE  void pause();
	Q_INVOKABLE  void unpause();
	Q_INVOKABLE  bool paused();
	Q_INVOKABLE  int  framecount();
	Q_INVOKABLE  int  lagcount();
	Q_INVOKABLE  bool lagged();
	Q_INVOKABLE  void setlagflag(bool flag);
	Q_INVOKABLE  bool emulating();
	Q_INVOKABLE  void registerBefore(const QJSValue& func);
	Q_INVOKABLE  void registerAfter(const QJSValue& func);
	Q_INVOKABLE  void registerStop(const QJSValue& func);
	Q_INVOKABLE  void message(const QString& msg);
	Q_INVOKABLE  void speedMode(const QString& mode);
	Q_INVOKABLE  bool loadRom(const QString& romPath);
	Q_INVOKABLE  QString getDir();

};

class MemoryScriptObject: public QObject
{
	Q_OBJECT
public:
	MemoryScriptObject(QObject* parent = nullptr);
	~MemoryScriptObject();

	void setEngine(QJSEngine* _engine){ engine = _engine; }
	void setDialog(QScriptDialog_t* _dialog){ dialog = _dialog; }

private:
	QJSEngine* engine = nullptr;
	QScriptDialog_t* dialog = nullptr;
	QtScriptInstance* script = nullptr;

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
	void onFrameBegin();
	void onFrameFinish();

	int  throwError(QJSValue::ErrorType errorType, const QString &message = QString());

	QJSEngine* getEngine(){ return engine; };
private:

	int configEngine();
	void printSymbols(QJSValue& val, int iter = 0);
	void loadObjectChildren(QJSValue& jsObject, QObject* obj);

	QJSEngine* engine = nullptr;
	QScriptDialog_t* dialog = nullptr;
	EmuScriptObject* emu = nullptr;
	MemoryScriptObject* mem = nullptr;
	QWidget* ui_rootWidget = nullptr;
	QJSValue onFrameBeginCallback;
	QJSValue onFrameFinishCallback;
	QJSValue onScriptStopCallback;
	bool running = false;

public slots:
	Q_INVOKABLE  void print(const QString& msg);
	Q_INVOKABLE  void loadUI(const QString& uiFilePath);
	Q_INVOKABLE  QString openFileBrowser(const QString& initialPath = QString());
	Q_INVOKABLE  void registerBefore(const QJSValue& func);
	Q_INVOKABLE  void registerAfter(const QJSValue& func);
	Q_INVOKABLE  void registerStop(const QJSValue& func);
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

	QList<QtScriptInstance*> scriptList;
	FCEU::timeStampRecord lastFrameUpdate;

public slots:
	void frameBeginUpdate();
	void frameFinishedUpdate();
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
