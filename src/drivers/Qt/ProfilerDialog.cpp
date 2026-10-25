// ProfilerDialog.cpp
//

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QSettings>
#include <QDir>
#include <QMenuBar>
#include <QAction>
#include <QKeySequence>
#include <QFileInfo>
#include <QStyle>
#include <QIcon>
#include <QPainter>
#include <QPixmap>

#include "driver.h"
#include "Qt/main.h"
#include "Qt/ProfilerDialog.h"

namespace
{
	// Magnifying glass with a plus or minus inside the lens, drawn in the given color
	QIcon makeZoomIcon(const QColor &color, bool zoomIn)
	{
		const int size = 64;
		QPixmap pm(size, size);
		pm.fill(Qt::transparent);

		QPainter p(&pm);
		p.setRenderHint(QPainter::Antialiasing);
		p.setPen(QPen(color, 7, Qt::SolidLine, Qt::RoundCap));
		p.drawEllipse(QPointF(27, 27), 20, 20);
		p.setPen(QPen(color, 6, Qt::SolidLine, Qt::RoundCap));
		p.drawLine(QPointF(43, 43), QPointF(58, 58));
		p.setPen(QPen(color, 7, Qt::SolidLine, Qt::RoundCap));
		p.drawLine(QPointF(17, 27), QPointF(37, 27));
		if (zoomIn)
		{
			p.drawLine(QPointF(27, 17), QPointF(27, 37));
		}
		p.end();
		return QIcon(pm);
	}

