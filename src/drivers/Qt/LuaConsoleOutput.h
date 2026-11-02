#pragma once

#include <deque>

#include <QByteArray>
#include <QMutex>
#include <QObject>
#include <QString>

class QPlainTextEdit;

// Application-lifetime output history; neither Lua window owns this object.
class LuaConsoleOutput : public QObject
{
	Q_OBJECT

public:
	// Complete lines retained in history and shown by views.
	static constexpr int historyLineLimit = 5000;

	// Initialize on the GUI thread before starting Lua.
	static LuaConsoleOutput &instance(void);

	// Thread-safe; output is batched and delivered asynchronously on the GUI thread.
	// Line endings are normalized to '\n'.
	void append(const char *text);
	void clear(void);

	// Delivered history, for initializing a newly opened view.
	// Call on the GUI thread; text not yet delivered arrives later through appended().
	QString text(void) const;

signals:
	// Views must discard their contents; emitted before appended() in the same batch.
	void reset(void);
	// New text since the previous delivery.
	void appended(const QString &text);

private:
	Q_SIGNAL
	void deliveryRequested(void);

	// Complete lines (each ending in '\n') plus any unterminated final line.
	struct LineBuffer
	{
		std::deque<QByteArray> lines;
		QByteArray tail;

		void append(const QByteArray &text);
		// Drops the oldest complete lines; returns true if any were dropped.
		bool trim(int maxLines);
		QByteArray toBytes(void) const;
		void clear(void);
	};

	LuaConsoleOutput(void);
	void publish(void);
	void scheduleDelivery(void);

	mutable QMutex mutex;
	LineBuffer history;
	LineBuffer pending;
	bool pendingReset = false;
	bool deliveryPending = false;
	bool lastWasCarriageReturn = false;
};

// Load the current history into a view and keep it updated by appending new output.
void LuaConsoleAttachView(QPlainTextEdit *view);
