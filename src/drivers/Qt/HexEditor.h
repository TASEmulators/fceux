// GamePadConf.h
//

#pragma once

#include <QWidget>
#include <QDialog>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QGroupBox>
#include <QBuffer>
#include <QByteArray>

#include "Qt/QHexEdit.h"

struct memByte_t
{
	unsigned char data;
	unsigned char color;
	unsigned char actv;
	unsigned char draw;
};

class HexEditorDialog_t : public QDialog
{
   Q_OBJECT

	public:
		HexEditorDialog_t(QWidget *parent = 0);
		~HexEditorDialog_t(void);

		enum {
			MODE_NES_RAM = 0,
			MODE_NES_PPU,
			MODE_NES_OAM,
			MODE_NES_ROM
		};
	protected:
		void keyPressEvent(QKeyEvent *event);
   	void keyReleaseEvent(QKeyEvent *event);

		void initMem(void);
		void setMode(int new_mode);
		void showMemViewResults (bool reset);
		int  checkMemActivity(void);
		int  calcVisibleRange( int *start_out, int *end_out, int *center_out );

		QFont     font;
		QHexEdit *editor;
		QTimer   *periodicTimer;
		QByteArray *dataArray;
		QBuffer    *dataBuffer;

		int mode;
		int numLines;
		int numCharsPerLine;
		int memSize;
		int mbuf_size;
		int (*memAccessFunc)( unsigned int offset);
		struct memByte_t *mbuf;

		uint64_t total_instructions_lp;

		bool redraw;

	private:

   public slots:
      void closeWindow(void);
	private slots:
		void updatePeriodic(void);

};