	// The marker the emulator places around each emulated frame (FCEUI_Emulate)
	const char *emulatorFrameComment = "Emulate Single Frame";
}
//----------------------------------------------------------------------------
ProfilerDialog_t::ProfilerDialog_t(QWidget *parent)
	: QDialog(parent)
{
	QVBoxLayout *mainLayout = new QVBoxLayout();
	QVBoxLayout *vbox;
	QHBoxLayout *hbox;
	QGroupBox *captureBox, *viewBox;
	QPushButton *closeBtn;
	QSettings settings;

	setWindowTitle(tr("Profiler"));
	resize(1100, 640);

	QString defaultPath = QDir(QString::fromUtf8(FCEUI_GetBaseDirectory())).filePath("profile.fcprof");

	// Capture controls
	captureBox = new QGroupBox(tr("Capture"));
	vbox = new QVBoxLayout();
	captureBox->setLayout(vbox);

	hbox = new QHBoxLayout();
	hbox->addWidget(new QLabel(tr("Capture File:")));
	pathEdit = new QLineEdit(settings.value("profilerDialog/capturePath", defaultPath).toString());
	hbox->addWidget(pathEdit, 1);
	browseBtn = new QPushButton(tr("Browse..."));
	hbox->addWidget(browseBtn);
	startStopBtn = new QPushButton(tr("Start Capture"));
	hbox->addWidget(startStopBtn);
	vbox->addLayout(hbox);

	statusLabel = new QLabel(tr("Idle"));
	vbox->addWidget(statusLabel);
	mainLayout->addWidget(captureBox);

	// Viewer
	viewBox = new QGroupBox(tr("Viewer"));
	vbox = new QVBoxLayout();
	viewBox->setLayout(vbox);

	hbox = new QHBoxLayout();
	openBtn = new QPushButton(tr("Open Capture..."));
	loadLabel = new QLabel(tr("No capture loaded"));
	hbox->addWidget(openBtn);
	hbox->addWidget(loadLabel, 1);
	vbox->addLayout(hbox);

	viewer = new ProfilerViewer_t();
	vbox->addWidget(viewer, 1);

	hbox = new QHBoxLayout();
	zoomInBtn = new QPushButton(tr("Zoom In"));
	zoomOutBtn = new QPushButton(tr("Zoom Out"));
	zoomFitBtn = new QPushButton(tr("Fit"));
	hbox->addWidget(zoomInBtn);
	hbox->addWidget(zoomOutBtn);
	hbox->addWidget(zoomFitBtn);
	hbox->addSpacing(16);
	hbox->addWidget(new QLabel(tr("Frame Marker:")));
	frameCombo = new QComboBox();
	frameCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
	hbox->addWidget(frameCombo);
	hbox->addWidget(new QLabel(tr("Frame:")));
	frameSpin = new QSpinBox();
	hbox->addWidget(frameSpin);
	framePrevBtn = new QPushButton(tr("Prev"));
	frameNextBtn = new QPushButton(tr("Next"));
	hbox->addWidget(framePrevBtn);
	hbox->addWidget(frameNextBtn);
	hbox->addStretch(1);
	vbox->addLayout(hbox);

	frameLabel = new QLabel();
	vbox->addWidget(frameLabel);

	hbox = new QHBoxLayout();
	hbox->addWidget(new QLabel(tr("Find Marker:")));
	findEdit = new QLineEdit();
	findEdit->setPlaceholderText(tr("name or function (empty matches all)"));
	findEdit->setClearButtonEnabled(true);
	hbox->addWidget(findEdit, 1);
	QPushButton *findFirstBtn = new QPushButton(tr("First"));
	QPushButton *findPrevBtn = new QPushButton(tr("Prev"));
	QPushButton *findNextBtn = new QPushButton(tr("Next"));
	QPushButton *findLastBtn = new QPushButton(tr("Last"));
	QPushButton *longestBtn = new QPushButton(tr("Longest"));
	QPushButton *nextLongestBtn = new QPushButton(tr("Next Longest"));
	hbox->addWidget(findFirstBtn);
	hbox->addWidget(findPrevBtn);
	hbox->addWidget(findNextBtn);
	hbox->addWidget(findLastBtn);
	hbox->addSpacing(16);
	hbox->addWidget(longestBtn);
	hbox->addWidget(nextLongestBtn);
	vbox->addLayout(hbox);

	findLabel = new QLabel();
	vbox->addWidget(findLabel);
	mainLayout->addWidget(viewBox, 1);

	hbox = new QHBoxLayout();
	closeBtn = new QPushButton(tr("Close"));
	hbox->addStretch(1);
	hbox->addWidget(closeBtn);
	mainLayout->addLayout(hbox);

	setLayout(mainLayout);

	// Prefer theme icons and fall back to the style's standard icons
	auto icon = [this](const char *theme, QStyle::StandardPixmap fallback)
	{
		return QIcon::fromTheme(QString(theme), style()->standardIcon(fallback));
	};
	const QIcon zoomInIcon = makeZoomIcon(palette().color(QPalette::ButtonText), true);
	const QIcon zoomOutIcon = makeZoomIcon(palette().color(QPalette::ButtonText), false);

	QMenuBar *menuBar = new QMenuBar(this);
	QMenu *menu = menuBar->addMenu(tr("&File"));
	QAction *act;

	act = menu->addAction(tr("&Open Capture..."), this, SLOT(openClicked(void)));
	act->setShortcut(QKeySequence::Open);
	act->setIcon(icon("document-open", QStyle::SP_DialogOpenButton));
	menu->addSeparator();
	act = menu->addAction(tr("&Close"), this, SLOT(closeWindow(void)));
	act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_W));
	act->setIcon(icon("window-close", QStyle::SP_DialogCloseButton));

	menu = menuBar->addMenu(tr("&View"));
	act = menu->addAction(tr("Zoom &In"), viewer, SLOT(zoomIn(void)));
	act->setShortcuts({QKeySequence(Qt::CTRL | Qt::Key_Plus), QKeySequence(Qt::CTRL | Qt::Key_Equal), QKeySequence(Qt::Key_Plus), QKeySequence(Qt::Key_Equal)});
	act->setIcon(zoomInIcon);
	act = menu->addAction(tr("Zoom &Out"), viewer, SLOT(zoomOut(void)));
	act->setShortcuts({QKeySequence(Qt::CTRL | Qt::Key_Minus), QKeySequence(Qt::Key_Minus)});
	act->setIcon(zoomOutIcon);
	act = menu->addAction(tr("&Fit All"), viewer, SLOT(zoomFit(void)));
	act->setShortcuts({QKeySequence(Qt::CTRL | Qt::Key_0), QKeySequence(Qt::Key_0)});
	act->setIcon(icon("zoom-fit-best", QStyle::SP_FileDialogDetailedView));
	menu->addSeparator();
	act = menu->addAction(tr("&Previous Frame"), this, SLOT(framePrevClicked(void)));
	act->setShortcuts({QKeySequence(Qt::Key_BracketLeft), QKeySequence(Qt::Key_Comma)});
	act->setIcon(icon("go-previous", QStyle::SP_ArrowLeft));
	act = menu->addAction(tr("&Next Frame"), this, SLOT(frameNextClicked(void)));
	act->setShortcuts({QKeySequence(Qt::Key_BracketRight), QKeySequence(Qt::Key_Period)});
	act->setIcon(icon("go-next", QStyle::SP_ArrowRight));

	menu = menuBar->addMenu(tr("F&ind"));
	act = menu->addAction(tr("&Find Marker..."), this, SLOT(focusFind(void)));
	act->setShortcut(QKeySequence::Find);
	act->setIcon(icon("edit-find", QStyle::SP_FileDialogContentsView));
	menu->addSeparator();
	act = menu->addAction(tr("&First Marker"), viewer, SLOT(findFirst(void)));
	act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Home));
	act->setIcon(icon("go-first", QStyle::SP_MediaSkipBackward));
	act = menu->addAction(tr("&Next Marker"), this, SLOT(findNextOrFirst(void)));
	act->setShortcuts({QKeySequence(Qt::Key_F3), QKeySequence(Qt::CTRL | Qt::Key_G)});
	act->setIcon(icon("go-next", QStyle::SP_MediaSeekForward));
	act = menu->addAction(tr("&Previous Marker"), viewer, SLOT(findPrevious(void)));
	act->setShortcuts({QKeySequence(Qt::SHIFT | Qt::Key_F3), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_G)});
	act->setIcon(icon("go-previous", QStyle::SP_MediaSeekBackward));
	act = menu->addAction(tr("&Last Marker"), viewer, SLOT(findLast(void)));
	act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_End));
	act->setIcon(icon("go-last", QStyle::SP_MediaSkipForward));
	menu->addSeparator();
	act = menu->addAction(tr("L&ongest Marker"), viewer, SLOT(findLongest(void)));
	act->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_L));
	act->setIcon(icon("go-top", QStyle::SP_ArrowUp));
	act = menu->addAction(tr("Ne&xt Longest Marker"), viewer, SLOT(findNextLongest(void)));
	act->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_L));
	act->setIcon(icon("go-down", QStyle::SP_ArrowDown));

	mainLayout->setMenuBar(menuBar);

	// No button is the dialog default, so Enter in a text field never triggers one
	for (QPushButton *btn : findChildren<QPushButton*>())
	{
		btn->setAutoDefault(false);
		btn->setDefault(false);
	}

	browseBtn->setIcon(icon("document-open", QStyle::SP_DirOpenIcon));
	startStopBtn->setIcon(icon("media-record", QStyle::SP_MediaPlay));
	openBtn->setIcon(icon("document-open", QStyle::SP_DialogOpenButton));
	zoomInBtn->setIcon(zoomInIcon);
	zoomOutBtn->setIcon(zoomOutIcon);
	zoomFitBtn->setIcon(icon("zoom-fit-best", QStyle::SP_FileDialogDetailedView));
	framePrevBtn->setIcon(icon("go-previous", QStyle::SP_ArrowLeft));
	frameNextBtn->setIcon(icon("go-next", QStyle::SP_ArrowRight));
	findFirstBtn->setIcon(icon("go-first", QStyle::SP_MediaSkipBackward));
	findPrevBtn->setIcon(icon("go-previous", QStyle::SP_MediaSeekBackward));
	findNextBtn->setIcon(icon("go-next", QStyle::SP_MediaSeekForward));
	findLastBtn->setIcon(icon("go-last", QStyle::SP_MediaSkipForward));
	longestBtn->setIcon(icon("go-top", QStyle::SP_ArrowUp));
	nextLongestBtn->setIcon(icon("go-down", QStyle::SP_ArrowDown));
	closeBtn->setIcon(icon("window-close", QStyle::SP_DialogCloseButton));
	updateFrameControls();

	updateTimer = new QTimer(this);
	updateTimer->setInterval(250);

	connect(browseBtn, SIGNAL(clicked(void)), this, SLOT(browseClicked(void)));
	connect(startStopBtn, SIGNAL(clicked(void)), this, SLOT(startStopClicked(void)));
	connect(closeBtn, SIGNAL(clicked(void)), this, SLOT(closeWindow(void)));
	connect(updateTimer, SIGNAL(timeout(void)), this, SLOT(updatePeriodic(void)));

	connect(openBtn, SIGNAL(clicked(void)), this, SLOT(openClicked(void)));
	connect(zoomInBtn, SIGNAL(clicked(void)), viewer, SLOT(zoomIn(void)));
	connect(zoomOutBtn, SIGNAL(clicked(void)), viewer, SLOT(zoomOut(void)));
	connect(zoomFitBtn, SIGNAL(clicked(void)), viewer, SLOT(zoomFit(void)));
	connect(frameCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(frameSiteChanged(int)));
	connect(frameSpin, SIGNAL(valueChanged(int)), this, SLOT(frameSpinChanged(int)));
	connect(viewer, SIGNAL(frameSelected(int)), this, SLOT(viewerFrameSelected(int)));
	connect(findEdit, SIGNAL(textChanged(const QString &)), this, SLOT(findTextChanged(const QString &)));
	connect(findEdit, SIGNAL(returnPressed(void)), this, SLOT(findNextOrFirst(void)));
	connect(findFirstBtn, SIGNAL(clicked(void)), viewer, SLOT(findFirst(void)));
	connect(findPrevBtn, SIGNAL(clicked(void)), viewer, SLOT(findPrevious(void)));
	connect(findNextBtn, SIGNAL(clicked(void)), this, SLOT(findNextOrFirst(void)));
	connect(findLastBtn, SIGNAL(clicked(void)), viewer, SLOT(findLast(void)));
	connect(longestBtn, SIGNAL(clicked(void)), viewer, SLOT(findLongest(void)));
	connect(nextLongestBtn, SIGNAL(clicked(void)), viewer, SLOT(findNextLongest(void)));
	connect(viewer, SIGNAL(findResult(const QString &)), findLabel, SLOT(setText(const QString &)));
	connect(framePrevBtn, SIGNAL(clicked(void)), this, SLOT(framePrevClicked(void)));
	connect(frameNextBtn, SIGNAL(clicked(void)), this, SLOT(frameNextClicked(void)));
}
//----------------------------------------------------------------------------
ProfilerDialog_t::~ProfilerDialog_t(void)
{
#ifdef __FCEU_PROFILER_ENABLE__
	if (captureThread)
	{
		captureThread->stopCapture();
		captureThread->wait();
	}
#endif
	if (loadThread)
	{
		loadThread->requestInterruption();
		loadThread->wait();
	}
}
//----------------------------------------------------------------------------
// Esc must not close the window; it is used to clear the measurement reference
void ProfilerDialog_t::reject(void)
{
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
void ProfilerDialog_t::findTextChanged(const QString &text)
{
	viewer->setFindText(text);
}
//----------------------------------------------------------------------------
void ProfilerDialog_t::focusFind(void)
{
	findEdit->setFocus();
	findEdit->selectAll();
}
//----------------------------------------------------------------------------
void ProfilerDialog_t::findNextOrFirst(void)
{
	viewer->findNext();
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
#ifndef __FCEU_PROFILER_ENABLE__
	QMessageBox::information(this, tr("Profiler"),
		tr("Capturing profiler data is not available in this build.\n\n"
		   "Rebuild FCEUX with the profiler capture tooling enabled "
		   "(cmake -DFCEU_PROFILER_ENABLE=ON) to record captures. "
		   "Existing capture files can still be opened and viewed."));
#else
	if (captureThread)
	{
		stopCapture();
	}
	else
	{
		startCapture();
	}
#endif
}
//----------------------------------------------------------------------------
#ifdef __FCEU_PROFILER_ENABLE__
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
	startStopBtn->setIcon(QIcon::fromTheme(QString("media-playback-stop"), style()->standardIcon(QStyle::SP_MediaStop)));

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
	startStopBtn->setIcon(QIcon::fromTheme(QString("media-record"), style()->standardIcon(QStyle::SP_MediaPlay)));
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
#else
void ProfilerDialog_t::captureFinished(void){}
void ProfilerDialog_t::captureError(const QString &){}
void ProfilerDialog_t::updateStatus(void){}
#endif
//----------------------------------------------------------------------------
void ProfilerDialog_t::updatePeriodic(void)
{
	updateStatus();
}
//----------------------------------------------------------------------------
void ProfilerDialog_t::openClicked(void)
{
	QString path = QFileDialog::getOpenFileName(this, tr("Open Capture File"),
		pathEdit->text(), tr("Profiler Captures (*.fcprof);;All Files (*)"));

	if (!path.isEmpty())
	{
		loadCapture(path);
	}
}
//----------------------------------------------------------------------------
void ProfilerDialog_t::loadCapture(const QString &path)
{
	if (loadThread)
	{
		return;
	}
	loadName = QFileInfo(path).fileName();
	loadThread = new ProfilerLoadThread_t(this);
	loadThread->setFilePath(path);

	connect(loadThread, SIGNAL(progress(int)), this, SLOT(loadProgress(int)));
	connect(loadThread, SIGNAL(finished(void)), this, SLOT(loadFinished(void)));

	openBtn->setEnabled(false);
	loadLabel->setText(tr("Loading %1 ...").arg(loadName));
	loadThread->start();
}
//----------------------------------------------------------------------------
void ProfilerDialog_t::loadProgress(int percent)
{
	if (loadThread)
	{
		loadLabel->setText(tr("Loading %1 ... %2%").arg(loadName).arg(percent));
	}
}
//----------------------------------------------------------------------------
void ProfilerDialog_t::loadFinished(void)
{
	ProfilerLoadThread_t *t = loadThread;

	loadThread = nullptr;
	openBtn->setEnabled(true);

	if (t == nullptr)
	{
		return;
	}
	if (!t->succeeded())
	{
		loadLabel->setText(tr("Failed to load capture"));
		QMessageBox::critical(this, tr("Profiler"), tr("Failed to load capture: %1").arg(t->errorString()));
		t->deleteLater();
		return;
	}
	auto data = t->data();
	QString info = tr("%1: %2 markers on %3 threads, %4")
		.arg(loadName).arg(data->markerCount).arg(data->threads.size())
		.arg(QString::number(data->countsToSeconds(data->duration), 'f', 3) + " s");

	if (data->truncated)
	{
		info += tr("  [file is truncated]");
	}
	else if (!data->hasFooter)
	{
		info += tr("  [capture was not finalized]");
	}
	if (data->unmatchedStarts || data->strayEnds)
	{
		info += tr("  [%1 unclosed, %2 stray end markers]").arg(data->unmatchedStarts).arg(data->strayEnds);
	}
	loadLabel->setText(info);

	viewer->setData(data);
	viewer->setFindText(findEdit->text());
	populateFrameCombo();
	t->deleteLater();
}
//----------------------------------------------------------------------------
void ProfilerDialog_t::populateFrameCombo(void)
{
	auto data = viewer->getData();
	int defaultIndex = 0;

	frameCombo->blockSignals(true);
	frameCombo->clear();

	if (data)
	{
		// Only list sites that actually produced blocks
		std::vector<bool> used(data->sites.size(), false);

		for (const ProfilerThreadData &t : data->threads)
		{
			for (const auto &row : t.depths)
			{
				for (const ProfilerBlock &b : row)
				{
					used[b.site] = true;
				}
			}
		}
		for (size_t i = 0; i < data->sites.size(); i++)
		{
			if (!used[i])
			{
				continue;
			}
			const ProfilerSite &s = data->sites[i];
			const QString name = s.comment.isEmpty() ? s.func : s.comment;

			if (s.comment == emulatorFrameComment)
			{
				defaultIndex = frameCombo->count();
			}
			frameCombo->addItem(QString("%1  (%2:%3)").arg(name).arg(QFileInfo(s.file).fileName()).arg(s.line),
				static_cast<int>(i));
		}
	}
	frameCombo->setCurrentIndex(frameCombo->count() ? defaultIndex : -1);
	frameCombo->blockSignals(false);

	frameSiteChanged(frameCombo->currentIndex());
}
//----------------------------------------------------------------------------
void ProfilerDialog_t::updateFrameControls(void)
{
	const int count = viewer->frameCount();
	const bool hasFrames = count > 0;

	frameSpin->blockSignals(true);
	frameSpin->setRange(hasFrames ? 1 : 0, count);
	if (!hasFrames)
	{
		frameSpin->setValue(0);
	}
	frameSpin->blockSignals(false);

	frameSpin->setEnabled(hasFrames);
	framePrevBtn->setEnabled(hasFrames);
	frameNextBtn->setEnabled(hasFrames);
	frameCombo->setEnabled(frameCombo->count() > 0);
	frameLabel->setText(hasFrames ? tr("%1 frames found").arg(count) : tr("No frames found"));
}
//----------------------------------------------------------------------------
void ProfilerDialog_t::frameSiteChanged(int comboIndex)
{
	const int site = (comboIndex >= 0) ? frameCombo->itemData(comboIndex).toInt() : -1;

	viewer->setFrameSite(site);
	updateFrameControls();
}
//----------------------------------------------------------------------------
void ProfilerDialog_t::frameSpinChanged(int value)
{
	if (value >= 1)
	{
		viewer->gotoFrame(value - 1);
		frameLabel->setText(viewer->frameDescription(value - 1));
	}
}
//----------------------------------------------------------------------------
void ProfilerDialog_t::viewerFrameSelected(int index)
{
	// Update the spin box without triggering gotoFrame, which would re-zoom the view
	frameSpin->blockSignals(true);
	frameSpin->setValue(index + 1);
	frameSpin->blockSignals(false);
	frameLabel->setText(viewer->frameDescription(index));
}
//----------------------------------------------------------------------------
void ProfilerDialog_t::framePrevClicked(void)
{
	frameSpin->setValue(frameSpin->value() - 1);
}
//----------------------------------------------------------------------------
void ProfilerDialog_t::frameNextClicked(void)
{
	frameSpin->setValue(frameSpin->value() + 1);
}
//----------------------------------------------------------------------------

