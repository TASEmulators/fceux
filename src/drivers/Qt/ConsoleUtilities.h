// ConsoleUtilities.h

#pragma once

#include <string>
#include <type_traits>

#include <QColor>
#include <QTimer>
#include <QValidator>
#include <QDialog>
#include <QHelpEvent>
#include <QCheckBox>

int  getDirFromFile( const char *path, std::string &dir );

const char *getRomFile( void );

int getFileBaseName( const char *filepath, char *base, char *suffix = nullptr );

int parseFilepath( const char *filepath, std::string *dir, std::string *base = nullptr, std::string *suffix = nullptr );

const char *fceuExecutablePath(void);

int fceuLoadConfigColor( const char *confName, QColor *color );

class fceuDecIntValidtor : public QValidator
{ 
   public:
   	fceuDecIntValidtor( long long int min, long long int max, QObject *parent);

		QValidator::State validate(QString &input, int &pos) const;

		void  setMinMax( long long int min, long long int max );
	private:
		long long int  min;
		long long int  max;
};

class fceuHexIntValidtor : public QValidator
{ 
	public:
		fceuHexIntValidtor( long long int min, long long int max, QObject *parent);

		QValidator::State validate(QString &input, int &pos) const;

		void  setMinMax( long long int min, long long int max );
	private:
		long long int  min;
		long long int  max;
};

class fceuCustomToolTip : public QDialog
{
	Q_OBJECT

	public:
		fceuCustomToolTip( QWidget *parent = nullptr );
		~fceuCustomToolTip( void );

		void setHideOnMouseMove(bool);
	protected:
		bool eventFilter(QObject *obj, QEvent *event) override;
		void mouseMoveEvent( QMouseEvent *event ) override;

		void hideTip(void);
		void hideTipImmediately(void);
	private:
		QWidget *w;
		QTimer  *hideTimer;
		bool     hideOnMouseMode;

		static fceuCustomToolTip *instance;

	private slots:
		void  hideTimerExpired(void);
};

// Read Only Checkbox for state display only.
class QCheckBoxRO : public QCheckBox
{
	Q_OBJECT

	public:
		QCheckBoxRO( const QString &text, QWidget *parent = nullptr );
		QCheckBoxRO( QWidget *parent = nullptr );

	protected:
		void mousePressEvent( QMouseEvent *event ) override;
		void mouseReleaseEvent( QMouseEvent *event ) override;

};

QString fceuGetOpcodeToolTip( uint8_t *opcode, int size );

QDialog *fceuCustomToolTipShow( const QPoint &globalPos, QDialog *popup );

// Faster replacement functions for sprintf
class StringBuilder
{
public:
	inline StringBuilder(char *str) : start(str), end(str)
	{
		*end = '\0';
	}

	inline char *str() const
	{
		return start;
	}

	inline size_t size() const
	{
		return size_t(end - start);
	}

	template <class T>
	inline StringBuilder &operator << (T value)
	{
		return append(value);
	}

	inline StringBuilder &append(char ch)
	{
		*(end++) = ch;
		*end = '\0';

		return *this;
	}

	inline StringBuilder &append(const char *src)
	{
		size_t len = strlen(src);
		memcpy(end, src, len + 1);

		end += len;

		return *this;
	}

	template <unsigned Radix, class T>
	StringBuilder &appendInt(T x, int minLen = 0, char leadChar = ' ', bool upperCase = false)
	{
		static_assert(Radix >= 2 && Radix <= 36, "Radix must be between 2 and 36");
		static_assert(std::is_integral<T>::value, "T must be an integral type");

		static const char *upperCaseDigits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ",
			*lowerCaseDigits = "0123456789abcdefghijklmnopqrstuvwxyz";
		const char *digits = upperCase ? upperCaseDigits : lowerCaseDigits;

		const bool isSigned = std::is_signed<T>::value;
		bool isNeg = isSigned && x < 0;
		if (isSigned && isNeg)
			// This convoluted expression is to silence an unnecessary compiler warning when unsigned
			x = T(-std::make_signed<T>::type(x));

		unsigned i = 0;
		if (x != 0)
		{
			do
			{
				end[i++] = digits[x % Radix];
				x /= Radix;
			} while (x != 0);
		}
		else
			end[i++] = '0';

		if (isNeg)
			end[i++] = '-';

		if (minLen < 0)
		{
			end[i] = '\0';

			strrev(end);

			for (; i < (unsigned)-minLen; i++)
				end[i] = leadChar;

			end[i] = '\0';
		}
		else
		{
			for (; i < (unsigned)minLen; i++)
				end[i] = leadChar;

			end[i] = '\0';

			strrev(end);
		}

		end += i;

		return *this;
	}

	template <class T>
	inline StringBuilder &appendDec(T x, int minLen = 0, char leadChar = ' ')
	{
		appendInt<10>(x, minLen, leadChar);

		return *this;
	}

	template <class T>
	inline StringBuilder &appendHex(T x, int minLen = 0, bool upperCase = true, char leadChar = '0')
	{
		appendInt<16>(x, minLen, leadChar, upperCase);

		return *this;
	}

	template <class T>
	inline StringBuilder &appendAddr(T addr, int minLen = 4, bool upperCase = true)
	{
		*(end++) = '$';
		return appendHex(addr, minLen, upperCase);
	}

	template <class T>
	inline StringBuilder &appendLit(T x, int minLen = 2, bool upperCase = true)
	{
		*(end++) = '#';
		*(end++) = '$';
		return appendHex(x, minLen, upperCase);
	}

protected:
	char *start;
	char *end;
};