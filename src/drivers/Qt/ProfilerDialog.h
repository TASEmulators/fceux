// ProfilerDialog.h
//

#pragma once


#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QComboBox>
#include <QSpinBox>

#ifdef __FCEU_PROFILER_ENABLE__
#include "Qt/ProfilerCapture.h"
#endif
#include "Qt/ProfilerCaptureFile.h"
#include "Qt/ProfilerViewer.h"

class ProfilerDialog_t : public QDialog
{
	Q_OBJECT

public:
	ProfilerDialog_t(QWidget *parent = nullptr);
	~ProfilerDialog_t(void);

protected:
	void closeEvent(QCloseEvent *event) override;
	void reject(void) override;

private:
	void startCapture(void);
	void stopCapture(void);
	void updateStatus(void);
	void loadCapture(const QString &path);
	void populateFrameCombo(void);
	void updateFrameControls(void);

	QLineEdit   *pathEdit;
	QPushButton *browseBtn;
	QPushButton *startStopBtn;
	QLabel      *statusLabel;
	QTimer      *updateTimer;

	QPushButton *openBtn;
	QLabel      *loadLabel;
	ProfilerViewer_t *viewer;
	QPushButton *zoomInBtn;
	QPushButton *zoomOutBtn;
	QPushButton *zoomFitBtn;
	QComboBox   *frameCombo;
	QSpinBox    *frameSpin;
	QPushButton *framePrevBtn;
	QPushButton *frameNextBtn;
	QLabel      *frameLabel;
	QLineEdit   *findEdit;
	QLabel      *findLabel;

#ifdef __FCEU_PROFILER_ENABLE__
	ProfilerCaptureThread_t *captureThread = nullptr;
#endif
	ProfilerLoadThread_t    *loadThread = nullptr;
	QString loadName;

private slots:
	void browseClicked(void);
	void startStopClicked(void);
	void captureFinished(void);
	void captureError(const QString &msg);
	void updatePeriodic(void);
	void closeWindow(void);
	void openClicked(void);
	void loadProgress(int percent);
	void loadFinished(void);
	void frameSiteChanged(int comboIndex);
	void frameSpinChanged(int value);
	void viewerFrameSelected(int index);
	void findTextChanged(const QString &text);
	void focusFind(void);
	void findNextOrFirst(void);
	void framePrevClicked(void);
	void frameNextClicked(void);
};


