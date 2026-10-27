// LuaSourceDebugger.cpp
//
#ifdef _S9XLUA_H

#include <stdio.h>
#include <string.h>

#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QCollator>
#include <QContextMenuEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QHeaderView>
#include <QInputDialog>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMenu>
#include <QMimeData>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSettings>
#include <QTextBlock>
#include <QThread>
#include <QToolTip>

#include "Qt/LuaSourceDebugger.h"
#include "Qt/LuaControl.h"
#include "Qt/fceuWrapper.h"
#include "Qt/nes_shm.h"

extern int luaRunning;

static LuaSourceDebuggerDialog_t *debuggerWin = nullptr;

//----------------------------------------------------------------------------
// Blocks the thread that hit a breakpoint until the GUI requests a resume.
//----------------------------------------------------------------------------
static void luaDebuggerWaitForResume(void)
{
	if (QThread::currentThread() == QCoreApplication::instance()->thread())
	{
		// The script was started from the GUI thread (first frame of a freshly loaded script),
		// so keep the GUI alive with a nested event loop.
		while (!FCEU_LuaDbgResumeRequested())
		{
			QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
			QThread::msleep(5);
		}
	}
	else
	{
		// Emulation thread: release the emulator lock so the GUI can operate.
		FCEU_WRAPPER_UNLOCK();

		while (!FCEU_LuaDbgResumeRequested() && nes_shm->runEmulator)
		{
			QThread::msleep(10);
		}
		FCEU_WRAPPER_LOCK();
	}
}
//----------------------------------------------------------------------------
// Syntax highlighting
//----------------------------------------------------------------------------
LuaSyntaxTheme LuaSyntaxTheme::light(void)
{
	LuaSyntaxTheme t;

	t.background = QColor(255, 255, 255);
	t.text       = QColor(0, 0, 0);
	t.gutterBg   = QColor(240, 240, 240);
	t.gutterText = QColor(130, 130, 130);
	t.keyword    = QColor(0, 0, 200);
	t.builtin    = QColor(0, 110, 130);
	t.string     = QColor(163, 21, 21);
	t.number     = QColor(9, 134, 88);
	t.comment    = QColor(0, 128, 0);
	t.constant   = QColor(150, 0, 150);
	t.op         = QColor(90, 90, 90);
	t.execTop    = QColor(255, 245, 160);
	t.execOther  = QColor(190, 235, 190);
	t.execText   = QColor(0, 0, 0);
	return t;
}

LuaSyntaxTheme LuaSyntaxTheme::dark(void)
{
	LuaSyntaxTheme t;

	t.background = QColor(30, 30, 30);
	t.text       = QColor(212, 212, 212);
	t.gutterBg   = QColor(37, 37, 38);
	t.gutterText = QColor(133, 133, 133);
	t.keyword    = QColor(86, 156, 214);
	t.builtin    = QColor(78, 201, 176);
	t.string     = QColor(206, 145, 120);
	t.number     = QColor(181, 206, 168);
	t.comment    = QColor(106, 153, 85);
	t.constant   = QColor(197, 134, 192);
	t.op         = QColor(170, 170, 170);
	t.execTop    = QColor(90, 80, 20);
	t.execOther  = QColor(35, 80, 45);
	t.execText   = QColor(240, 240, 240);
	return t;
}

LuaSyntaxHighlighter::LuaSyntaxHighlighter(QTextDocument *doc)
	: QSyntaxHighlighter(doc), theme(LuaSyntaxTheme::light())
{
}

void LuaSyntaxHighlighter::setTheme(const LuaSyntaxTheme &t)
{
	theme = t;
	rehighlight();
}

// Block state: 0 = normal, otherwise 1 + 2 * level + (isComment ? 1 : 0) for an open long bracket.
void LuaSyntaxHighlighter::highlightBlock(const QString &text)
{
	static const QSet<QString> keywords = {
		"and", "break", "do", "else", "elseif", "end", "for", "function", "if", "in",
		"local", "not", "or", "repeat", "return", "then", "until", "while" };
	static const QSet<QString> constants = { "true", "false", "nil" };
	static const QSet<QString> builtins = {
		"assert", "collectgarbage", "dofile", "error", "getfenv", "getmetatable", "ipairs", "load",
		"loadfile", "loadstring", "next", "pairs", "pcall", "print", "rawequal", "rawget", "rawset",
		"require", "select", "setfenv", "setmetatable", "tonumber", "tostring", "type", "unpack",
		"xpcall", "_G", "_VERSION", "string", "table", "math", "io", "os", "debug", "coroutine",
		"package", "emu", "memory", "rom", "ppu", "gui", "input", "joypad", "savestate", "movie",
		"sound", "zapper", "bit", "taseditor", "FCEU", "SNES", "AND", "OR", "XOR", "SHIFT", "BIT" };

	auto fmt = [](const QColor &c, bool italic = false, bool bold = false) {
		QTextCharFormat f;

		f.setForeground(c);
		f.setFontItalic(italic);
		if (bold) f.setFontWeight(QFont::Bold);
		return f;
	};
	const QTextCharFormat fText = fmt(theme.text), fKeyword = fmt(theme.keyword, false, true),
	                      fBuiltin = fmt(theme.builtin), fString = fmt(theme.string),
	                      fNumber = fmt(theme.number), fComment = fmt(theme.comment, true),
	                      fConst = fmt(theme.constant, false, true), fOp = fmt(theme.op);

	// Finds the end of a long bracket of the given level starting at 'from'; returns -1 if not on this line.
	auto longEnd = [&text](int from, int level) {
		const QString close = QStringLiteral("]") + QString(level, QChar('=')) + QStringLiteral("]");
		const int p = text.indexOf(close, from);

		return (p < 0) ? -1 : p + close.length();
	};
	// Detects "[" "="* "[" at position i; returns level or -1.
	auto longOpen = [&text](int i, int &len) {
		if (i >= text.length() || text[i] != QChar('['))
		{
			return -1;
		}
		int j = i + 1;

		while (j < text.length() && text[j] == QChar('='))
		{
			j++;
		}
		if (j < text.length() && text[j] == QChar('['))
		{
			len = j - i + 1;
			return j - i - 1;
		}
		return -1;
	};

	setFormat(0, text.length(), fText);

	int i = 0;
	const int prev = previousBlockState();

	setCurrentBlockState(0);

	if (prev > 0)
	{
		const int level = (prev - 1) / 2;
		const bool isComment = ((prev - 1) % 2) == 1;
		const int end = longEnd(0, level);

		if (end < 0)
		{
			setFormat(0, text.length(), isComment ? fComment : fString);
			setCurrentBlockState(prev);
			return;
		}
		setFormat(0, end, isComment ? fComment : fString);
		i = end;
	}

	while (i < text.length())
	{
		const QChar ch = text[i];
		int len = 0;

		if (ch == QChar('-') && i + 1 < text.length() && text[i + 1] == QChar('-'))
		{
			const int level = longOpen(i + 2, len);

			if (level >= 0)
			{
				const int end = longEnd(i + 2 + len, level);

				if (end < 0)
				{
					setFormat(i, text.length() - i, fComment);
					setCurrentBlockState(1 + 2 * level + 1);
					return;
				}
				setFormat(i, end - i, fComment);
				i = end;
				continue;
			}
			setFormat(i, text.length() - i, fComment);
			return;
		}
		if (ch == QChar('"') || ch == QChar('\''))
		{
			int j = i + 1;

			while (j < text.length() && text[j] != ch)
			{
				j += (text[j] == QChar('\\')) ? 2 : 1;
			}
			j = qMin(j + 1, (int)text.length());
			setFormat(i, j - i, fString);
			i = j;
			continue;
		}
		const int level = longOpen(i, len);

		if (level >= 0)
		{
			const int end = longEnd(i + len, level);

			if (end < 0)
			{
				setFormat(i, text.length() - i, fString);
				setCurrentBlockState(1 + 2 * level);
				return;
			}
			setFormat(i, end - i, fString);
			i = end;
			continue;
		}
		if (ch.isDigit() || (ch == QChar('.') && i + 1 < text.length() && text[i + 1].isDigit()))
		{
			int j = i + 1;

			if (ch == QChar('0') && j < text.length() && (text[j] == QChar('x') || text[j] == QChar('X')))
			{
				j++;
				while (j < text.length() && (text[j].isDigit() || QString("abcdefABCDEF").contains(text[j])))
				{
					j++;
				}
			}
			else
			{
				while (j < text.length())
				{
					if (text[j].isDigit() || text[j] == QChar('.'))
					{
						j++;
					}
					else if ((text[j] == QChar('e') || text[j] == QChar('E')) && j + 1 < text.length())
					{
						j++;
						if (text[j] == QChar('+') || text[j] == QChar('-'))
						{
							j++;
						}
					}
					else
					{
						break;
					}
				}
			}
			setFormat(i, j - i, fNumber);
			i = j;
			continue;
		}
		if (ch.isLetter() || ch == QChar('_'))
		{
			int j = i + 1;

			while (j < text.length() && (text[j].isLetterOrNumber() || text[j] == QChar('_')))
			{
				j++;
			}
			const QString word = text.mid(i, j - i);
			const bool afterDot = (i > 0 && text[i - 1] == QChar('.'));

			if (!afterDot && keywords.contains(word)) setFormat(i, j - i, fKeyword);
			else if (!afterDot && constants.contains(word)) setFormat(i, j - i, fConst);
			else if (!afterDot && builtins.contains(word)) setFormat(i, j - i, fBuiltin);
			i = j;
			continue;
		}
		if (!ch.isSpace() && !ch.isLetterOrNumber())
		{
			setFormat(i, 1, fOp);
		}
		i++;
	}
}
//----------------------------------------------------------------------------
// LuaSourceGutter
//----------------------------------------------------------------------------
LuaSourceGutter::LuaSourceGutter(LuaSourceView *v)
	: QWidget(v), view(v)
{
}

