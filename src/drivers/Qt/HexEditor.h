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
#include <QPlainTextEdit>

struct memByte_t
{
	unsigned char data;
	unsigned char color;
	unsigned char actv;
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
		void paintEvent(QPaintEvent *event);
		void keyPressEvent(QKeyEvent *event);
   	void keyReleaseEvent(QKeyEvent *event);

		void calcFontData(void);
		void initMem(void);
		void setMode(int new_mode);
		void showMemViewResults (bool reset);
		int  checkMemActivity(void);
		int  calcVisibleRange( int *start_out, int *end_out, int *center_out );

		QFont      font;
		QWidget   *editor;
		QTimer    *periodicTimer;
		//QByteArray *dataArray;
		//QBuffer    *dataBuffer;
		//
		int pxCharWidth;
		int pxCharHeight;
		int pxCursorHeight;
		int pxLineSpacing;
		int pxLineLead;
		int pxXoffset;
		int pxYoffset;
		int pxHexOffset;
		int pxHexAscii;

		int cursorPosX;
		int cursorPosY;
		int cursorBlinkCount;
		int mode;
		int numLines;
		int numCharsPerLine;
		int memSize;
		int mbuf_size;
		int (*memAccessFunc)( unsigned int offset);
		struct memByte_t *mbuf;

		uint64_t total_instructions_lp;

		bool redraw;
		bool cursorBlink;

	private:

   public slots:
      void closeWindow(void);
	private slots:
		void updatePeriodic(void);

};
