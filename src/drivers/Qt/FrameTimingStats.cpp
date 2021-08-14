/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2020 mjbudd77
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
// FrameTimingStats.cpp
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include <SDL.h>
#include <QHeaderView>
#include <QCloseEvent>
#include <QSettings>

#include "Qt/main.h"
#include "Qt/dface.h"
#include "Qt/input.h"
#include "Qt/config.h"
#include "Qt/keyscan.h"
#include "Qt/throttle.h"
#include "Qt/fceuWrapper.h"
#include "Qt/FrameTimingStats.h"

//----------------------------------------------------------------------------
FrameTimingDialog_t::FrameTimingDialog_t(QWidget *parent)
	: QDialog(parent)
{
	QVBoxLayout *mainLayout, *vbox;
	QHBoxLayout *hbox;
	QTreeWidgetItem *item;
	QPushButton *resetBtn, *closeButton;
	struct frameTimingStat_t stats;
	QSettings settings;

	getFrameTimingStats(&stats);

	setWindowTitle("Frame Timing Statistics");

	resize(512, 512);

	mainLayout = new QVBoxLayout();
	vbox = new QVBoxLayout();
	statFrame = new QGroupBox(tr("Timing Statistics"));
	statFrame->setLayout(vbox);

	tree = new QTreeWidget();
	vbox->addWidget(tree);

	tree->setColumnCount(4);

	item = new QTreeWidgetItem();
	item->setText(0, tr("Parameter"));
	item->setText(1, tr("Target"));
	item->setText(2, tr("Current"));
	item->setText(3, tr("Minimum"));
	item->setText(4, tr("Maximum"));
	item->setTextAlignment(0, Qt::AlignLeft);
	item->setTextAlignment(1, Qt::AlignCenter);
	item->setTextAlignment(2, Qt::AlignCenter);
	item->setTextAlignment(3, Qt::AlignCenter);
	item->setTextAlignment(4, Qt::AlignCenter);

	tree->setHeaderItem(item);
	tree->header()->setSectionResizeMode(QHeaderView::Stretch);
	tree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);

	frameTimeAbs = new QTreeWidgetItem();
	frameTimeDel = new QTreeWidgetItem();
	frameTimeWork = new QTreeWidgetItem();
	frameTimeIdle = new QTreeWidgetItem();
	frameTimeWorkPct = new QTreeWidgetItem();
	frameTimeIdlePct = new QTreeWidgetItem();
	frameLateCount = new QTreeWidgetItem();
	videoTimeAbs = new QTreeWidgetItem();

	tree->addTopLevelItem(frameTimeAbs);
	tree->addTopLevelItem(frameTimeDel);
	tree->addTopLevelItem(frameTimeWork);
	tree->addTopLevelItem(frameTimeIdle);
	tree->addTopLevelItem(frameTimeWorkPct);
	tree->addTopLevelItem(frameTimeIdlePct);
	tree->addTopLevelItem(videoTimeAbs);
	tree->addTopLevelItem(frameLateCount);

	frameTimeAbs->setFlags(Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);
	frameTimeDel->setFlags(Qt::ItemIsEnabled | Qt::ItemNeverHasChildren);

	frameTimeAbs->setText(0, tr("Frame Period ms"));
	frameTimeDel->setText(0, tr("Frame Delta ms"));
	frameTimeWork->setText(0, tr("Frame Work ms"));
	frameTimeIdle->setText(0, tr("Frame Idle ms"));
	frameTimeWorkPct->setText(0, tr("Frame Work %"));
	frameTimeIdlePct->setText(0, tr("Frame Idle %"));
	frameLateCount->setText(0, tr("Frame Late Count"));
	videoTimeAbs->setText(0, tr("Video Period ms"));

	frameTimeAbs->setTextAlignment(0, Qt::AlignLeft);
	frameTimeDel->setTextAlignment(0, Qt::AlignLeft);
	frameTimeWork->setTextAlignment(0, Qt::AlignLeft);
	frameTimeIdle->setTextAlignment(0, Qt::AlignLeft);
	frameTimeWorkPct->setTextAlignment(0, Qt::AlignLeft);
	frameTimeIdlePct->setTextAlignment(0, Qt::AlignLeft);
	frameLateCount->setTextAlignment(0, Qt::AlignLeft);
	videoTimeAbs->setTextAlignment(0, Qt::AlignLeft);

	for (int i = 0; i < 4; i++)
	{
		frameTimeAbs->setTextAlignment(i + 1, Qt::AlignCenter);
		frameTimeDel->setTextAlignment(i + 1, Qt::AlignCenter);
		frameTimeWork->setTextAlignment(i + 1, Qt::AlignCenter);
		frameTimeIdle->setTextAlignment(i + 1, Qt::AlignCenter);
		frameTimeWorkPct->setTextAlignment(i + 1, Qt::AlignCenter);
		frameTimeIdlePct->setTextAlignment(i + 1, Qt::AlignCenter);
		frameLateCount->setTextAlignment(i + 1, Qt::AlignCenter);
		videoTimeAbs->setTextAlignment(i + 1, Qt::AlignCenter);
	}

	hbox = new QHBoxLayout();
	timingEnable = new QCheckBox(tr("Enable Timing Statistics Calculations"));
	resetBtn = new QPushButton(tr("Reset"));
	resetBtn->setIcon(style()->standardIcon(QStyle::SP_DialogResetButton));

	timingEnable->setChecked(stats.enabled);
	statFrame->setEnabled(stats.enabled);

	hbox->addWidget(timingEnable);
	hbox->addWidget(resetBtn);

	connect(timingEnable, SIGNAL(stateChanged(int)), this, SLOT(timingEnableChanged(int)));
	connect(resetBtn, SIGNAL(clicked(void)), this, SLOT(resetTimingClicked(void)));

	mainLayout->addLayout(hbox);
	mainLayout->addWidget(statFrame);

	closeButton = new QPushButton( tr("Close") );
	closeButton->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
	connect(closeButton, SIGNAL(clicked(void)), this, SLOT(closeWindow(void)));

	hbox = new QHBoxLayout();
	hbox->addStretch(5);
	hbox->addWidget( closeButton, 1 );
	mainLayout->addLayout( hbox );

	setLayout(mainLayout);

	updateTimingStats();

	updateTimer = new QTimer(this);

	connect(updateTimer, &QTimer::timeout, this, &FrameTimingDialog_t::updatePeriodic);

	updateTimer->start(200); // 5hz

	restoreGeometry(settings.value("frameTimingWindow/geometry").toByteArray());
}
//----------------------------------------------------------------------------
FrameTimingDialog_t::~FrameTimingDialog_t(void)
{
	QSettings settings;

	//printf("Destroy Frame Timing Window\n");
	updateTimer->stop();

	settings.setValue("frameTimingWindow/geometry", saveGeometry());
}
//----------------------------------------------------------------------------
void FrameTimingDialog_t::closeEvent(QCloseEvent *event)
{
	//printf("Frame Timing Close Window Event\n");
	done(0);
	deleteLater();
	event->accept();
}
//----------------------------------------------------------------------------
void FrameTimingDialog_t::closeWindow(void)
{
	//printf("Close Window\n");
	done(0);
	deleteLater();
}
//----------------------------------------------------------------------------
void FrameTimingDialog_t::updateTimingStats(void)
{
	char stmp[128];
	struct frameTimingStat_t stats;

	getFrameTimingStats(&stats);

	// Absolute
	sprintf(stmp, "%.3f", stats.frameTimeAbs.tgt * 1e3);
	frameTimeAbs->setText(1, tr(stmp));

	sprintf(stmp, "%.3f", stats.frameTimeAbs.cur * 1e3);
	frameTimeAbs->setText(2, tr(stmp));

	sprintf(stmp, "%.3f", stats.frameTimeAbs.min * 1e3);
	frameTimeAbs->setText(3, tr(stmp));

	sprintf(stmp, "%.3f", stats.frameTimeAbs.max * 1e3);
	frameTimeAbs->setText(4, tr(stmp));

	// Delta
	sprintf(stmp, "%.3f", stats.frameTimeDel.tgt * 1e3);
	frameTimeDel->setText(1, tr(stmp));

	sprintf(stmp, "%.3f", stats.frameTimeDel.cur * 1e3);
	frameTimeDel->setText(2, tr(stmp));

	sprintf(stmp, "%.3f", stats.frameTimeDel.min * 1e3);
	frameTimeDel->setText(3, tr(stmp));

	sprintf(stmp, "%.3f", stats.frameTimeDel.max * 1e3);
	frameTimeDel->setText(4, tr(stmp));

	// Work
	sprintf(stmp, "lt %.3f", stats.frameTimeWork.tgt * 1e3);
	frameTimeWork->setText(1, tr(stmp));

	sprintf(stmp, "%.3f", stats.frameTimeWork.cur * 1e3);
	frameTimeWork->setText(2, tr(stmp));

	sprintf(stmp, "%.3f", stats.frameTimeWork.min * 1e3);
	frameTimeWork->setText(3, tr(stmp));

	sprintf(stmp, "%.3f", stats.frameTimeWork.max * 1e3);
	frameTimeWork->setText(4, tr(stmp));

	// Idle
	sprintf(stmp, "gt %.3f", stats.frameTimeIdle.tgt * 1e3);
	frameTimeIdle->setText(1, tr(stmp));

	sprintf(stmp, "%.3f", stats.frameTimeIdle.cur * 1e3);
	frameTimeIdle->setText(2, tr(stmp));

	sprintf(stmp, "%.3f", stats.frameTimeIdle.min * 1e3);
	frameTimeIdle->setText(3, tr(stmp));

	sprintf(stmp, "%.3f", stats.frameTimeIdle.max * 1e3);
	frameTimeIdle->setText(4, tr(stmp));

	// Work %
	sprintf(stmp, "lt %.1f", 100.0 * stats.frameTimeWork.tgt / stats.frameTimeAbs.tgt);
	frameTimeWorkPct->setText(1, tr(stmp));

	sprintf(stmp, "%.1f", 100.0 * stats.frameTimeWork.cur / stats.frameTimeAbs.tgt);
	frameTimeWorkPct->setText(2, tr(stmp));

	sprintf(stmp, "%.1f", 100.0 * stats.frameTimeWork.min / stats.frameTimeAbs.tgt);
	frameTimeWorkPct->setText(3, tr(stmp));

	sprintf(stmp, "%.1f", 100.0 * stats.frameTimeWork.max / stats.frameTimeAbs.tgt);
	frameTimeWorkPct->setText(4, tr(stmp));

	// Idle %
	sprintf(stmp, "gt %.1f", 100.0 * stats.frameTimeIdle.tgt / stats.frameTimeAbs.tgt);
	frameTimeIdlePct->setText(1, tr(stmp));

	sprintf(stmp, "%.1f", 100.0 * stats.frameTimeIdle.cur / stats.frameTimeAbs.tgt);
	frameTimeIdlePct->setText(2, tr(stmp));

	sprintf(stmp, "%.1f", 100.0 * stats.frameTimeIdle.min / stats.frameTimeAbs.tgt);
	frameTimeIdlePct->setText(3, tr(stmp));

	sprintf(stmp, "%.1f", 100.0 * stats.frameTimeIdle.max / stats.frameTimeAbs.tgt);
	frameTimeIdlePct->setText(4, tr(stmp));

	// Video
	sprintf(stmp, "%.3f", stats.videoTimeDel.tgt * 1e3);
	videoTimeAbs->setText(1, tr(stmp));

	sprintf(stmp, "%.3f", stats.videoTimeDel.cur * 1e3);
	videoTimeAbs->setText(2, tr(stmp));

	sprintf(stmp, "%.3f", stats.videoTimeDel.min * 1e3);
	videoTimeAbs->setText(3, tr(stmp));

	sprintf(stmp, "%.3f", stats.videoTimeDel.max * 1e3);
	videoTimeAbs->setText(4, tr(stmp));

	// Late Count
	sprintf(stmp, "%u", stats.lateCount);
	frameLateCount->setText(1, tr("0"));
	frameLateCount->setText(2, tr(stmp));

	statFrame->setEnabled(stats.enabled);

	tree->viewport()->update();
}
//----------------------------------------------------------------------------
void FrameTimingDialog_t::updatePeriodic(void)
{
	updateTimingStats();
}
//----------------------------------------------------------------------------
void FrameTimingDialog_t::timingEnableChanged(int state)
{
	setFrameTimingEnable(state != Qt::Unchecked);
}
//----------------------------------------------------------------------------
void FrameTimingDialog_t::resetTimingClicked(void)
{
	resetFrameTiming();
}
//----------------------------------------------------------------------------
