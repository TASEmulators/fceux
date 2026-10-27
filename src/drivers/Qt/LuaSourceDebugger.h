// LuaSourceDebugger.h
//
#pragma once

#ifdef _S9XLUA_H

#include <map>
#include <set>
#include <string>
#include <vector>

#include <QAction>
#include <QDialog>
#include <QLabel>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QColor>
#include <QRegularExpression>
#include <QSet>
#include <QSyntaxHighlighter>
#include <QTabBar>
#include <QTabWidget>
#include <QTextCharFormat>
#include <QSplitter>
#include <QTimer>
#include <QToolBar>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "../../types.h"
#include "../../fceulua.h"

class LuaSourceView;

struct LuaSyntaxTheme
{
	QColor background, text, gutterBg, gutterText;
	QColor keyword, builtin, string, number, comment, constant, op;
	QColor execTop, execOther;
	QColor execText;

	static LuaSyntaxTheme light(void);
	static LuaSyntaxTheme dark(void);
};

class LuaSyntaxHighlighter : public QSyntaxHighlighter
{
public:
	LuaSyntaxHighlighter(QTextDocument *doc);

	void setTheme(const LuaSyntaxTheme &t);

protected:
	void highlightBlock(const QString &text) override;

private:
	LuaSyntaxTheme theme;
};

// Line number / breakpoint margin of the source view
class LuaSourceGutter : public QWidget
{
public:
	LuaSourceGutter(LuaSourceView *view);

	QSize sizeHint(void) const override;

protected:
	void paintEvent(QPaintEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;

private:
	LuaSourceView *view;
};

class LuaSourceView : public QPlainTextEdit
{
	Q_OBJECT

public:
	LuaSourceView(QWidget *parent = nullptr);

	bool loadFile(const QString &path, const std::string &normalizedPath);
	const std::string &file(void) const { return filePath; }

	void setBreakpoints(const std::set<int> &bps, const std::set<int> &disabled = std::set<int>());
	void setExecLine(int line, bool topFrame);
	void setDarkTheme(bool dark);
	int  currentLine(void) const;

	int  gutterWidth(void) const;
	void gutterPaint(QPaintEvent *event);
	void gutterClick(int y);

signals:
	void gutterClicked(int line);
	void runToLineRequested(int line);

protected:
	void resizeEvent(QResizeEvent *event) override;
	void contextMenuEvent(QContextMenuEvent *event) override;

private slots:
	void updateGutterWidth(void);
	void updateGutter(const QRect &rect, int dy);

private:
	void updateHighlights(void);
	int  lineAtViewportY(int y) const;

	LuaSourceGutter *gutter;
	LuaSyntaxHighlighter *highlighter;
	LuaSyntaxTheme theme;
	std::string filePath;
	std::set<int> breakpoints;
	std::set<int> disabledBreakpoints;
	int execLine;
	bool execTopFrame;
};

class LuaDbgTabWidget;

// Tab bar that lets a tab be dragged out to be dropped on another tab widget
class LuaDbgTabBar : public QTabBar
{
	Q_OBJECT

public:
	LuaDbgTabBar(QWidget *parent = nullptr);

signals:
	void beginDragOut(int index);

protected:
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void contextMenuEvent(QContextMenuEvent *event) override;

private:
	bool dragPress;
	bool dragOut;
};

// A tab group in the debugger's panel grid at (column, row)
class LuaDbgTabWidget : public QTabWidget
{
	Q_OBJECT

public:
	LuaDbgTabWidget(int column, int row, QWidget *parent = nullptr);

	int column(void) const { return col; }
	int row(void) const { return rw; }

signals:
	void tabDropped(QWidget *page, LuaDbgTabWidget *target);
	void moveRequested(QWidget *page, int column, int row);

protected:
	void dragEnterEvent(QDragEnterEvent *event) override;
	void dropEvent(QDropEvent *event) override;
	void contextMenuEvent(QContextMenuEvent *event) override;

private:
	friend class LuaDbgTabBar;
	void buildContextMenu(const QPoint &globalPos);

	int col;
	int rw;
};

class LuaSourceDebuggerDialog_t : public QDialog
{
	Q_OBJECT

public:
	LuaSourceDebuggerDialog_t(QWidget *parent = nullptr);
	~LuaSourceDebuggerDialog_t(void);

	bool openScript(const QString &path);

protected:
	void closeEvent(QCloseEvent *event) override;

private:
	enum VarRoot { ROOT_WATCH = 0, ROOT_LOCALS, ROOT_UPVALUES, ROOT_GLOBALS };

