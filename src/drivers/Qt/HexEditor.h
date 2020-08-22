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
#include <QKeyEvent>

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
	int  numLines(void){ return _maxLines; }
	int  reAlloc( int newSize );
	void setAccessFunc( int (*newMemAccessFunc)( unsigned int offset) );

	struct memByte_t *buf;
	int  _size;
   int  _maxLines;
	int (*memAccessFunc)( unsigned int offset);
};

class QHexEdit : public QWidget
{
   Q_OBJECT

	public:
		QHexEdit(memBlock_t *blkPtr, QWidget *parent = 0);
		~QHexEdit(void);

		void setMode( int mode );
		void setLine( int newLineOffset );
		void setAddr( int newAddrOffset );
		void setScrollBars( QScrollBar *h, QScrollBar *v );
		void setHighlightActivity( int enable );
		void setHighlightReverseVideo( int enable );

		static const int HIGHLIGHT_ACTIVITY_NUM_COLORS = 16;
	protected:
		void paintEvent(QPaintEvent *event);
		void keyPressEvent(QKeyEvent *event);
   	void keyReleaseEvent(QKeyEvent *event);
		void mousePressEvent(QMouseEvent * event);
		void resizeEvent(QResizeEvent *event);

		void calcFontData(void);
		void resetCursor(void);
		QPoint convPixToCursor( QPoint p );

		QFont      font;

		memBlock_t *mb;

		QScrollBar *vbar;
		QScrollBar *hbar;
		QColor      highLightColor[ HIGHLIGHT_ACTIVITY_NUM_COLORS ];
		QColor      rvActvTextColor[ HIGHLIGHT_ACTIVITY_NUM_COLORS ];

      int viewMode;
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
		int viewLines;
		int viewWidth;
		int viewHeight;
      int maxLineOffset;
      int editAddr;
      int editValue;
      int editMask;

		bool cursorBlink;
		bool reverseVideo;
		bool actvHighlightEnable;
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
 
      int  getMode(void){ return mode; }
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
		void setViewRAM(void);
		void setViewPPU(void);
		void setViewOAM(void);
		void setViewROM(void);
		void actvHighlightCB(bool value); 
		void actvHighlightRVCB(bool value); 
};
