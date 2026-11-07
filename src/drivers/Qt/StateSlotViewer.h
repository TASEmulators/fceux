// StateSlotViewer.h
//
// Dockable widget that shows the selected save state slot and its
// neighbors along with a thumbnail of the screen stored in each state.

#pragma once

#include <string>
#include <vector>

#include <QDockWidget>
#include <QWidget>
#include <QImage>
#include <QString>
#include <QColor>
#include <QBoxLayout>
#include <QTimer>

class StateSlotThumb_t : public QWidget
{
	Q_OBJECT

public:
	StateSlotThumb_t(QWidget *parent = 0);

	QSize sizeHint(void) const override;
	QSize minimumSizeHint(void) const override;

	// Absolute slot id (bank * 10 + slot), valid is false for positions past either end of the slot range
	int  slotId = -1;
	bool valid = false;
	bool selected = false;
	bool exists = false;
	bool hasImage = false;
	QImage image;

	// Background of the cell, follows the video viewport background
	QColor bgColor = Qt::black;

	// Identity of the file the image was loaded from, used to detect changes
	std::string path;
	qint64 fileSize = -1;
	qint64 fileTime = -1;

signals:
	void selectRequested(int slotId);
	void loadRequested(int slotId);
	void saveRequested(int slotId);

protected:
	void paintEvent(QPaintEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseDoubleClickEvent(QMouseEvent *event) override;
	void contextMenuEvent(QContextMenuEvent *event) override;
};

class StateSlotDock_t : public QDockWidget
{
	Q_OBJECT

public:
	StateSlotDock_t(QWidget *parent = 0);
	~StateSlotDock_t(void);

	// Upper limit on the number of slots shown. The number actually shown is odd and
	// depends on the space available, with the selected slot in the center.
	static constexpr int MAX_THUMBS = 25;

signals:
	void selectRequested(int slotId);
	void loadRequested(int slotId);
	void saveRequested(int slotId);

protected:
	void showEvent(QShowEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;

private:
	bool loadThumbImage(StateSlotThumb_t *thumb, const std::string &path);

	void updateVisibleCount(void);
	void applyBgColor(const QColor &c);

	std::vector<StateSlotThumb_t*> thumbs;
	int visibleCount = 3;
	QBoxLayout *thumbLayout;
	QWidget    *container;
	QColor      bgColor = Qt::black;
	QTimer     *pollTimer;

private slots:
	void refresh(void);
	void dockAreaChanged(Qt::DockWidgetArea area);
};
