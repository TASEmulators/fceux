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

		QLineEdit   *custom_palette_path;
		QCheckBox   *useCustom;
		QCheckBox   *GrayScale;
		QCheckBox   *deemphSwap;
		QCheckBox   *useNTSC;
		QSlider     *tintSlider;
		QSlider     *hueSlider;
		QGroupBox   *tintFrame;
		QGroupBox   *hueFrame;
	private:

   public slots:
      void closeWindow(void);
	private slots:
		void hueChanged(int value);
		void tintChanged(int value);
		void openPaletteFile(void);
		void clearPalette(void);
		void use_NTSC_Changed(int v);
		void use_Custom_Changed(int v);
		void force_GrayScale_Changed(int v);
		void deemphswap_Changed(int v);

};