QSize LuaSourceGutter::sizeHint(void) const
{
	return QSize(view->gutterWidth(), 0);
}

void LuaSourceGutter::paintEvent(QPaintEvent *event)
{
	view->gutterPaint(event);
}

void LuaSourceGutter::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		view->gutterClick(event->pos().y());
	}
}
//----------------------------------------------------------------------------
// LuaSourceView
//----------------------------------------------------------------------------
LuaSourceView::LuaSourceView(QWidget *parent)
	: QPlainTextEdit(parent), execLine(-1), execTopFrame(true)
{
	QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);

	setFont(font);
	setReadOnly(true);
	setLineWrapMode(QPlainTextEdit::NoWrap);

#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
	setTabStopDistance(4 * QFontMetrics(font).horizontalAdvance(' '));
#endif

	gutter = new LuaSourceGutter(this);
	highlighter = new LuaSyntaxHighlighter(document());
	setDarkTheme(false);

	connect(this, SIGNAL(blockCountChanged(int)), this, SLOT(updateGutterWidth(void)));
	connect(this, SIGNAL(updateRequest(const QRect &, int)), this, SLOT(updateGutter(const QRect &, int)));

	updateGutterWidth();
}

int LuaSourceView::gutterWidth(void) const
{
	int digits = 1;
	int max = qMax(1, blockCount());

	while (max >= 10)
	{
		max /= 10;
		digits++;
	}
	return 20 + 6 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

void LuaSourceView::updateGutterWidth(void)
{
	setViewportMargins(gutterWidth(), 0, 0, 0);
	gutter->updateGeometry();
}

void LuaSourceView::updateGutter(const QRect &rect, int dy)
{
	if (dy)
	{
		gutter->scroll(0, dy);
	}
	else
	{
		gutter->update(0, rect.y(), gutter->width(), rect.height());
	}
	if (rect.contains(viewport()->rect()))
	{
		updateGutterWidth();
	}
}

void LuaSourceView::resizeEvent(QResizeEvent *event)
{
	QPlainTextEdit::resizeEvent(event);

	const QRect cr = contentsRect();

	gutter->setGeometry(QRect(cr.left(), cr.top(), gutterWidth(), cr.height()));
}

void LuaSourceView::gutterPaint(QPaintEvent *event)
{
	QPainter painter(gutter);

	painter.fillRect(event->rect(), theme.gutterBg);

	QTextBlock block = firstVisibleBlock();
	int blockNumber = block.blockNumber();
	int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
	int bottom = top + qRound(blockBoundingRect(block).height());

	while (block.isValid() && top <= event->rect().bottom())
	{
		if (block.isVisible() && bottom >= event->rect().top())
		{
			const int line = blockNumber + 1;
			const int h = fontMetrics().height();
			const QRect marker(3, top + (h - 12) / 2, 12, 12);

			if (breakpoints.count(line))
			{
				painter.setRenderHint(QPainter::Antialiasing, true);
				if (disabledBreakpoints.count(line))
				{
					painter.setPen(QPen(QColor(200, 30, 30), 1.5));
					painter.setBrush(Qt::NoBrush);
					painter.drawEllipse(marker.adjusted(1, 1, -1, -1));
				}
				else
				{
					painter.setPen(Qt::NoPen);
					painter.setBrush(QColor(200, 30, 30));
					painter.drawEllipse(marker);
				}
			}
			if (line == execLine)
			{
				QPolygon arrow;

				arrow << QPoint(marker.left(), marker.top() + 2) << QPoint(marker.center().x(), marker.top() + 2)
				      << QPoint(marker.right(), marker.center().y()) << QPoint(marker.center().x(), marker.bottom() - 2)
				      << QPoint(marker.left(), marker.bottom() - 2);
				painter.setRenderHint(QPainter::Antialiasing, true);
				painter.setPen(Qt::black);
				painter.setBrush(execTopFrame ? QColor(255, 220, 0) : QColor(60, 190, 60));
				painter.drawPolygon(arrow);
			}
			painter.setRenderHint(QPainter::Antialiasing, false);
			painter.setPen(theme.gutterText);
			painter.drawText(0, top, gutter->width() - 4, h, Qt::AlignRight, QString::number(line));
		}
		block = block.next();
		top = bottom;
		bottom = top + qRound(blockBoundingRect(block).height());
		blockNumber++;
	}
}

int LuaSourceView::lineAtViewportY(int y) const
{
	QTextBlock block = firstVisibleBlock();
	int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());

	while (block.isValid())
	{
		const int bottom = top + qRound(blockBoundingRect(block).height());

		if (y >= top && y < bottom)
		{
			return block.blockNumber() + 1;
		}
		if (top > y)
		{
			break;
		}
		top = bottom;
		block = block.next();
	}
	return -1;
}

void LuaSourceView::gutterClick(int y)
{
	const int line = lineAtViewportY(y);

	if (line > 0 && !filePath.empty())
	{
		emit gutterClicked(line);
	}
}

void LuaSourceView::contextMenuEvent(QContextMenuEvent *event)
{
	QMenu *menu = createStandardContextMenu();
	const int line = lineAtViewportY(event->pos().y());

	if (line > 0 && !filePath.empty())
	{
		QAction *first = menu->actions().isEmpty() ? nullptr : menu->actions().first();
		QAction *bpAct = new QAction(tr("Toggle Breakpoint"), menu);
		QAction *runToAct = new QAction(tr("Run to Line"), menu);

		connect(bpAct, &QAction::triggered, this, [this, line]() { emit gutterClicked(line); });
		connect(runToAct, &QAction::triggered, this, [this, line]() { emit runToLineRequested(line); });

		menu->insertAction(first, bpAct);
		menu->insertAction(first, runToAct);
		menu->insertSeparator(first);
	}
	menu->exec(event->globalPos());
	delete menu;
}

bool LuaSourceView::loadFile(const QString &path, const std::string &normalizedPath)
{
	QFile f(path);

	if (!f.open(QIODevice::ReadOnly))
	{
		return false;
	}
	const QByteArray data = f.readAll();

	filePath = normalizedPath;
	execLine = -1;
	setPlainText(QString::fromUtf8(data));
	updateHighlights();
	gutter->update();
	return true;
}

void LuaSourceView::setDarkTheme(bool dark)
{
	theme = dark ? LuaSyntaxTheme::dark() : LuaSyntaxTheme::light();

	setStyleSheet(QStringLiteral("QPlainTextEdit { background-color: %1; color: %2; }")
	              .arg(theme.background.name(), theme.text.name()));
	highlighter->setTheme(theme);
	updateHighlights();
	gutter->update();
}

bool LuaSourceView::event(QEvent *e)
{
	if (e->type() == QEvent::ToolTip)
	{
		QHelpEvent *he = static_cast<QHelpEvent *>(e);
		QString expr;

		if (hoverProvider)
		{
			const QPoint vp = viewport()->mapFrom(this, he->pos());
			QTextCursor sel = textCursor();
			QTextCursor at = cursorForPosition(vp);

			if (sel.hasSelection() && at.position() >= sel.selectionStart() && at.position() <= sel.selectionEnd())
			{
				expr = sel.selectedText().trimmed();
			}
			else
			{
				const QString text = at.block().text();
				// cursorForPosition snaps to the nearest boundary; pick the character actually under the mouse
				const int col = at.positionInBlock() - ((vp.x() < cursorRect(at).x()) ? 1 : 0);
				int s = col, t = col;

				auto isId = [&](int i) { return i >= 0 && i < text.length() && (text[i].isLetterOrNumber() || text[i] == '_'); };

				while (isId(s - 1)) s--;
				while (isId(t)) t++;

				// Extend backwards over dotted/colon-free field chains such as a.b.c
				while (s >= 2 && text[s - 1] == '.' && isId(s - 2))
				{
					s -= 1;
					while (isId(s - 1)) s--;
				}
				expr = text.mid(s, t - s);
			}
			static const QRegularExpression safe(QStringLiteral("^[A-Za-z_][A-Za-z0-9_]*(\\.[A-Za-z_][A-Za-z0-9_]*)*$"));

			if (!safe.match(expr).hasMatch())
			{
				expr.clear();
			}
		}
		const QString tip = expr.isEmpty() ? QString() : hoverProvider(expr);

		if (tip.isEmpty())
		{
			QToolTip::hideText();
			e->ignore();
		}
		else
		{
			QToolTip::showText(he->globalPos(), tip, this);
		}
		return true;
	}
	return QPlainTextEdit::event(e);
}
//----------------------------------------------------------------------------
void LuaSourceView::setBreakpoints(const std::set<int> &bps, const std::set<int> &disabled)
{
	breakpoints = bps;
	disabledBreakpoints = disabled;
	gutter->update();
}

int LuaSourceView::currentLine(void) const
{
	return textCursor().blockNumber() + 1;
}

