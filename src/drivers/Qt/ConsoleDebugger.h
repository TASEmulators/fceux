// ConsoleDebugger.h
//

#pragma once

#include <QWidget>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QPushButton>
#include <QFont>
#include <QLabel>
#include <QFrame>
#include <QGroupBox>
#include <QTreeView>
#include <QTreeWidget>
#include <QLineEdit>
#include <QTextEdit>

#include "Qt/main.h"

struct dbg_asm_entry_t
{
	int  addr;
	int  bank;
	int  rom;
	int  size;
	int  line;
	uint8  opcode[3];
	std::string  text;


	dbg_asm_entry_t(void)
	{
		addr = 0; bank = 0; rom = -1; 
		size = 0; line = 0;

		for (int i=0; i<3; i++)
		{
			opcode[i] = 0;
		}
	}
};

class ConsoleDebugger : public QDialog
{
   Q_OBJECT

	public:
		ConsoleDebugger(QWidget *parent = 0);
		~ConsoleDebugger(void);

		void updateWindowData(void);
		void updateAssemblyView(void);
		void asmClear(void);
		int  getAsmLineFromAddr(int addr);

	protected:
		void closeEvent(QCloseEvent *event);
		//void keyPressEvent(QKeyEvent *event);
   	//void keyReleaseEvent(QKeyEvent *event);

		//QTreeWidget *tree;
		QTextEdit *asmText;
		QTextEdit *stackText;
		QLineEdit *seekEntry;
		QLineEdit *pcEntry;
		QLineEdit *regAEntry;
		QLineEdit *regXEntry;
		QLineEdit *regYEntry;
		QLineEdit *cpuCycExdVal;
		QLineEdit *instrExdVal;
		QGroupBox *stackFrame;
		QGroupBox *bpFrame;
		QGroupBox *sfFrame;
		QTreeWidget * bpTree;
		QCheckBox *brkBadOpsCbox;
		QCheckBox *N_chkbox;
		QCheckBox *V_chkbox;
		QCheckBox *U_chkbox;
		QCheckBox *B_chkbox;
		QCheckBox *D_chkbox;
		QCheckBox *I_chkbox;
		QCheckBox *Z_chkbox;
		QCheckBox *C_chkbox;
		QCheckBox *brkCpuCycExd;
		QCheckBox *brkInstrsExd;
		QLabel    *ppuLbl;
		QLabel    *spriteLbl;
		QLabel    *scanLineLbl;
		QLabel    *pixLbl;
		QLabel    *cpuCyclesLbl;
		QLabel    *cpuInstrsLbl;
		QFont      font;

		dbg_asm_entry_t  *asmPC;
		std::vector <dbg_asm_entry_t*> asmEntry;

		char  displayROMoffsets;

	private:

   public slots:
      void closeWindow(void);
	private slots:

};
