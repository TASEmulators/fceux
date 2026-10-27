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

	void setBreakpoints(const std::set<int> &bps);
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
	int execLine;
	bool execTopFrame;
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
	void buildVarView(void);

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

private:
	LuaSourceView *sourceView;
	QTreeWidget *stackView;
	QTreeWidget *varView;
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
