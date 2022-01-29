
//

#ifndef __GameAppH__
#define __GameAppH__

#include <vector>
#include <string>

#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QPushButton>
#include <QMenu>
#include <QMenuBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QTimer>
#include <QThread>
#include <QCursor>
#include <QMutex>
#include <QColor>
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
#include <QRecursiveMutex>
#endif

#include "Qt/ConsoleViewerGL.h"
#include "Qt/ConsoleViewerSDL.h"
#include "Qt/GamePadConf.h"
#include "Qt/AviRecord.h"

class  emulatorThread_t : public QThread
{
	Q_OBJECT

	protected:
		void run( void ) override;

	public:
		emulatorThread_t( QObject *parent = 0 );

		void setPriority( QThread::Priority priority );

		#if defined(__linux__) || defined(__APPLE__) || defined(__unix__)
		int setSchedParam( int policy, int priority );
		int getSchedParam( int &policy, int &priority );
		int setNicePriority( int value );
		int getNicePriority( void );
		int getMinSchedPriority(void);
		int getMaxSchedPriority(void);
		#endif
		void signalFrameFinished(void);
		void signalRomLoad(const char *rom);
	private:
		void init(void);

		#if defined(__linux__) || defined(__APPLE__) || defined(__unix__)
		pthread_t  pself;
		int        pid;
		#endif

	signals:
		void finished(void);
		void frameFinished(void);
		void loadRomRequest( QString s );
};

class  consoleMenuBar : public QMenuBar
{
	public:
		consoleMenuBar(QWidget *parent = 0);
		~consoleMenuBar(void);

	protected:
		void keyPressEvent(QKeyEvent *event);
		void keyReleaseEvent(QKeyEvent *event);
};

class  autoFireMenuAction : public QAction
{
	Q_OBJECT

	public:
		autoFireMenuAction(int on, int off, QString name, QWidget *parent = 0);
		~autoFireMenuAction(void);

		bool isMatch( int on, int off );

		void setPattern( int on, int off );

		int  getOnValue(void){ return onFrames; };
		int  getOffValue(void){ return offFrames; };

	public slots:
		void activateCB(void);

	private:
		int  onFrames;
		int  offFrames;
};

class  consoleRecentRomAction : public QAction
{
	Q_OBJECT

	public:
		consoleRecentRomAction( QString title, QWidget *parent = 0);
		~consoleRecentRomAction(void);

		std::string  path;

	public slots:
		void activateCB(void);

};

class  consoleWin_t : public QMainWindow
{
	Q_OBJECT

	public:
		consoleWin_t(QWidget *parent = 0);
		~consoleWin_t(void);

		ConsoleViewGL_t   *viewport_GL;
		ConsoleViewSDL_t  *viewport_SDL;

		void setCyclePeriodms( int ms );

#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
		QRecursiveMutex *mutex;
#else
		QMutex *mutex;
#endif

		int  videoInit(void);
		void videoReset(void);
		void requestClose(void);

	 	void QueueErrorMsgWindow( const char *msg );

		int  showListSelectDialog( const char *title, std::vector <std::string> &l );

		#if defined(__linux__) || defined(__APPLE__) || defined(__unix__)
		int setSchedParam( int policy, int priority );
		int getSchedParam( int &policy, int &priority );
		int setNicePriority( int value );
		int getNicePriority( void );
		int getMinSchedPriority(void);
		int getMaxSchedPriority(void);
		#endif

		int loadVideoDriver( int driverId, bool force = false );

		double getRefreshRate(void){ return refreshRate; }

		emulatorThread_t *emulatorThread;
		AviRecordDiskThread_t *aviDiskThread;

		void addRecentRom( const char *rom );

		QSize  calcRequiredSize(void);

		void setViewportAspect(void);

		void loadCursor(void);
		void setViewerCursor( QCursor s );
		void setViewerCursor( Qt::CursorShape s );
		Qt::CursorShape getViewerCursor(void);

		void setMenuAccessPauseEnable(bool enable);
		void setContextMenuEnable(bool enable);
		void setSoundUseGlobalFocus(bool enable);

		void OpenHelpWindow(std::string subpage = "");

		int  getPeriodicInterval(void);

		QColor *getVideoBgColorPtr(void){ return &videoBgColor; }

	protected:
		consoleMenuBar *menubar;

