// SymbolicDebug.h
//
#ifndef __SYMBOLIC_DEBUG_H__
#define __SYMBOLIC_DEBUG_H__

#include <string>

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
#include <QTimer>
#include <QFont>
#include <QFrame>
#include <QGroupBox>
#include <QLineEdit>
#include <QPlainTextEdit>

#include "../../debugsymboltable.h"


class SymbolEditWindow : public QDialog
{
   Q_OBJECT

	public:
		SymbolEditWindow(QWidget *parent = 0);
		~SymbolEditWindow(void);

		void setAddr( int addrIn );
		void setBank( int bankIn );
		void setSym( debugSymbol_t *symIn );

	protected:
		void closeEvent(QCloseEvent *event);

		void updateArraySensitivity(void);
		void setSymNameWithArray(int idx = 0);
		void determineArrayStart(void);

		QFont      font;
		QLineEdit *filepath;
		QLineEdit *addrEntry;
		QLineEdit *nameEntry;
		QPlainTextEdit *commentEntry;
		QLineEdit      *arraySize;
		QLineEdit      *arrayInit;
		QCheckBox      *isArrayBox;
		QCheckBox      *arrayNameOverWrite;
		QCheckBox      *arrayCommentOverWrite;
		QCheckBox      *commentHeadOnly;
		QCheckBox      *deleteBox;
		QLabel         *arraySizeLbl[2];
		QLabel         *arrayInitLbl;

		QPushButton *okButton;
		QPushButton *cancelButton;

		debugSymbol_t *sym;

		int  addr;
		int  bank;
		int  charWidth;

	public slots:
		void closeWindow(void);
		int  exec(void);

	private slots:
		void isArrayStateChanged( int state );
		void arrayCommentHeadOnlyChanged( int state );

};


int generateNLFilenameForBank(int bank, std::string &NLfilename);
int generateNLFilenameForAddress(int address, std::string &NLfilename);

#define  ASM_DEBUG_SYMS       0x0001
#define  ASM_DEBUG_REGS       0x0002
#define  ASM_DEBUG_REPLACE    0x0004
#define  ASM_DEBUG_ADDR_02X   0x0008
#define  ASM_DEBUG_TRACES     0x0010

int DisassembleWithDebug(int addr, uint8_t *opcode, int flags, char *str, debugSymbol_t *symOut = NULL, debugSymbol_t *symOut2 = NULL );

#endif
