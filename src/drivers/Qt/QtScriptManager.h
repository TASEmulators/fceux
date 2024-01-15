// QtScriptManager.h
//

#pragma once

#ifdef __FCEU_QSCRIPT_ENABLE__

#include <stdio.h>
#include <stdarg.h>

#include <QWidget>
#include <QDialog>
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
	Q_INVOKABLE  void message(const QString& msg);
	Q_INVOKABLE  void speedMode(const QString& mode);
	Q_INVOKABLE  bool loadRom(const QString& romPath);
	Q_INVOKABLE  QString getDir();

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
	QJSValue onFrameFinishCallback;
	QJSValue onScriptStopCallback;
	bool running = false;

public slots:
	Q_INVOKABLE  void print(const QString& msg);
	Q_INVOKABLE  void loadUI(const QString& uiFilePath);
	Q_INVOKABLE  QString openFileBrowser(const QString& initialPath = QString());
};

class QtScriptManager : public QObject
{
	Q_OBJECT

public:
	QtScriptManager(QObject* parent = nullptr);
	~QtScriptManager();

	static QtScriptManager* getInstance(){ return _instance; }
	static QtScriptManager* create(QObject* parent = nullptr);

	void addScriptInstance(QtScriptInstance* script);
	void removeScriptInstance(QtScriptInstance* script);
private:
	static QtScriptManager* _instance;

	QList<QtScriptInstance*> scriptList;
	FCEU::timeStampRecord lastFrameUpdate;

public slots:
	void frameFinishedUpdate();
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

	QTimer *periodicTimer;
	QLineEdit *scriptPath;
	QLineEdit *scriptArgs;
	QPushButton *browseButton;
	QPushButton *stopButton;
	QPushButton *startButton;
	QTextEdit *jsOutput;
	QtScriptInstance *scriptInstance;

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