	enum { ITEM_VAR_ROLE = Qt::UserRole, ITEM_ROOT_ROLE = Qt::UserRole + 1 };

	QMenuBar *buildMenuBar(void);
	QToolBar *buildToolBar(void);
	void buildStackView(void);
	QTreeWidget *buildVarTree(void);
	void buildVarView(void);
	void buildBreakpointView(void);
	void setPageTitle(QWidget *page, const QString &title);
	LuaDbgTabWidget *tabWidgetOf(QWidget *page) const;
	void addPage(LuaDbgTabWidget *tw, QWidget *page);
	void moveTab(QWidget *page, int column, int row);
	void setLayoutOption(int opt);
	void updateTabVisibility(void);
	void loadDisplayViews(void);
	void saveDisplayViews(void);
	void refreshBreakpointList(void);
	void refreshWatches(void);

	void updateActions(bool scriptRunning, bool paused);
	void refreshPausedState(void);
	void clearPausedState(void);
	void showFrame(int level);
	void refreshVariables(void);
	void populateRoot(QTreeWidgetItem *root, int rootKind);
	void populateChildren(QTreeWidgetItem *item);
	QTreeWidgetItem *addVarItem(QTreeWidgetItem *parent, const LuaDbgVar &var, bool editable);
	QString itemPath(QTreeWidgetItem *item) const;
	QString itemExpression(QTreeWidgetItem *item) const;
	void restoreExpansion(QTreeWidgetItem *item);
	void syncBreakpoints(void);
	void saveWatches(void);

private slots:
	void updatePeriodic(void);
	void openScriptDialog(void);
	void reloadScript(void);
	void runContinueCB(void);
	void breakCB(void);
	void stopCB(void);
	void stepIntoCB(void);
	void stepOverCB(void);
	void stepOutCB(void);
	void runToCursorCB(void);
	void runToLineCB(int line);
	void toggleBreakpointCB(void);
	void toggleBreakpointEnabledCB(void);
	void bpItemChanged(QTreeWidgetItem *item, int column);
	void gutterClickedCB(int line);
	void clearBreakpointsCB(void);
	void breakOnStartCB(bool checked);
	void themeCB(bool dark);
	void addWatchCB(void);
	void clearWatchesCB(void);
	void frameChanged(void);
	void varItemExpanded(QTreeWidgetItem *item);
	void varItemCollapsed(QTreeWidgetItem *item);
	void varItemChanged(QTreeWidgetItem *item, int column);
	void varItemDoubleClicked(QTreeWidgetItem *item, int column);
	void varContextMenu(const QPoint &pos);
	void bpItemActivated(QTreeWidgetItem *item, int column);
	void bpContextMenu(const QPoint &pos);
	void removeSelectedBreakpoints(void);
	void applyBreakpointEnable(const std::string &path, int line, bool enable);
	void syncSourceMarkers(void);
	void setSelectedBreakpointsEnabled(bool enable);
	void setAllBreakpointsEnabled(bool enable);

private:
	LuaSourceView *sourceView;
	QTreeWidget *stackView;
	QTreeWidget *varView;
	QTreeWidget *watchView;
	QTreeWidget *bpView;
	QSplitter *hsplitter;
	QSplitter *vsplitter[2];
	LuaDbgTabWidget *tabView[2][3];
	QWidget *pages[4];
	QString pageTitles[4];
	int layoutOpt;
	QLabel *statusLabel;
	QTimer *periodicTimer;

	QAction *openAct;
	QAction *reloadAct;
	QAction *runAct;
	QAction *breakAct;
	QAction *stopAct;
	QAction *stepIntoAct;
	QAction *stepOverAct;
	QAction *stepOutAct;
	QAction *runToCursorAct;
	QAction *toggleBpAct;
	QAction *breakOnStartAct;
	QAction *lightThemeAct;
	QAction *darkThemeAct;

	std::map< std::string, std::set<int> > breakpointMap;
	std::map< std::string, std::set<int> > disabledMap;
	bool bpRefreshing = false;
	std::vector<LuaDbgFrame> frames;
	std::vector<LuaDbgVar> varStore;
	QSet<QString> expandedPaths;
	QStringList watches;

	unsigned int lastSerial;
	bool wasPaused;
	bool updatingVars;
};

// Opens (or raises) the debugger window; optionally loads the given script into the source view.
void openLuaSourceDebugger(QWidget *parent, const QString &scriptPath = QString());

#endif