		QMenu *fileMenu;
		QMenu *optMenu;
		QMenu *emuMenu;
		QMenu *toolsMenu;
		QMenu *debugMenu;
		QMenu *movieMenu;
		QMenu *helpMenu;
		QMenu *recentRomMenu;
		
		QAction *openROM;
		QAction *closeROM;
		QAction *playNSF;
		QAction *loadStateAct;
		QAction *saveStateAct;
		QAction *quickLoadAct;
		QAction *quickSaveAct;
		QAction *loadLuaAct;
		QAction *scrShotAct;
		QAction *quitAct;
		QAction *inputConfig;
		QAction *gamePadConfig;
		QAction *gameSoundConfig;
		QAction *gameVideoConfig;
		QAction *hotkeyConfig;
		QAction *paletteConfig;
		QAction *guiConfig;
		QAction *timingConfig;
		QAction *movieConfig;
		QAction *autoResume;
		QAction *winSizeAct[4];
		QAction *fullscreen;
		QAction *aboutAct;
		QAction *aboutActQt;
		QAction *msgLogAct;
		QAction *state[10];
		QAction *powerAct;
		QAction *resetAct;
		QAction *sresetAct;
		QAction *pauseAct;
		QAction *gameGenieAct;
		QAction *loadGgROMAct;
		QAction *insCoinAct;
		QAction *fdsSwitchAct;
		QAction *fdsEjectAct;
		QAction *fdsLoadBiosAct;
		QAction *cheatsAct;
		QAction *ramWatchAct;
		QAction *ramSearchAct;
		QAction *debuggerAct;
		QAction *codeDataLogAct;
		QAction *traceLogAct;
		QAction *hexEditAct;
		QAction *ppuViewAct;
		QAction *oamViewAct;
		QAction *ntViewAct;
		QAction *ggEncodeAct;
		QAction *iNesEditAct;
		QAction *openMovAct;
		QAction *playMovBeginAct;
		QAction *stopMovAct;
		QAction *recMovAct;
		QAction *region[3];
		QAction *ramInit[4];
		QAction *recAviAct;
		QAction *recAsAviAct;
		QAction *stopAviAct;
		QAction *recWavAct;
		QAction *recAsWavAct;
		QAction *stopWavAct;
		QAction *tasEditorAct;
		//QAction *aviHudAct;
		//QAction *aviMsgAct;

		QTimer  *gameTimer;
		QColor   videoBgColor;

		std::string errorMsg;
		bool        errorMsgValid;
		bool        closeRequested;
		bool        recentRomMenuReset;
		bool        firstResize;
		bool        mainMenuEmuPauseSet;
		bool        mainMenuEmuWasPaused;
		bool        mainMenuPauseWhenActv;
		bool        scrHandlerConnected;
		bool        contextMenuEnable;
		bool        soundUseGlobalFocus;

		std::list <std::string*> romList;
		std::vector <autoFireMenuAction*> afActList;
		autoFireMenuAction *afActCustom;

		double       refreshRate;
		unsigned int updateCounter;
#ifdef WIN32
		HWND   helpWin;
#else
		int    helpWin;
#endif
	protected:
		void resizeEvent(QResizeEvent *event) override;
		void closeEvent(QCloseEvent *event) override;
		void keyPressEvent(QKeyEvent *event) override;
		void keyReleaseEvent(QKeyEvent *event) override;
		void dragEnterEvent(QDragEnterEvent *event) override;
		void dropEvent(QDropEvent *event) override;
		void showEvent(QShowEvent *event) override;
		void contextMenuEvent(QContextMenuEvent *event) override;
		void syncActionConfig( QAction *act, const char *property );
		void showErrorMsgWindow(void);

	private:
		void initHotKeys(void);
		void initScreenHandler(void);
		void createMainMenu(void);
		void buildRecentRomMenu(void);
		void saveRecentRomMenu(void);
		void clearRomList(void);
		void setRegion(int region);
		void changeState(int slot);
		void saveState(int slot);
		void loadState(int slot);
		void transferVideoBuffer(void);
		void syncAutoFirePatternMenu(void);

		std::string findHelpFile(void);

