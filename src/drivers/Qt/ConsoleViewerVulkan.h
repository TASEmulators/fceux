#pragma once

#include <QColor>
#include <QWidget>
#include <atomic>

#include "Qt/ConsoleViewerInterface.h"

class ConsoleVulkanWindow_t;

class ConsoleViewVulkan_t : public QWidget, public ConsoleViewerBase
{
	Q_OBJECT

public:
	ConsoleViewVulkan_t(QWidget *parent = nullptr);
	~ConsoleViewVulkan_t() override;
	void shutdown(void);

	int init(void) override;
	void reset(void) override;
	void queueRedraw(void) override;
	int driver(void) override { return VIDEO_DRIVER_VULKAN; }
	void transfer2LocalBuffer(void) override;
	void setVsyncEnable(bool ena) override;
	void setLinearFilterEnable(bool ena) override;
	bool getForceAspectOpt(void) override { return forceAspect; }
	void setForceAspectOpt(bool val) override { forceAspect = val; }
	bool getAutoScaleOpt(void) override { return autoScaleEna; }
	void setAutoScaleOpt(bool val) override { autoScaleEna = val; }
	double getScaleX(void) override { return xscale; }
	double getScaleY(void) override { return yscale; }
	void setScaleXY(double xs, double ys) override;
	void getNormalizedCursorPos(double &x, double &y) override;
	bool getMouseButtonState(unsigned int btn) override;
	void setAspectXY(double x, double y) override;
	void getAspectXY(double &x, double &y) override;
	double getAspectRatio(void) override { return aspectRatio; }
	void setCursor(const QCursor &c) override;
	void setCursor(Qt::CursorShape s) override;
	void setBgColor(QColor &c) override;
	QSize size(void) override { return QWidget::size(); }
	QCursor cursor(void) override { return QWidget::cursor(); }
	void setMinimumSize(const QSize &s) override { QWidget::setMinimumSize(s); }
	void setMaximumSize(const QSize &s) override { QWidget::setMaximumSize(s); }

protected:
	void resizeEvent(QResizeEvent *event) override;

private:
	void calculateViewport(int frameWidth, int frameHeight, int &x, int &y, int &w, int &h);

	ConsoleVulkanWindow_t *vulkanWindow;
	QWidget *windowContainer;
	QColor *bgColor;
	uint32_t *localBuf;
	uint32_t localBufSize;
	double aspectRatio;
	double aspectX;
	double aspectY;
	double xscale;
	double yscale;
	int sx;
	int sy;
	int rw;
	int rh;
	bool forceAspect;
	bool autoScaleEna;
	bool linearFilter;
	bool vsyncEnabled;
	unsigned int mouseButtonMask;
	std::atomic_bool drawQueued;
};