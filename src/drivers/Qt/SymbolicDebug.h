// SymbolicDebug.h
//
#ifndef __SYMBOLIC_DEBUG_H__
#define __SYMBOLIC_DEBUG_H__

#include <string>
#include <list>
#include <map>

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

struct debugSymbol_t 
{
	int   ofs;
	std::string  name;
	std::string  comment;

	debugSymbol_t(void)
	{
		ofs = 0;
	};

	debugSymbol_t( int ofs, const char *name, const char *comment = NULL )
	{
		this->ofs = ofs;
		this->name.assign( name );

		if ( comment )
		{
			this->comment.assign( comment );
		}
	}

	void trimTrailingSpaces(void)
	{
		while ( name.size() > 0 )
		{
			if ( isspace( name.back() ) )
			{
				name.pop_back();
			}
			else
			{
				break;
			}
		}
		while ( comment.size() > 0 )
		{
			if ( isspace( comment.back() ) )
			{
				comment.pop_back();
			}
			else
			{
				break;
			}
		}
	}
};

struct debugSymbolPage_t
{
	int pageNum;

	debugSymbolPage_t(void);
	~debugSymbolPage_t(void);

	int  save(void);
	void print(void);
	int size(void){ return symMap.size(); }

	int addSymbol( debugSymbol_t *sym );

	int deleteSymbolAtOffset( int ofs );

	debugSymbol_t *getSymbolAtOffset( int ofs );


	std::map <int, debugSymbol_t*> symMap;
};

class debugSymbolTable_t
{

	public:
		debugSymbolTable_t(void);
		~debugSymbolTable_t(void);

		int loadFileNL( int addr );
		int loadGameSymbols(void);
		int numPages(void){ return pageMap.size(); }

		void save(void);
		void clear(void);
		void print(void);

		debugSymbol_t *getSymbolAtBankOffset( int bank, int ofs );

		int addSymbolAtBankOffset( int bank, int ofs, debugSymbol_t *sym );

		int deleteSymbolAtBankOffset( int bank, int ofs );

	private:
		std::map <int, debugSymbolPage_t*> pageMap;

		int loadRegisterMap(void);

};

extern  debugSymbolTable_t  debugSymbolTable;

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


int generateNLFilenameForBank(int bank, char *NLfilename);
int generateNLFilenameForAddress(int address, char *NLfilename);

#define  ASM_DEBUG_SYMS       0x0001
#define  ASM_DEBUG_REGS       0x0002
#define  ASM_DEBUG_REPLACE    0x0004
#define  ASM_DEBUG_ADDR_02X   0x0008

int DisassembleWithDebug(int addr, uint8_t *opcode, int flags, char *str, debugSymbol_t *symOut = NULL, debugSymbol_t *symOut2 = NULL );

#endif