	public slots:
		void openDebugWindow(void);
		void openHexEditor(void);
		void openGamePadConfWin(void);
		void toggleFullscreen(void);
		void toggleMenuVis(void);
		void recordMovie(void);
		void winResizeIx(int iScale);
	private slots:
		void closeApp(void);
		void openROMFile(void);
		void loadNSF(void);
		void loadStateFrom(void);
		void saveStateAs(void);
		void quickLoad(void);
		void quickSave(void);
		void closeROMCB(void);
		void aboutFCEUX(void);
		void aboutQt(void);
		void openOnlineDocs(void);
		void openOfflineDocs(void);
		void openTasEditor(void);
		void openMsgLogWin(void);
		void openInputConfWin(void);
		void openGameSndConfWin(void);
		void openGameVideoConfWin(void);
		void openHotkeyConfWin(void);
		void openPaletteConfWin(void);
		void openGuiConfWin(void);
		void openTimingConfWin(void);
		void openPaletteEditorWin(void);
		void openAviRiffViewer(void);
		void openTimingStatWin(void);
		void openMovieOptWin(void);
		void openCodeDataLogger(void);
		void openTraceLogger(void);
		void toggleAutoResume(void);
		void updatePeriodic(void);
		void changeState0(void);
		void changeState1(void);
		void changeState2(void);
		void changeState3(void);
		void changeState4(void);
		void changeState5(void);
		void changeState6(void);
		void changeState7(void);
		void changeState8(void);
		void changeState9(void);
		void incrementState(void);
		void decrementState(void);
		void loadLua(void);
		void takeScreenShot(void);
		void prepareScreenShot(void);
		void powerConsoleCB(void);
		void consoleHardReset(void);
		void consoleSoftReset(void);
		void consolePause(void);
		void toggleGameGenie(bool checked);
		void loadGameGenieROM(void);
		void loadMostRecentROM(void);
		void setRegionNTSC(void);
		void setRegionPAL(void);
		void setRegionDendy(void);
		void setRamInit0(void);
		void setRamInit1(void);
		void setRamInit2(void);
		void setRamInit3(void);
		void insertCoin(void);
		void fdsSwitchDisk(void);
		void fdsEjectDisk(void);
		void fdsLoadBiosFile(void);
		void emuSpeedUp(void);
		void emuSlowDown(void);
		void emuSlowestSpd(void);
		void emuNormalSpd(void);
		void emuFastestSpd(void);
		void emuCustomSpd(void);
		void emuSetFrameAdvDelay(void);
		void openPPUViewer(void);
		void openOAMViewer(void);
		void openNTViewer(void);
		void openGGEncoder(void);
		void openNesHeaderEditor(void);
		void openCheats(void);
		void openRamWatch(void);
		void openRamSearch(void);
		void openMovie(void);
		void stopMovie(void);
		void playMovieFromBeginning(void);
		void setCustomAutoFire(void);
		void incrSoundVolume(void);
		void decrSoundVolume(void);
		void toggleLagCounterDisplay(void);
		void toggleFrameAdvLagSkip(void);
		void toggleMovieBindSaveState(void);
		void toggleMovieFrameDisplay(void);
		void toggleMovieReadWrite(void);
		void toggleInputDisplay(void);
		void toggleTurboMode(void);
		void toggleBackground(void);
		void toggleForeground(void);
		void toggleFamKeyBrdEnable(void);
		void saveState0(void);
		void saveState1(void);
		void saveState2(void);
		void saveState3(void);
		void saveState4(void);
		void saveState5(void);
		void saveState6(void);
		void saveState7(void);
		void saveState8(void);
		void saveState9(void);
		void loadState0(void);
		void loadState1(void);
		void loadState2(void);
		void loadState3(void);
		void loadState4(void);
		void loadState5(void);
		void loadState6(void);
		void loadState7(void);
		void loadState8(void);
		void loadState9(void);
		void mainMenuOpen(void);
		void mainMenuClose(void);
		void warnAmbiguousShortcut( QShortcut*);
		void aviRecordStart(void);
		void aviRecordAsStart(void);
		void aviRecordStop(void);
		void aviAudioEnableChange(bool);
		void aviVideoFormatChanged(int idx);
		void setAviHudEnable(bool);
		void setAviMsgEnable(bool);
		void wavRecordStart(void);
		void wavRecordAsStart(void);
		void wavRecordStop(void);
		void winScreenChanged( QScreen *scr );
		void winActiveChanged(void);
		void emuFrameFinish(void);
		void videoBgColorChanged( QColor &c );
		void loadRomRequestCB( QString s );

};

extern consoleWin_t *consoleWindow;

#endif
