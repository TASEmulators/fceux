#include "LuaConsoleOutput.h"

#include <cstring>

#include <QCoreApplication>
#include <QMutexLocker>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QThread>

//----------------------------------------------------
void LuaConsoleOutput::LineBuffer::append(const QByteArray &text)
{
	int start = 0;
	int newline;

	while ((newline = text.indexOf('\n', start)) >= 0)
	{
		tail.append(text.constData() + start, newline + 1 - start);
		lines.push_back(tail);
		tail.clear();
		start = newline + 1;
	}
	tail.append(text.constData() + start, text.size() - start);
}
//----------------------------------------------------
bool LuaConsoleOutput::LineBuffer::trim(int maxLines)
{
	const bool trimmed = lines.size() > static_cast<size_t>(maxLines);

	while (lines.size() > static_cast<size_t>(maxLines))
	{
		lines.pop_front();
	}
	return trimmed;
}
//----------------------------------------------------
QByteArray LuaConsoleOutput::LineBuffer::toBytes(void) const
{
	qsizetype size = tail.size();

	for (const QByteArray &line : lines)
	{
		size += line.size();
	}

	QByteArray bytes;
	bytes.reserve(size);
	for (const QByteArray &line : lines)
	{
		bytes.append(line);
	}
	bytes.append(tail);
	return bytes;
}
//----------------------------------------------------
void LuaConsoleOutput::LineBuffer::clear(void)
{
	lines.clear();
	tail.clear();
}
//----------------------------------------------------
LuaConsoleOutput &LuaConsoleOutput::instance(void)
{
	static LuaConsoleOutput output;
	return output;
}
//----------------------------------------------------
LuaConsoleOutput::LuaConsoleOutput(void)
{
	Q_ASSERT(QCoreApplication::instance() != nullptr);
	Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());
	connect(this, &LuaConsoleOutput::deliveryRequested,
		this, &LuaConsoleOutput::publish, Qt::QueuedConnection);
}
//----------------------------------------------------
void LuaConsoleOutput::append(const char *text)
{
	const size_t length = std::strlen(text);
	QByteArray normalized;

	normalized.reserve(static_cast<qsizetype>(length));

	QMutexLocker lock(&mutex);

	// Lua print() ends lines with "\r\n", and Qt views treat '\r' as its own line
	// break; normalize so history and views count lines identically.
	for (size_t i = 0; i < length; i++)
	{
		const char c = text[i];

		if (c == '\r')
		{
			normalized.append('\n');
			lastWasCarriageReturn = true;
			continue;
		}
		if ((c != '\n') || !lastWasCarriageReturn)
		{
			normalized.append(c);
		}
		lastWasCarriageReturn = false;
	}

	pending.append(normalized);
	if (pending.trim(historyLineLimit))
	{
		// The batch alone fills the history, so views must replace everything.
		pendingReset = true;
	}
	scheduleDelivery();
}
//----------------------------------------------------
void LuaConsoleOutput::clear(void)
{
	QMutexLocker lock(&mutex);
	pending.clear();
	pendingReset = true;
	lastWasCarriageReturn = false;
	scheduleDelivery();
}
//----------------------------------------------------
QString LuaConsoleOutput::text(void) const
{
	QMutexLocker lock(&mutex);
	return QString::fromLocal8Bit(history.toBytes());
}
//----------------------------------------------------
void LuaConsoleOutput::scheduleDelivery(void)
{
	// Called with the mutex held: output floods enqueue only one pending delivery.
	if (!deliveryPending)
	{
		deliveryPending = true;
		emit deliveryRequested();
	}
}
//----------------------------------------------------
void LuaConsoleOutput::publish(void)
{
	QByteArray chunk;
	bool doReset;
	{
		QMutexLocker lock(&mutex);
		chunk = pending.toBytes();
		pending.clear();
		doReset = pendingReset;
		pendingReset = false;
		deliveryPending = false;

		if (doReset)
		{
			history.clear();
		}
		history.append(chunk);
		history.trim(historyLineLimit);
	}
	if (doReset)
	{
		emit reset();
	}
	if (!chunk.isEmpty())
	{
		emit appended(QString::fromLocal8Bit(chunk));
	}
}
//----------------------------------------------------
static void appendToView(QPlainTextEdit *view, const QString &text)
{
	QTextDocument *doc = view->document();
	QScrollBar *sb = view->verticalScrollBar();
	const bool follow = sb->value() >= sb->maximum() - 2;

	// Remember the top visible block so trimming old lines doesn't shift a scrolled-back view.
	const QTextBlock top = doc->findBlockByLineNumber(sb->value());
	const int topNumber = top.blockNumber();
	const int topOffset = sb->value() - top.firstLineNumber();
	const int blocksBefore = doc->blockCount();

	// A separate cursor leaves the user's selection intact; the document's
	// maximum block count drops the oldest lines.
	QTextCursor cursor(doc);
	cursor.movePosition(QTextCursor::End);
	cursor.insertText(text);

	if (follow)
	{
		sb->setValue(sb->maximum());
		return;
	}

	const int removed = blocksBefore + static_cast<int>(text.count(QLatin1Char('\n'))) - doc->blockCount();

	if (removed > 0)
	{
		const int number = topNumber - removed;

		sb->setValue(number < 0 ? 0 : doc->findBlockByNumber(number).firstLineNumber() + topOffset);
	}
}
//----------------------------------------------------
void LuaConsoleAttachView(QPlainTextEdit *view)
{
	auto &output = LuaConsoleOutput::instance();

	// One more block than lines: the empty block after the final newline.
	view->setMaximumBlockCount(LuaConsoleOutput::historyLineLimit + 1);
	view->setUndoRedoEnabled(false);
	view->setPlainText(output.text());
	view->verticalScrollBar()->setValue(view->verticalScrollBar()->maximum());

	QObject::connect(&output, &LuaConsoleOutput::reset, view, [view]() { view->clear(); });
	QObject::connect(&output, &LuaConsoleOutput::appended, view,
		[view](const QString &text) { appendToView(view, text); });
}