void LuaSourceView::setExecLine(int line, bool topFrame)
{
	execLine = line;
	execTopFrame = topFrame;

	if (line > 0)
	{
		QTextCursor cursor(document()->findBlockByNumber(line - 1));

		setTextCursor(cursor);
		centerCursor();
	}
	updateHighlights();
	gutter->update();
}

void LuaSourceView::updateHighlights(void)
{
	QList<QTextEdit::ExtraSelection> selections;

	if (execLine > 0)
	{
		QTextEdit::ExtraSelection sel;

		sel.format.setBackground(execTopFrame ? theme.execTop : theme.execOther);
		sel.format.setProperty(QTextFormat::FullWidthSelection, true);
		sel.cursor = QTextCursor(document()->findBlockByNumber(execLine - 1));
		sel.cursor.clearSelection();
		selections.append(sel);
	}
	setExtraSelections(selections);
}
//----------------------------------------------------------------------------
// LuaSourceDebuggerDialog_t
//----------------------------------------------------------------------------
LuaSourceDebuggerDialog_t::LuaSourceDebuggerDialog_t(QWidget *parent)
	: QDialog(parent, Qt::Window), outputSerial(0xFFFFFFFFu), lastSerial(0), wasPaused(false), updatingVars(false)
{
	QSettings settings;
	QMenuBar *menuBar;
	QToolBar *toolBar;
	QVBoxLayout *mainLayout;

	setWindowTitle(tr("Lua Source Debugger"));
	resize(1024, 640);

	sourceView = new LuaSourceView();
	statusLabel = new QLabel();

	menuBar = buildMenuBar();
	toolBar = buildToolBar();
	buildStackView();
	buildVarView();
	buildBreakpointView();
	buildOutputView();

	stackView->setObjectName("luaDbgStack");
	varView->setObjectName("luaDbgVariables");
	watchView->setObjectName("luaDbgWatch");
	bpView->setObjectName("luaDbgBreakpoints");
	outputView->setObjectName("luaDbgOutput");

	pages[0] = stackView;  pageTitles[0] = tr("Stack");
	pages[1] = varView;    pageTitles[1] = tr("Variables");
	pages[2] = watchView;  pageTitles[2] = tr("Watch");
	pages[3] = bpView;     pageTitles[3] = tr("Breakpoints");
	pages[4] = outputView; pageTitles[4] = tr("Output");

	hsplitter = new QSplitter(Qt::Horizontal);
	hsplitter->setOpaqueResize(true);
	hsplitter->addWidget(sourceView);

	for (int i = 0; i < 2; i++)
	{
		vsplitter[i] = new QSplitter(Qt::Vertical);
		vsplitter[i]->setOpaqueResize(true);

		for (int j = 0; j < 3; j++)
		{
			tabView[i][j] = new LuaDbgTabWidget(i, j);
			tabView[i][j]->setObjectName(QStringLiteral("luaDbgTabView%1%2").arg(i + 1).arg(j + 1));
			vsplitter[i]->addWidget(tabView[i][j]);

			connect(tabView[i][j], &LuaDbgTabWidget::tabDropped, this, [this](QWidget *page, LuaDbgTabWidget *target) {
				moveTab(page, target->column(), target->row());
			});
			connect(tabView[i][j], &LuaDbgTabWidget::moveRequested, this, &LuaSourceDebuggerDialog_t::moveTab);
		}
		hsplitter->addWidget(vsplitter[i]);
	}
	hsplitter->setStretchFactor(0, 3);
	hsplitter->setStretchFactor(1, 1);
	hsplitter->setStretchFactor(2, 1);

	mainLayout = new QVBoxLayout();
	mainLayout->setMenuBar(menuBar);
	mainLayout->addWidget(toolBar);
	mainLayout->addWidget(hsplitter, 1);
	mainLayout->addWidget(statusLabel);
	setLayout(mainLayout);

	watches = settings.value("luaDebugger/watches").toStringList();
	expandedPaths.insert(QStringLiteral("Locals"));
	expandedPaths.insert(QStringLiteral("Watch"));

	sourceView->hoverProvider = [this](const QString &expr) -> QString {
		static const QSet<QString> keywords = { "and", "break", "do", "else", "elseif", "end", "false", "for",
			"function", "if", "in", "local", "nil", "not", "or", "repeat", "return", "then", "true", "until", "while" };

		if (!FCEU_LuaDbgIsPaused() || keywords.contains(expr))
		{
			return QString();
		}
		QTreeWidgetItem *cur = stackView->currentItem();
		const int level = cur ? cur->data(0, Qt::UserRole).toInt() : 0;
		LuaDbgVar var;

		if (!FCEU_LuaDbgEvaluate(level, expr.toLocal8Bit().constData(), var))
		{
			return QString();
		}
		QString val = QString::fromStdString(var.value);

		if (val.length() > 300)
		{
			val = val.left(300) + QStringLiteral("...");
		}
		return QStringLiteral("%1 (%2) = %3").arg(expr.toHtmlEscaped(), QString::fromStdString(var.type).toHtmlEscaped(), val.toHtmlEscaped());
	};

	connect(sourceView, &LuaSourceView::gutterClicked, this, &LuaSourceDebuggerDialog_t::gutterClickedCB);
	connect(sourceView, &LuaSourceView::runToLineRequested, this, &LuaSourceDebuggerDialog_t::runToLineCB);

	loadDisplayViews();

	debuggerWin = this;

	{
		LuaDbgCallbacks cb;
		cb.waitForResume = luaDebuggerWaitForResume;

		FCEU_WRAPPER_LOCK();
		FCEU_LuaDbgAttach(&cb);
		FCEU_LuaDbgSetBreakOnStart(breakOnStartAct->isChecked());
		FCEU_WRAPPER_UNLOCK();
	}

	// Only react to scripts started after the dialog opens
	FCEU_WRAPPER_LOCK();
	lastScriptName = FCEU_LuaDbgGetScriptName();
	FCEU_WRAPPER_UNLOCK();
	prevRunning = (luaRunning != 0);

	periodicTimer = new QTimer(this);
	connect(periodicTimer, &QTimer::timeout, this, &LuaSourceDebuggerDialog_t::updatePeriodic);
	periodicTimer->start(100);

	updatePeriodic();
}
//----------------------------------------------------------------------------
// Relocatable tab panels
//----------------------------------------------------------------------------
LuaDbgTabBar::LuaDbgTabBar(QWidget *parent)
	: QTabBar(parent), dragPress(false), dragOut(false)
{
	setMouseTracking(true);
	setAcceptDrops(true);
}

void LuaDbgTabBar::mousePressEvent(QMouseEvent *event)
{
	QTabBar::mousePressEvent(event);

	if (event->button() == Qt::LeftButton && currentIndex() >= 0)
	{
		dragPress = true;
		dragOut = false;
	}
}

void LuaDbgTabBar::mouseReleaseEvent(QMouseEvent *event)
{
	QTabBar::mouseReleaseEvent(event);
	dragPress = false;
	dragOut = false;
}

