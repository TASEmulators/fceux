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

struct memBlock_t
{
	memBlock_t(void);
	~memBlock_t(void);

	void init(void);
	int  size(void){ return _size; }
	int  reAlloc( int newSize );
	void setAccessFunc( int (*newMemAccessFunc)( unsigned int offset) );

	struct memByte_t *buf;
	int  _size;
	int (*memAccessFunc)( unsigned int offset);
};

class QHexEdit : public QWidget
{
   Q_OBJECT

	public:
		QHexEdit(memBlock_t *blkPtr, QWidget *parent = 0);
		~QHexEdit(void);

		void setLine( int newLineOffset );
		void setAddr( int newAddrOffset );
	protected:
		void paintEvent(QPaintEvent *event);
		void keyPressEvent(QKeyEvent *event);
   	void keyReleaseEvent(QKeyEvent *event);

		void calcFontData(void);

		QFont      font;

		memBlock_t *mb;

		int lineOffset;
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
		int numLines;
		int numCharsPerLine;

		bool cursorBlink;
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

		void initMem(void);
		void setMode(int new_mode);
		void showMemViewResults (bool reset);
		int  checkMemActivity(void);
		int  calcVisibleRange( int *start_out, int *end_out, int *center_out );

		QScrollBar *vbar;
		QScrollBar *hbar;
		QHexEdit   *editor;
		QTimer     *periodicTimer;

		int mode;
		//int memSize;
		//int mbuf_size;
		memBlock_t mb;
		int (*memAccessFunc)( unsigned int offset);

		uint64_t total_instructions_lp;

		bool redraw;

	private:

   public slots:
      void closeWindow(void);
	private slots:
		void updatePeriodic(void);
		void vbarMoved(int value);
		void vbarChanged(int value);

};
