// PaletteConf.h
//

#pragma once

#include <QWidget>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QGroupBox>
#include <QLineEdit>

#include "Qt/main.h"

class PaletteConfDialog_t : public QDialog
{
	Q_OBJECT

public:
	PaletteConfDialog_t(QWidget *parent = 0);
	~PaletteConfDialog_t(void);

protected:
	void closeEvent(QCloseEvent *event);

	QLineEdit *custom_palette_path;
	QCheckBox *useCustom;
	QCheckBox *GrayScale;
	QCheckBox *deemphSwap;
	QPushButton *palReset;
	QSlider *tintSlider;
	QSlider *hueSlider;
	QSlider *notchSlider;
	QSlider *saturationSlider;
	QSlider *sharpnessSlider;
	QSlider *contrastSlider;
	QSlider *brightnessSlider;
	QGroupBox *tintFrame;
	QGroupBox *hueFrame;
	QGroupBox *ntscFrame;
	QGroupBox *palFrame;
	QGroupBox *notchFrame;
	QGroupBox *saturationFrame;
	QGroupBox *sharpnessFrame;
	QGroupBox *contrastFrame;
	QGroupBox *brightnessFrame;

private:
public slots:
	void closeWindow(void);
private slots:
	void hueChanged(int value);
	void tintChanged(int value);
	void openPaletteFile(void);
	void clearPalette(void);
	void use_NTSC_Changed(bool v);
	void use_Custom_Changed(int v);
	void force_GrayScale_Changed(int v);
	void deemphswap_Changed(int v);
	void palResetClicked(void);
	void palNotchChanged(int value);
	void palSaturationChanged(int value);
	void palSharpnessChanged(int value);
	void palContrastChanged(int value);
	void palBrightnessChanged(int value);
};
