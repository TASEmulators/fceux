// ProfilerDialog.h
//

#pragma once

#ifdef __FCEU_PROFILER_ENABLE__

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTimer>

#include "Qt/ProfilerCapture.h"

class ProfilerDialog_t : public QDialog
{
	Q_OBJECT

public:
	ProfilerDialog_t(QWidget *parent = nullptr);
	~ProfilerDialog_t(void);

protected:
	void closeEvent(QCloseEvent *event) override;

private:
	void startCapture(void);
	void stopCapture(void);
	void updateStatus(void);

	QLineEdit   *pathEdit;
	QPushButton *browseBtn;
	QPushButton *startStopBtn;
	QLabel      *statusLabel;
	QTimer      *updateTimer;

	ProfilerCaptureThread_t *captureThread = nullptr;

private slots:
	void browseClicked(void);
	void startStopClicked(void);
	void captureFinished(void);
	void captureError(const QString &msg);
	void updatePeriodic(void);
	void closeWindow(void);
};

#endif // __FCEU_PROFILER_ENABLE__