void LuaDbgTabBar::mouseMoveEvent(QMouseEvent *event)
{
	QTabBar::mouseMoveEvent(event);

	if (dragPress && !dragOut && event->buttons() && !contentsRect().contains(event->pos()))
	{
		dragOut = true;
		emit beginDragOut(currentIndex());

		// QDrag::exec swallows the release event, so synthesize it.
		QMouseEvent *e = new QMouseEvent(QEvent::MouseButtonRelease, mapFromGlobal(QCursor::pos()), QCursor::pos(),
		                                 Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
		QApplication::postEvent(this, e);
	}
}

void LuaDbgTabBar::contextMenuEvent(QContextMenuEvent *event)
{
	const int idx = tabAt(event->pos());

	if (idx >= 0 && idx != currentIndex())
	{
		setCurrentIndex(idx);
	}
	LuaDbgTabWidget *p = qobject_cast<LuaDbgTabWidget *>(parent());

	if (p)
	{
		p->buildContextMenu(event->globalPos());
	}
}
//----------------------------------------------------------------------------
LuaDbgTabWidget::LuaDbgTabWidget(int column, int row, QWidget *parent)
	: QTabWidget(parent), col(column), rw(row)
{
	LuaDbgTabBar *bar = new LuaDbgTabBar(this);

	setTabBar(bar);
	bar->setAcceptDrops(true);
	bar->setChangeCurrentOnDrag(true);
	setMouseTracking(true);
	setAcceptDrops(true);
	setMovable(true);
	setUsesScrollButtons(true);

	connect(bar, &LuaDbgTabBar::beginDragOut, this, [this, bar](int index) {
		QWidget *page = widget(index);

		if (page == nullptr)
		{
			return;
		}
		QPixmap pixmap(bar->tabRect(index).size());
		pixmap.fill(palette().color(QPalette::Window));

		QPainter painter(&pixmap);
		painter.drawText(pixmap.rect(), Qt::AlignCenter, tabText(index));
		painter.drawRect(pixmap.rect().adjusted(0, 0, -1, -1));
		painter.end();

		QDrag *drag = new QDrag(bar);

		drag->setMimeData(new QMimeData);
		drag->setPixmap(pixmap);
		drag->setHotSpot(QPoint(10, 0));
		drag->exec(Qt::MoveAction);
	});
}

void LuaDbgTabWidget::dragEnterEvent(QDragEnterEvent *event)
{
	if (qobject_cast<LuaDbgTabBar *>(event->source()) != nullptr)
	{
		event->acceptProposedAction();
	}
}

void LuaDbgTabWidget::dropEvent(QDropEvent *event)
{
	LuaDbgTabBar *srcBar = qobject_cast<LuaDbgTabBar *>(event->source());

	if (srcBar == nullptr)
	{
		return;
	}
	LuaDbgTabWidget *src = qobject_cast<LuaDbgTabWidget *>(srcBar->parent());

	if (src != nullptr && src != this && srcBar->currentIndex() >= 0)
	{
		event->acceptProposedAction();
		emit tabDropped(src->widget(srcBar->currentIndex()), this);
	}
}

void LuaDbgTabWidget::contextMenuEvent(QContextMenuEvent *event)
{
	buildContextMenu(event->globalPos());
}

void LuaDbgTabWidget::buildContextMenu(const QPoint &globalPos)
{
	QMenu menu(this);
	QMenu *moveMenu = menu.addMenu(tr("Move Tab To..."));
	QWidget *page = widget(currentIndex());
	const char *rowNames[] = { "Upper", "Middle", "Lower" };
	const char *colNames[] = { "Left", "Right" };

	if (page == nullptr)
	{
		return;
	}
	for (int j = 0; j < 3; j++)
	{
		QMenu *sub = moveMenu->addMenu(tr(rowNames[j]));

		for (int i = 0; i < 2; i++)
		{
			QAction *act = sub->addAction(tr(colNames[i]));

			connect(act, &QAction::triggered, this, [this, page, i, j]() { emit moveRequested(page, i, j); });
		}
	}
	menu.exec(globalPos);
}
//----------------------------------------------------------------------------
LuaDbgTabWidget *LuaSourceDebuggerDialog_t::tabWidgetOf(QWidget *page) const
{
	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			if (tabView[i][j]->indexOf(page) >= 0)
			{
				return tabView[i][j];
			}
		}
	}
	return nullptr;
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::setPageTitle(QWidget *page, const QString &title)
{
	LuaDbgTabWidget *tw = tabWidgetOf(page);

	for (int k = 0; k < 5; k++)
	{
		if (pages[k] == page)
		{
			pageTitles[k] = title;
		}
	}
	if (tw)
	{
		tw->setTabText(tw->indexOf(page), title);
	}
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::addPage(LuaDbgTabWidget *tw, QWidget *page)
{
	for (int k = 0; k < 5; k++)
	{
		if (pages[k] == page)
		{
			tw->addTab(page, pageTitles[k]);
		}
	}
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::moveTab(QWidget *page, int column, int row)
{
	LuaDbgTabWidget *src = tabWidgetOf(page);

	if (src == nullptr || column < 0 || column > 1 || row < 0 || row > 2)
	{
		return;
	}
	if (src != tabView[column][row])
	{
		src->removeTab(src->indexOf(page));
		addPage(tabView[column][row], page);
		tabView[column][row]->setCurrentWidget(page);
	}
	updateTabVisibility();
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::updateTabVisibility(void)
{
	for (int i = 0; i < 2; i++)
	{
		bool any = false;
		const bool colHidden = !vsplitter[i]->isVisible();

		for (int j = 0; j < 3; j++)
		{
			const bool has = (tabView[i][j]->count() > 0);

			any = any || has;
			tabView[i][j]->setVisible(has);
		}
		vsplitter[i]->setVisible(any);

		if (any)
		{
			QList<int> s = vsplitter[i]->sizes();
			int total = 0, visible = 0;

			for (int j = 0; j < 3; j++)
			{
				total += s[j];
				visible += (tabView[i][j]->count() > 0) ? 1 : 0;
			}
			for (int j = 0; j < 3; j++)
			{
				if (tabView[i][j]->count() > 0 && s[j] == 0)
				{
					// A group that was just revealed has no space yet; share evenly.
					total = qMax(total, qMax(vsplitter[i]->height(), 300));

					for (int k = 0; k < 3; k++)
					{
						s[k] = (tabView[i][k]->count() > 0) ? total / visible : 0;
					}
					vsplitter[i]->setSizes(s);
					break;
				}
			}
		}
		if (any && colHidden)
		{
			QList<int> hs = hsplitter->sizes();

			if (hs[i + 1] == 0)
			{
				hs[0] = qMax(1, hsplitter->width() * 3 / 5);
				hs[1] = hs[2] = qMax(1, (hsplitter->width() - hs[0]) / 2);
				hsplitter->setSizes(hs);
			}
		}
	}
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::setLayoutOption(int opt)
{
	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			while (tabView[i][j]->count() > 0)
			{
				tabView[i][j]->removeTab(0);
			}
		}
	}
	layoutOpt = opt;

	switch (opt)
	{
		default:
		case 1: // Compact: one group
			for (int k = 0; k < 5; k++) addPage(tabView[0][0], pages[k]);
		break;
		case 2: // Compact Split: one column, two groups
			addPage(tabView[0][0], stackView);
			addPage(tabView[0][0], bpView);
			addPage(tabView[0][1], varView);
			addPage(tabView[0][1], watchView);
			addPage(tabView[0][1], outputView);
		break;
		case 3: // Wide: two columns
			addPage(tabView[0][0], stackView);
			addPage(tabView[0][0], bpView);
			addPage(tabView[1][0], varView);
			addPage(tabView[1][0], watchView);
			addPage(tabView[1][0], outputView);
		break;
		case 4: // Wide Quad: two columns, two groups each
			addPage(tabView[0][0], stackView);
			addPage(tabView[0][1], bpView);
			addPage(tabView[0][1], outputView);
			addPage(tabView[1][0], varView);
			addPage(tabView[1][1], watchView);
		break;
	}

	// Force an even re-share of space
	for (int i = 0; i < 2; i++)
	{
		vsplitter[i]->setSizes(QList<int>() << 0 << 0 << 0);
	}
	updateTabVisibility();
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::loadDisplayViews(void)
{
	QSettings settings;
	const bool saved = settings.value("luaDebugger/prevStateSaved", false).toBool();

	layoutOpt = settings.value("luaDebugger/layoutOpt", 2).toInt();

	restoreGeometry(settings.value("luaDebugger/geometry").toByteArray());

	if (!saved)
	{
		setLayoutOption(2);
	}
	else
	{
		for (int i = 0; i < 2; i++)
		{
			for (int j = 0; j < 3; j++)
			{
				const QString val = settings.value(QStringLiteral("luaDebugger/tabView%1%2").arg(i + 1).arg(j + 1)).toString();

				for (const QString &name : val.split(',', Qt::SkipEmptyParts))
				{
					for (int k = 0; k < 5; k++)
					{
						if (pages[k]->objectName() == name && tabWidgetOf(pages[k]) == nullptr)
						{
							addPage(tabView[i][j], pages[k]);
						}
					}
				}
			}
		}
		// Anything not restored goes to the first group
		for (int k = 0; k < 5; k++)
		{
			if (tabWidgetOf(pages[k]) == nullptr)
			{
				addPage(tabView[0][0], pages[k]);
			}
		}
		hsplitter->restoreState(settings.value("luaDebugger/hPanelState").toByteArray());

		for (int i = 0; i < 2; i++)
		{
			vsplitter[i]->restoreState(settings.value(QStringLiteral("luaDebugger/vPanelState%1").arg(i + 1)).toByteArray());
		}
		updateTabVisibility();
	}
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::saveDisplayViews(void)
{
	QSettings settings;

	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			QString val;

			for (int k = 0; k < tabView[i][j]->count(); k++)
			{
				val += tabView[i][j]->widget(k)->objectName() + ",";
			}
			settings.setValue(QStringLiteral("luaDebugger/tabView%1%2").arg(i + 1).arg(j + 1), val);
		}
		settings.setValue(QStringLiteral("luaDebugger/vPanelState%1").arg(i + 1), vsplitter[i]->saveState());
	}
	settings.setValue("luaDebugger/hPanelState", hsplitter->saveState());
	settings.setValue("luaDebugger/geometry", saveGeometry());
	settings.setValue("luaDebugger/layoutOpt", layoutOpt);
	settings.setValue("luaDebugger/prevStateSaved", true);
}
//----------------------------------------------------------------------------
LuaSourceDebuggerDialog_t::~LuaSourceDebuggerDialog_t(void)
{
	QSettings settings;

	periodicTimer->stop();

	saveDisplayViews();
	settings.setValue("luaDebugger/watches", watches);

	FCEU_WRAPPER_LOCK();
	FCEU_LuaDbgDetach();
	FCEU_WRAPPER_UNLOCK();

	if (debuggerWin == this)
	{
		debuggerWin = nullptr;
	}
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::closeEvent(QCloseEvent *event)
{
	saveDisplayViews();
	done(0);
	deleteLater();
	event->accept();
}
//----------------------------------------------------------------------------
QMenuBar *LuaSourceDebuggerDialog_t::buildMenuBar(void)
{
	QMenuBar *menuBar = new QMenuBar(this);
	QMenu *menu;
	QAction *act;

	// File
	menu = menuBar->addMenu(tr("&File"));

	openAct = act = new QAction(tr("&Open Script..."), this);
	act->setShortcut(QKeySequence(tr("Ctrl+O")));
	act->setStatusTip(tr("Open a Lua script in the source view"));
	act->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
	connect(act, &QAction::triggered, this, &LuaSourceDebuggerDialog_t::openScriptDialog);
	menu->addAction(act);

	reloadAct = act = new QAction(tr("&Reload Script File"), this);
	act->setShortcut(QKeySequence(tr("Ctrl+R")));
	act->setStatusTip(tr("Reload the source file from disk"));
	connect(act, &QAction::triggered, this, &LuaSourceDebuggerDialog_t::reloadScript);
	menu->addAction(act);

	menu->addSeparator();

	act = new QAction(tr("&Close"), this);
	act->setShortcut(QKeySequence(tr("Ctrl+W")));
	act->setStatusTip(tr("Close Window"));
	connect(act, &QAction::triggered, this, &QWidget::close);
	menu->addAction(act);

	// Debug
	menu = menuBar->addMenu(tr("&Debug"));

	runAct = act = new QAction(tr("&Run / Continue"), this);
	act->setShortcut(QKeySequence(tr("F5")));
	act->setStatusTip(tr("Start the script, or continue execution when paused"));
	act->setIcon(QIcon(":icons/debug-run.png"));
	connect(act, &QAction::triggered, this, &LuaSourceDebuggerDialog_t::runContinueCB);
	menu->addAction(act);

	breakAct = act = new QAction(tr("&Break"), this);
	act->setShortcut(QKeySequence(tr("F6")));
	act->setStatusTip(tr("Pause the script at the next executed line"));
	act->setIcon(QIcon(":icons/debug-pause.png"));
	connect(act, &QAction::triggered, this, &LuaSourceDebuggerDialog_t::breakCB);
	menu->addAction(act);

	stopAct = act = new QAction(tr("&Stop Script"), this);
	act->setShortcut(QKeySequence(tr("Shift+F5")));
	act->setStatusTip(tr("Stop the running script"));
	act->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
	connect(act, &QAction::triggered, this, &LuaSourceDebuggerDialog_t::stopCB);
	menu->addAction(act);

	menu->addSeparator();

	stepIntoAct = act = new QAction(tr("Step &Into"), this);
	act->setShortcut(QKeySequence(tr("F11")));
	act->setStatusTip(tr("Step Into"));
	act->setIcon(QIcon(":icons/StepInto.png"));
	connect(act, &QAction::triggered, this, &LuaSourceDebuggerDialog_t::stepIntoCB);
	menu->addAction(act);

	stepOverAct = act = new QAction(tr("Step &Over"), this);
	act->setShortcut(QKeySequence(tr("F10")));
	act->setStatusTip(tr("Step Over"));
	act->setIcon(QIcon(":icons/StepOver.png"));
	connect(act, &QAction::triggered, this, &LuaSourceDebuggerDialog_t::stepOverCB);
	menu->addAction(act);

	stepOutAct = act = new QAction(tr("Step Ou&t"), this);
	act->setShortcut(QKeySequence(tr("Shift+F11")));
	act->setStatusTip(tr("Step Out"));
	act->setIcon(QIcon(":icons/StepOut.png"));
	connect(act, &QAction::triggered, this, &LuaSourceDebuggerDialog_t::stepOutCB);
	menu->addAction(act);

	restartAct = act = new QAction(tr("Re&start Script"), this);
	act->setShortcut(QKeySequence(tr("Ctrl+Shift+F5")));
	act->setStatusTip(tr("Stop the script if it is running and start it again"));
	act->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));
	connect(act, &QAction::triggered, this, &LuaSourceDebuggerDialog_t::restartCB);
	menu->addAction(act);

	runToCursorAct = act = new QAction(tr("Run to &Cursor"), this);
	act->setShortcut(QKeySequence(tr("Ctrl+F10")));
	act->setStatusTip(tr("Run until the line at the cursor is reached"));
	act->setIcon(QIcon(":icons/arrow-cursor.png"));
	connect(act, &QAction::triggered, this, &LuaSourceDebuggerDialog_t::runToCursorCB);
	menu->addAction(act);

	menu->addSeparator();

	toggleBpAct = act = new QAction(tr("Toggle Brea&kpoint"), this);
	act->setShortcut(QKeySequence(tr("F9")));
	act->setStatusTip(tr("Toggle a breakpoint on the current line"));
	connect(act, &QAction::triggered, this, &LuaSourceDebuggerDialog_t::toggleBreakpointCB);
	menu->addAction(act);

	act = new QAction(tr("&Enable/Disable Breakpoint"), this);
	act->setShortcut(QKeySequence(tr("Ctrl+F9")));
	act->setStatusTip(tr("Disable or re-enable the breakpoint on the current line"));
	connect(act, &QAction::triggered, this, &LuaSourceDebuggerDialog_t::toggleBreakpointEnabledCB);
	menu->addAction(act);

	act = new QAction(tr("Enable All Breakpoints"), this);
	connect(act, &QAction::triggered, this, [this]() { setAllBreakpointsEnabled(true); });
	menu->addAction(act);

	act = new QAction(tr("Disable All Breakpoints"), this);
	connect(act, &QAction::triggered, this, [this]() { setAllBreakpointsEnabled(false); });
	menu->addAction(act);

	act = new QAction(tr("Clear &All Breakpoints"), this);
	act->setStatusTip(tr("Remove all breakpoints"));
	connect(act, &QAction::triggered, this, &LuaSourceDebuggerDialog_t::clearBreakpointsCB);
	menu->addAction(act);

	menu->addSeparator();

	{
		QSettings settings;

		breakOnStartAct = act = new QAction(tr("Break on Script &Start"), this);
		act->setCheckable(true);
		act->setChecked(settings.value("luaDebugger/breakOnStart", false).toBool());
		act->setStatusTip(tr("Pause on the first line executed when a script starts"));
		connect(act, &QAction::toggled, this, &LuaSourceDebuggerDialog_t::breakOnStartCB);
		menu->addAction(act);
	}

	// Watch
	menu = menuBar->addMenu(tr("&Watch"));

	act = new QAction(tr("&Add Watch..."), this);
	act->setStatusTip(tr("Add a watch expression"));
	connect(act, &QAction::triggered, this, &LuaSourceDebuggerDialog_t::addWatchCB);
	menu->addAction(act);

	act = new QAction(tr("&Clear All Watches"), this);
	act->setStatusTip(tr("Remove all watch expressions"));
	connect(act, &QAction::triggered, this, &LuaSourceDebuggerDialog_t::clearWatchesCB);
	menu->addAction(act);

	// View
	menu = menuBar->addMenu(tr("&View"));

	{
		QSettings settings;
		QActionGroup *group = new QActionGroup(this);
		const bool dark = (settings.value("luaDebugger/theme", "light").toString() == "dark");

		lightThemeAct = act = new QAction(tr("&Light Syntax Theme"), this);
		act->setCheckable(true);
		act->setChecked(!dark);
		act->setActionGroup(group);
		menu->addAction(act);

		darkThemeAct = act = new QAction(tr("&Dark Syntax Theme"), this);
		act->setCheckable(true);
		act->setChecked(dark);
		act->setActionGroup(group);
		menu->addAction(act);

		connect(darkThemeAct, &QAction::toggled, this, &LuaSourceDebuggerDialog_t::themeCB);
		sourceView->setDarkTheme(dark);
	}

	menu->addSeparator();

	{
		QMenu *layoutMenu = menu->addMenu(tr("Layout &Presets"));
		QActionGroup *group = new QActionGroup(this);
		const char *names[] = { "&Compact", "Compact &Split", "&Wide", "Wide &Quad" };

		group->setExclusive(true);

		for (int i = 0; i < 4; i++)
		{
			act = new QAction(tr(names[i]), this);
			act->setStatusTip(tr("Arrange the Stack, Variables, Watch, Breakpoints and Output tabs"));
			connect(act, &QAction::triggered, this, [this, i]() { setLayoutOption(i + 1); });
			group->addAction(act);
			layoutMenu->addAction(act);
		}
	}

	return menuBar;
}
//----------------------------------------------------------------------------
QToolBar *LuaSourceDebuggerDialog_t::buildToolBar(void)
{
	QToolBar *toolBar = new QToolBar(this);

	toolBar->addAction(openAct);
	toolBar->addSeparator();
	toolBar->addAction(runAct);
	toolBar->addAction(breakAct);
	toolBar->addAction(stopAct);
	toolBar->addSeparator();
	toolBar->addAction(stepIntoAct);
	toolBar->addAction(stepOverAct);
	toolBar->addAction(stepOutAct);
	toolBar->addAction(runToCursorAct);
	toolBar->addSeparator();
	toolBar->addAction(restartAct);
	toolBar->addWidget(new QLabel(tr(" Args: ")));
	argsEdit = new QLineEdit();
	argsEdit->setClearButtonEnabled(true);
	argsEdit->setMinimumWidth(160);
	argsEdit->setMaximumWidth(320);
	argsEdit->setText(QSettings().value("luaDebugger/scriptArgs").toString());
	toolBar->addWidget(argsEdit);

	return toolBar;
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::buildOutputView(void)
{
	outputView = new QPlainTextEdit();
	outputView->setReadOnly(true);
	outputView->setLineWrapMode(QPlainTextEdit::NoWrap);
	outputView->setContextMenuPolicy(Qt::CustomContextMenu);
	outputView->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

	connect(outputView, &QWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
		QMenu *menu = outputView->createStandardContextMenu();
		QAction *clr = menu->addAction(tr("Clear Output"));

		connect(clr, &QAction::triggered, this, []() { LuaConsoleClear(); });
		menu->exec(outputView->viewport()->mapToGlobal(pos));
		delete menu;
	});
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::refreshOutput(void)
{
	const unsigned int serial = LuaConsoleSerial();

	if (serial == outputSerial)
	{
		return;
	}
	outputSerial = serial;

	QScrollBar *sb = outputView->verticalScrollBar();
	const bool atEnd = (sb->value() >= sb->maximum() - 2);
	const int prev = sb->value();

	outputView->setPlainText(QString::fromLocal8Bit(LuaConsoleText().c_str()));
	sb->setValue(atEnd ? sb->maximum() : prev);
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::buildStackView(void)
{
	stackView = new QTreeWidget();
	stackView->setColumnCount(3);
	stackView->setHeaderLabels(QStringList() << tr("#") << tr("Function") << tr("Location"));
	stackView->setRootIsDecorated(false);
	stackView->setUniformRowHeights(true);
	stackView->setSelectionMode(QAbstractItemView::SingleSelection);
	stackView->header()->setStretchLastSection(true);
	stackView->setColumnWidth(0, 40);
	stackView->setColumnWidth(1, 140);

	connect(stackView, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem *, QTreeWidgetItem *) { frameChanged(); });
}
//----------------------------------------------------------------------------
QTreeWidget *LuaSourceDebuggerDialog_t::buildVarTree(void)
{
	QTreeWidget *tree = new QTreeWidget();

	tree->setColumnCount(3);
	tree->setHeaderLabels(QStringList() << tr("Name") << tr("Value") << tr("Type"));
	tree->setUniformRowHeights(true);
	tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
	tree->setContextMenuPolicy(Qt::CustomContextMenu);
	tree->setColumnWidth(0, 160);
	tree->setColumnWidth(1, 200);

	connect(tree, &QTreeWidget::itemExpanded, this, &LuaSourceDebuggerDialog_t::varItemExpanded);
	connect(tree, &QTreeWidget::itemCollapsed, this, &LuaSourceDebuggerDialog_t::varItemCollapsed);
	connect(tree, &QTreeWidget::itemChanged, this, &LuaSourceDebuggerDialog_t::varItemChanged);
	connect(tree, &QTreeWidget::itemDoubleClicked, this, &LuaSourceDebuggerDialog_t::varItemDoubleClicked);
	connect(tree, &QWidget::customContextMenuRequested, this, &LuaSourceDebuggerDialog_t::varContextMenu);

	return tree;
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::buildVarView(void)
{
	varView = buildVarTree();
	watchView = buildVarTree();
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::buildBreakpointView(void)
{
	bpView = new QTreeWidget();
	bpView->setColumnCount(2);
	bpView->setHeaderLabels(QStringList() << tr("File") << tr("Line"));
	bpView->setRootIsDecorated(false);
	bpView->setUniformRowHeights(true);
	bpView->setSelectionMode(QAbstractItemView::ExtendedSelection);
	bpView->setContextMenuPolicy(Qt::CustomContextMenu);
	bpView->setColumnWidth(0, 220);

	connect(bpView, &QTreeWidget::itemChanged, this, &LuaSourceDebuggerDialog_t::bpItemChanged);
	connect(bpView, &QTreeWidget::itemActivated, this, &LuaSourceDebuggerDialog_t::bpItemActivated);
	connect(bpView, &QWidget::customContextMenuRequested, this, &LuaSourceDebuggerDialog_t::bpContextMenu);
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::refreshBreakpointList(void)
{
	bpRefreshing = true;
	bpView->clear();

	for (const auto &entry : breakpointMap)
	{
		const QString path = QString::fromLocal8Bit(entry.first.c_str());

		for (int line : entry.second)
		{
			QTreeWidgetItem *item = new QTreeWidgetItem();

			const bool dis = disabledMap[entry.first].count(line) != 0;

			item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
			item->setCheckState(0, dis ? Qt::Unchecked : Qt::Checked);
			item->setText(0, QFileInfo(path).fileName());
			item->setText(1, QString::number(line));
			item->setToolTip(0, path);
			item->setData(0, Qt::UserRole, path);
			item->setData(1, Qt::UserRole, line);
			bpView->addTopLevelItem(item);
		}
	}
	bpRefreshing = false;
	setPageTitle(bpView, bpView->topLevelItemCount() > 0 ? tr("Breakpoints (%1)").arg(bpView->topLevelItemCount()) : tr("Breakpoints"));
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::applyBreakpointEnable(const std::string &path, int line, bool enable)
{
	if (enable)
	{
		disabledMap[path].erase(line);
	}
	else
	{
		disabledMap[path].insert(line);
	}
	FCEU_LuaDbgSetBreakpoint(path, line, enable);
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::syncSourceMarkers(void)
{
	sourceView->setBreakpoints(breakpointMap[sourceView->file()], disabledMap[sourceView->file()]);
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::bpItemChanged(QTreeWidgetItem *item, int column)
{
	if (bpRefreshing || column != 0)
	{
		return;
	}
	const std::string path = item->data(0, Qt::UserRole).toString().toLocal8Bit().toStdString();
	const int line = item->data(1, Qt::UserRole).toInt();

	applyBreakpointEnable(path, line, item->checkState(0) == Qt::Checked);
	syncSourceMarkers();
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::setSelectedBreakpointsEnabled(bool enable)
{
	const QList<QTreeWidgetItem *> sel = bpView->selectedItems();

	for (QTreeWidgetItem *item : sel)
	{
		applyBreakpointEnable(item->data(0, Qt::UserRole).toString().toLocal8Bit().toStdString(),
		                      item->data(1, Qt::UserRole).toInt(), enable);
	}
	syncSourceMarkers();
	refreshBreakpointList();
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::setAllBreakpointsEnabled(bool enable)
{
	for (const auto &entry : breakpointMap)
	{
		for (int line : entry.second)
		{
			applyBreakpointEnable(entry.first, line, enable);
		}
	}
	syncSourceMarkers();
	refreshBreakpointList();
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::toggleBreakpointEnabledCB(void)
{
	if (sourceView->file().empty())
	{
		return;
	}
	const int line = sourceView->currentLine();

	if (breakpointMap[sourceView->file()].count(line) == 0)
	{
		return;
	}
	applyBreakpointEnable(sourceView->file(), line, disabledMap[sourceView->file()].count(line) != 0);
	syncSourceMarkers();
	refreshBreakpointList();
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::bpItemActivated(QTreeWidgetItem *item, int)
{
	const QString path = item->data(0, Qt::UserRole).toString();
	const int line = item->data(1, Qt::UserRole).toInt();

	if (sourceView->file() != path.toLocal8Bit().toStdString() && !openScript(path))
	{
		return;
	}
	QTextCursor cursor(sourceView->document()->findBlockByNumber(line - 1));

	sourceView->setTextCursor(cursor);
	sourceView->centerCursor();
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::removeSelectedBreakpoints(void)
{
	const QList<QTreeWidgetItem *> sel = bpView->selectedItems();

	for (QTreeWidgetItem *item : sel)
	{
		const std::string path = item->data(0, Qt::UserRole).toString().toLocal8Bit().toStdString();
		const int line = item->data(1, Qt::UserRole).toInt();

		breakpointMap[path].erase(line);
		disabledMap[path].erase(line);
		FCEU_LuaDbgSetBreakpoint(path, line, false);
	}
	syncSourceMarkers();
	refreshBreakpointList();
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::bpContextMenu(const QPoint &pos)
{
	QMenu menu(this);
	QAction *goAct = nullptr;
	QAction *removeAct = nullptr;
	QAction *enableAct = nullptr;
	QAction *disableAct = nullptr;
	QAction *clearAct;
	QTreeWidgetItem *item = bpView->itemAt(pos);

	if (item != nullptr)
	{
		goAct = menu.addAction(tr("Go to Source"));
		enableAct = menu.addAction(tr("Enable Breakpoint"));
		disableAct = menu.addAction(tr("Disable Breakpoint"));
		removeAct = menu.addAction(tr("Remove Breakpoint"));
	}
	QAction *enableAllAct = menu.addAction(tr("Enable All Breakpoints"));
	QAction *disableAllAct = menu.addAction(tr("Disable All Breakpoints"));
	enableAllAct->setEnabled(bpView->topLevelItemCount() > 0);
	disableAllAct->setEnabled(bpView->topLevelItemCount() > 0);
	clearAct = menu.addAction(tr("Remove All Breakpoints"));
	clearAct->setEnabled(bpView->topLevelItemCount() > 0);

	QAction *sel = menu.exec(bpView->viewport()->mapToGlobal(pos));

	if (sel == nullptr)
	{
		return;
	}
	if (sel == goAct)
	{
		bpItemActivated(item, 0);
	}
	else if (sel == enableAct || sel == disableAct)
	{
		if (!item->isSelected())
		{
			item->setSelected(true);
		}
		setSelectedBreakpointsEnabled(sel == enableAct);
	}
	else if (sel == enableAllAct || sel == disableAllAct)
	{
		setAllBreakpointsEnabled(sel == enableAllAct);
	}
	else if (sel == removeAct)
	{
		item->setSelected(true);
		removeSelectedBreakpoints();
	}
	else if (sel == clearAct)
	{
		clearBreakpointsCB();
	}
}
//----------------------------------------------------------------------------
bool LuaSourceDebuggerDialog_t::openScript(const QString &path)
{
	const std::string norm = FCEU_LuaDbgNormalizePath(path.toLocal8Bit().constData());

	if (!sourceView->loadFile(path, norm))
	{
		QMessageBox::warning(this, tr("Open Script"), tr("Failed to open file:\n%1").arg(path));
		return false;
	}
	syncSourceMarkers();
	setWindowTitle(tr("Lua Source Debugger - %1").arg(QFileInfo(path).fileName()));
	return true;
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::openScriptDialog(void)
{
	QString start;
	QString path;

	if (!sourceView->file().empty())
	{
		start = QFileInfo(QString::fromLocal8Bit(sourceView->file().c_str())).absolutePath();
	}
	path = QFileDialog::getOpenFileName(this, tr("Open Lua Script"), start,
	                                    tr("Lua scripts (*.lua *.LUA);;Any files (*)"));

	if (!path.isEmpty())
	{
		openScript(path);
	}
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::reloadScript(void)
{
	if (!sourceView->file().empty())
	{
		const int line = sourceView->currentLine();

		if (openScript(QString::fromLocal8Bit(sourceView->file().c_str())))
		{
			if (FCEU_LuaDbgIsPaused())
			{
				refreshPausedState();
			}
			else
			{
				sourceView->setExecLine(-1, true);
			}
			QTextCursor cursor(sourceView->document()->findBlockByNumber(line - 1));
			sourceView->setTextCursor(cursor);
		}
	}
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::updateActions(bool scriptRunning, bool paused)
{
	runAct->setEnabled(paused || (!scriptRunning && !sourceView->file().empty()));
	breakAct->setEnabled(scriptRunning && !paused);
	stopAct->setEnabled(scriptRunning);
	stepIntoAct->setEnabled(paused);
	stepOverAct->setEnabled(paused);
	stepOutAct->setEnabled(paused);
	runToCursorAct->setEnabled(paused && !sourceView->file().empty());
	toggleBpAct->setEnabled(!sourceView->file().empty());
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::updatePeriodic(void)
{
	refreshOutput();
	LuaConsolePollKillPrompt(this);

	// Follow scripts started from elsewhere (e.g. the Lua Control window)
	{
		std::string name;

		FCEU_WRAPPER_LOCK();
		name = FCEU_LuaDbgGetScriptName();
		FCEU_WRAPPER_UNLOCK();

		const bool started = (luaRunning != 0) && !prevRunning;

		prevRunning = (luaRunning != 0);

		if (!name.empty() && (name != lastScriptName || started))
		{
			lastScriptName = name;
			openScript(QString::fromLocal8Bit(name.c_str()));
		}
	}

	if (restartPending && !luaRunning && !FCEU_LuaDbgIsPaused())
	{
		restartPending = false;
		startScript();
	}

	const bool paused = FCEU_LuaDbgIsPaused();
	const bool running = (luaRunning != 0);
	const unsigned int serial = FCEU_LuaDbgPauseSerial();

	if (paused)
	{
		if (!wasPaused || (serial != lastSerial))
		{
			lastSerial = serial;
			refreshPausedState();
		}
	}
	else if (wasPaused)
	{
		clearPausedState();
	}
	wasPaused = paused;

	updateActions(running, paused);
	restartAct->setEnabled(!sourceView->file().empty());

	if (paused)
	{
		if (!frames.empty())
		{
			statusLabel->setText(tr("Paused at %1:%2").arg(QString::fromStdString(frames[0].shortSource)).arg(frames[0].currentLine));
		}
	}
	else
	{
		statusLabel->setText(running ? tr("Running") : tr("No script running"));
	}
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::refreshPausedState(void)
{
	stackView->clear();
	frames.clear();

	if (!FCEU_LuaDbgGetStack(frames))
	{
		return;
	}
	for (const LuaDbgFrame &f : frames)
	{
		QTreeWidgetItem *item = new QTreeWidgetItem();
		const QString loc = (f.currentLine > 0) ? QStringLiteral("%1:%2").arg(QString::fromStdString(f.shortSource)).arg(f.currentLine)
		                                         : QString::fromStdString(f.shortSource);

		item->setText(0, QString::number(f.level));
		item->setText(1, QString::fromStdString(f.name));
		item->setText(2, loc);
		item->setData(0, Qt::UserRole, f.level);
		stackView->addTopLevelItem(item);
	}
	if (stackView->topLevelItemCount() > 0)
	{
		stackView->setCurrentItem(stackView->topLevelItem(0));
	}
	activateWindow();
	raise();
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::clearPausedState(void)
{
	stackView->clear();
	varView->clear();
	watchView->clear();
	varStore.clear();
	frames.clear();
	sourceView->setExecLine(-1, true);
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::frameChanged(void)
{
	QTreeWidgetItem *item = stackView->currentItem();

	if (item == nullptr || !FCEU_LuaDbgIsPaused())
	{
		return;
	}
	showFrame(item->data(0, Qt::UserRole).toInt());
	refreshVariables();
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::showFrame(int level)
{
	if (level < 0 || level >= (int)frames.size())
	{
		return;
	}
	const LuaDbgFrame &f = frames[level];

	if (f.source.empty())
	{
		sourceView->setExecLine(-1, true);
		return;
	}
	if (sourceView->file() != f.source)
	{
		if (!openScript(QString::fromLocal8Bit(f.source.c_str())))
		{
			return;
		}
	}
	sourceView->setExecLine(f.currentLine, level == 0);
}
//----------------------------------------------------------------------------
static bool isIdentifier(const QString &s)
{
	static const QRegularExpression re("^[A-Za-z_][A-Za-z0-9_]*$");

	return re.match(s).hasMatch();
}
//----------------------------------------------------------------------------
QString LuaSourceDebuggerDialog_t::itemPath(QTreeWidgetItem *item) const
{
	QStringList parts;

	if (item != nullptr && item->treeWidget() == watchView)
	{
		parts.append(QStringLiteral("\x01Watch"));
	}
	while (item != nullptr)
	{
		parts.prepend(item->text(0));
		item = item->parent();
	}
	return parts.join(QChar(0x1f));
}
//----------------------------------------------------------------------------
QString LuaSourceDebuggerDialog_t::itemExpression(QTreeWidgetItem *item) const
{
	QStringList names;
	QString expr;
	const bool inWatch = (item != nullptr && item->treeWidget() == watchView);

	while (item != nullptr && (item->parent() != nullptr || inWatch))
	{
		names.prepend(item->text(0));
		item = item->parent();
	}
	for (int i = 0; i < names.size(); i++)
	{
		const QString &n = names[i];

		if (i == 0)
		{
			expr = inWatch ? QStringLiteral("(") + n + QStringLiteral(")") : n;
		}
		else if (n.startsWith(QChar('[')))
		{
			expr += n;
		}
		else if (isIdentifier(n))
		{
			expr += QStringLiteral(".") + n;
		}
		else
		{
			QString esc = n;

			esc.replace(QStringLiteral("\\"), QStringLiteral("\\\\"));
			esc.replace(QStringLiteral("\""), QStringLiteral("\\\""));
			expr += QStringLiteral("[\"") + esc + QStringLiteral("\"]");
		}
	}
	return expr;
}
//----------------------------------------------------------------------------
QTreeWidgetItem *LuaSourceDebuggerDialog_t::addVarItem(QTreeWidgetItem *parent, const LuaDbgVar &var, bool editable)
{
	QTreeWidgetItem *item = new QTreeWidgetItem();

	varStore.push_back(var);

	item->setText(0, QString::fromStdString(var.name));
	item->setText(1, QString::fromStdString(var.value));
	item->setText(2, QString::fromStdString(var.type));
	item->setData(0, ITEM_VAR_ROLE, (int)varStore.size() - 1);
	item->setData(0, ITEM_ROOT_ROLE, -1);

	if (editable && var.ref == 0)
	{
		item->setFlags(item->flags() | Qt::ItemIsEditable);
	}
	if (var.ref != 0)
	{
		// Placeholder so the expansion arrow is shown; replaced when expanded.
		QTreeWidgetItem *dummy = new QTreeWidgetItem();

		dummy->setData(0, ITEM_VAR_ROLE, -2);
		item->addChild(dummy);
	}
	if (parent != nullptr)
	{
		parent->addChild(item);
		restoreExpansion(item);
	}

	return item;
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::restoreExpansion(QTreeWidgetItem *item)
{
	if (item->childCount() > 0 && expandedPaths.contains(itemPath(item)))
	{
		item->setExpanded(true);
	}
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::populateRoot(QTreeWidgetItem *root, int rootKind)
{
	QTreeWidgetItem *cur = stackView->currentItem();
	const int level = cur ? cur->data(0, Qt::UserRole).toInt() : 0;
	std::vector<LuaDbgVar> vars;

	if (rootKind == ROOT_LOCALS || rootKind == ROOT_UPVALUES)
	{
		FCEU_LuaDbgGetScope(level, rootKind == ROOT_LOCALS ? LUADBG_SCOPE_LOCAL : LUADBG_SCOPE_UPVALUE, vars);

		for (const LuaDbgVar &var : vars)
		{
			addVarItem(root, var, true);
		}
	}
	else
	{
		QTreeWidgetItem *dummy = new QTreeWidgetItem();

		dummy->setData(0, ITEM_VAR_ROLE, -2);
		root->addChild(dummy);
	}
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::refreshVariables(void)
{
	static const char *rootNames[] = { "Watch", "Locals", "Upvalues", "Globals" };

	varView->setUpdatesEnabled(false);
	updatingVars = true;

	varView->clear();
	varStore.clear();

	if (FCEU_LuaDbgIsPaused())
	{
		for (int i = ROOT_LOCALS; i <= ROOT_GLOBALS; i++)
		{
			QTreeWidgetItem *root = new QTreeWidgetItem();

			root->setText(0, tr(rootNames[i]));
			root->setData(0, ITEM_VAR_ROLE, -1);
			root->setData(0, ITEM_ROOT_ROLE, i);
			varView->addTopLevelItem(root);

			populateRoot(root, i);
			restoreExpansion(root);
		}
	}
	updatingVars = false;
	varView->setUpdatesEnabled(true);

	refreshWatches();
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::refreshWatches(void)
{
	QTreeWidgetItem *cur = stackView->currentItem();
	const int level = cur ? cur->data(0, Qt::UserRole).toInt() : 0;

	updatingVars = true;
	watchView->clear();

	if (FCEU_LuaDbgIsPaused())
	{
		for (const QString &w : watches)
		{
			LuaDbgVar var;

			FCEU_LuaDbgEvaluate(level, w.toLocal8Bit().constData(), var);
			var.name = w.toLocal8Bit().constData();

			QTreeWidgetItem *item = addVarItem(nullptr, var, false);

			watchView->addTopLevelItem(item);
			restoreExpansion(item);
		}
	}
	updatingVars = false;
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::populateChildren(QTreeWidgetItem *item)
{
	if (item->childCount() != 1 || item->child(0)->data(0, ITEM_VAR_ROLE).toInt() != -2)
	{
		return;
	}
	std::vector<LuaDbgVar> vars;
	const int rootKind = item->data(0, ITEM_ROOT_ROLE).toInt();
	const int idx = item->data(0, ITEM_VAR_ROLE).toInt();

	delete item->takeChild(0);

	if (rootKind == ROOT_GLOBALS)
	{
		FCEU_LuaDbgGetScope(0, LUADBG_SCOPE_GLOBAL, vars);
	}
	else if (idx >= 0 && idx < (int)varStore.size())
	{
		FCEU_LuaDbgGetChildren(varStore[idx].ref, vars);
	}

	QCollator collator;
	collator.setNumericMode(true);

	std::sort(vars.begin(), vars.end(), [&collator](const LuaDbgVar &a, const LuaDbgVar &b) {
		return collator.compare(QString::fromStdString(a.name), QString::fromStdString(b.name)) < 0;
	});

	const bool wasUpdating = updatingVars;
	updatingVars = true;

	for (const LuaDbgVar &var : vars)
	{
		addVarItem(item, var, true);
	}
	updatingVars = wasUpdating;
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::varItemExpanded(QTreeWidgetItem *item)
{
	expandedPaths.insert(itemPath(item));
	populateChildren(item);
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::varItemCollapsed(QTreeWidgetItem *item)
{
	expandedPaths.remove(itemPath(item));
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::varItemDoubleClicked(QTreeWidgetItem *item, int column)
{
	if (column == 1 && (item->flags() & Qt::ItemIsEditable))
	{
		item->treeWidget()->editItem(item, 1);
	}
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::varItemChanged(QTreeWidgetItem *item, int column)
{
	if (updatingVars || column != 1 || !FCEU_LuaDbgIsPaused())
	{
		return;
	}
	const int idx = item->data(0, ITEM_VAR_ROLE).toInt();

	if (idx < 0 || idx >= (int)varStore.size())
	{
		return;
	}
	std::string err;

	if (!FCEU_LuaDbgSetValue(varStore[idx], item->text(1).toLocal8Bit().constData(), err))
	{
		QMessageBox::warning(this, tr("Set Value"), tr("Failed to set value: %1").arg(QString::fromLocal8Bit(err.c_str())));
	}
	// Rebuild after the editor closes; this item is being modified right now.
	QTimer::singleShot(0, this, [this]() { refreshVariables(); });
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::varContextMenu(const QPoint &pos)
{
	QTreeWidget *tree = qobject_cast<QTreeWidget *>(sender());

	if (tree == nullptr)
	{
		return;
	}
	QTreeWidgetItem *item = tree->itemAt(pos);
	QMenu menu(this);
	QAction *addAct = menu.addAction(tr("Add Watch..."));
	QAction *removeAct = nullptr;
	QAction *watchVarAct = nullptr;

	if (item != nullptr)
	{
		if (tree == watchView)
		{
			if (item->parent() == nullptr)
			{
				removeAct = menu.addAction(tr("Remove Watch"));
			}
		}
		else if (item->parent() != nullptr)
		{
			watchVarAct = menu.addAction(tr("Add to Watch"));
		}
	}

	QAction *sel = menu.exec(tree->viewport()->mapToGlobal(pos));

	if (sel == nullptr)
	{
		return;
	}
	if (sel == addAct)
	{
		addWatchCB();
	}
	else if (sel == removeAct)
	{
		watches.removeAll(item->text(0));
		saveWatches();
		refreshWatches();
	}
	else if (sel == watchVarAct)
	{
		const QString expr = itemExpression(item);

		if (!expr.isEmpty() && !watches.contains(expr))
		{
			watches.append(expr);
			saveWatches();
			refreshWatches();
		}
	}
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::saveWatches(void)
{
	QSettings settings;

	settings.setValue("luaDebugger/watches", watches);
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::addWatchCB(void)
{
	bool ok = false;
	const QString expr = QInputDialog::getText(this, tr("Add Watch"), tr("Lua expression:"), QLineEdit::Normal, QString(), &ok).trimmed();

	if (ok && !expr.isEmpty() && !watches.contains(expr))
	{
		watches.append(expr);
		saveWatches();

		refreshWatches();
	}
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::clearWatchesCB(void)
{
	watches.clear();
	saveWatches();

	refreshWatches();
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::runContinueCB(void)
{
	if (FCEU_LuaDbgIsPaused())
	{
		FCEU_LuaDbgContinue();
	}
	else if (!luaRunning && !sourceView->file().empty())
	{
		startScript();
	}
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::startScript(void)
{
	const QByteArray args = argsEdit->text().toLocal8Bit();

	QSettings().setValue("luaDebugger/scriptArgs", argsEdit->text());
	LuaConsoleClear();

	FCEU_WRAPPER_LOCK();
	if (0 == FCEU_LoadLuaCode(sourceView->file().c_str(), args.isEmpty() ? nullptr : args.constData()))
	{
		QMessageBox::warning(this, tr("Run Script"), tr("Could not load the Lua script:\n%1").arg(QString::fromStdString(sourceView->file())));
	}
	FCEU_WRAPPER_UNLOCK();
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::restartCB(void)
{
	if (sourceView->file().empty())
	{
		return;
	}
	if (luaRunning || FCEU_LuaDbgIsPaused())
	{
		stopCB();
		restartPending = true;
		return;
	}
	startScript();
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::breakCB(void)
{
	FCEU_LuaDbgRequestPause();
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::stopCB(void)
{
	FCEU_WRAPPER_LOCK();
	FCEU_LuaStop();
	FCEU_WRAPPER_UNLOCK();
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::stepIntoCB(void)
{
	FCEU_LuaDbgStepInto();
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::stepOverCB(void)
{
	FCEU_LuaDbgStepOver();
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::stepOutCB(void)
{
	FCEU_LuaDbgStepOut();
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::runToCursorCB(void)
{
	runToLineCB(sourceView->currentLine());
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::runToLineCB(int line)
{
	if (!sourceView->file().empty())
	{
		FCEU_LuaDbgRunToLine(sourceView->file(), line);
	}
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::gutterClickedCB(int line)
{
	if (sourceView->file().empty())
	{
		return;
	}
	std::set<int> &bps = breakpointMap[sourceView->file()];
	const bool enable = (bps.count(line) == 0);

	disabledMap[sourceView->file()].erase(line);

	if (enable)
	{
		bps.insert(line);
	}
	else
	{
		bps.erase(line);
	}
	FCEU_LuaDbgSetBreakpoint(sourceView->file(), line, enable);
	syncSourceMarkers();
	refreshBreakpointList();
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::toggleBreakpointCB(void)
{
	gutterClickedCB(sourceView->currentLine());
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::clearBreakpointsCB(void)
{
	breakpointMap.clear();
	disabledMap.clear();
	FCEU_LuaDbgClearBreakpoints();
	sourceView->setBreakpoints(std::set<int>(), std::set<int>());
	refreshBreakpointList();
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::breakOnStartCB(bool checked)
{
	QSettings settings;

	settings.setValue("luaDebugger/breakOnStart", checked);
	FCEU_LuaDbgSetBreakOnStart(checked);
}
//----------------------------------------------------------------------------
void LuaSourceDebuggerDialog_t::themeCB(bool dark)
{
	QSettings settings;

	settings.setValue("luaDebugger/theme", dark ? "dark" : "light");
	sourceView->setDarkTheme(dark);
}
//----------------------------------------------------------------------------
void openLuaSourceDebugger(QWidget *parent, const QString &scriptPath)
{
	if (debuggerWin == nullptr)
	{
		debuggerWin = new LuaSourceDebuggerDialog_t(parent);
	}
	if (!scriptPath.isEmpty() && !FCEU_LuaDbgIsPaused())
	{
		debuggerWin->openScript(scriptPath);
	}
	debuggerWin->show();
	debuggerWin->raise();
	debuggerWin->activateWindow();
}
//----------------------------------------------------------------------------

#endif
