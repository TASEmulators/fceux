// ProfilerDialog.cpp
//
#ifdef __FCEU_PROFILER_ENABLE__

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QSettings>
#include <QDir>

#include "driver.h"
#include "Qt/main.h"
#include "Qt/ProfilerDialog.h"

//----------------------------------------------------------------------------
ProfilerDialog_t::ProfilerDialog_t(QWidget *parent)
	: QDialog(parent)
{
	QVBoxLayout *mainLayout = new QVBoxLayout();
	QHBoxLayout *hbox;
	QPushButton *closeBtn;
	QSettings settings;

	setWindowTitle(tr("Profiler"));
	resize(512, 128);

	QString defaultPath = QDir(QString::fromUtf8(FCEUI_GetBaseDirectory())).filePath("profile.fcprof");

	hbox = new QHBoxLayout();
	hbox->addWidget(new QLabel(tr("Capture File:")));
	pathEdit = new QLineEdit(settings.value("profilerDialog/capturePath", defaultPath).toString());
	hbox->addWidget(pathEdit, 1);
	browseBtn = new QPushButton(tr("Browse..."));
	hbox->addWidget(browseBtn);
	mainLayout->addLayout(hbox);

	statusLabel = new QLabel(tr("Idle"));
	mainLayout->addWidget(statusLabel);
	mainLayout->addStretch(1);

	hbox = new QHBoxLayout();
	startStopBtn = new QPushButton(tr("Start Capture"));
	closeBtn = new QPushButton(tr("Close"));
	hbox->addWidget(startStopBtn);
	hbox->addStretch(1);
	hbox->addWidget(closeBtn);
	mainLayout->addLayout(hbox);

	setLayout(mainLayout);

	updateTimer = new QTimer(this);
	updateTimer->setInterval(250);

	connect(browseBtn, SIGNAL(clicked(void)), this, SLOT(browseClicked(void)));
	connect(startStopBtn, SIGNAL(clicked(void)), this, SLOT(startStopClicked(void)));
	connect(closeBtn, SIGNAL(clicked(void)), this, SLOT(closeWindow(void)));
	connect(updateTimer, SIGNAL(timeout(void)), this, SLOT(updatePeriodic(void)));
}
//----------------------------------------------------------------------------
ProfilerDialog_t::~ProfilerDialog_t(void)
{
	if (captureThread)
	{
		captureThread->stopCapture();
		captureThread->wait();
	}
}
//----------------------------------------------------------------------------
void ProfilerDialog_t::closeEvent(QCloseEvent *event)
{
	done(0);
	deleteLater();
	event->accept();
}
//----------------------------------------------------------------------------
void ProfilerDialog_t::closeWindow(void)
{
	done(0);
	deleteLater();
}
//----------------------------------------------------------------------------
void ProfilerDialog_t::browseClicked(void)
{
	QString path = QFileDialog::getSaveFileName(this, tr("Select Capture File"),
		pathEdit->text(), tr("Profiler Captures (*.fcprof);;All Files (*)"));

	if (!path.isEmpty())
	{
		pathEdit->setText(path);
	}
}
//----------------------------------------------------------------------------
void ProfilerDialog_t::startStopClicked(void)
{
	if (captureThread)
	{
		stopCapture();
	}
	else
	{
		startCapture();
	}
}
//----------------------------------------------------------------------------
void ProfilerDialog_t::startCapture(void)
{
	QSettings settings;
	QString path = pathEdit->text().trimmed();

	if (path.isEmpty())
	{
		QMessageBox::warning(this, tr("Profiler"), tr("Please select a capture file."));
		return;
	}
	settings.setValue("profilerDialog/capturePath", path);

	captureThread = new ProfilerCaptureThread_t(this);
	captureThread->setFilePath(path);

	connect(captureThread, SIGNAL(finished(void)), this, SLOT(captureFinished(void)));
	connect(captureThread, SIGNAL(captureError(const QString&)), this, SLOT(captureError(const QString&)));

	pathEdit->setEnabled(false);
	browseBtn->setEnabled(false);
	startStopBtn->setText(tr("Stop Capture"));

	captureThread->start();
	updateTimer->start();
	updateStatus();
}
//----------------------------------------------------------------------------
void ProfilerDialog_t::stopCapture(void)
{
	if (captureThread)
	{
		startStopBtn->setEnabled(false);
		statusLabel->setText(tr("Finalizing capture..."));
		captureThread->stopCapture();
	}
}
//----------------------------------------------------------------------------
void ProfilerDialog_t::captureFinished(void)
{
	updateTimer->stop();

	if (captureThread)
	{
		statusLabel->setText(tr("Capture saved: %1 markers, %2 bytes")
			.arg(captureThread->getMarkerCount()).arg(captureThread->getBytesWritten()));
		captureThread->deleteLater();
		captureThread = nullptr;
	}
	pathEdit->setEnabled(true);
	browseBtn->setEnabled(true);
	startStopBtn->setEnabled(true);
	startStopBtn->setText(tr("Start Capture"));
}
//----------------------------------------------------------------------------
void ProfilerDialog_t::captureError(const QString &msg)
{
	QMessageBox::critical(this, tr("Profiler"), msg);
}
//----------------------------------------------------------------------------
void ProfilerDialog_t::updateStatus(void)
{
	if (captureThread)
	{
		statusLabel->setText(tr("Capturing: %1 markers, %2 bytes")
			.arg(captureThread->getMarkerCount()).arg(captureThread->getBytesWritten()));
	}
}
//----------------------------------------------------------------------------
void ProfilerDialog_t::updatePeriodic(void)
{
	updateStatus();
}
//----------------------------------------------------------------------------
#endif // __FCEU_PROFILER_ENABLE__
