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

// Faster replacement functions for sprintf. It's uglier, but much faster.
class StringBuilder
{
public:
	// Helper struct. Do not use directly.
	template <unsigned Radix, typename T, typename Prefix = nullptr_t>
	struct IntInfo
	{
		T x;
		int minLen;
		char leadChar;
		bool upperCase;
		Prefix prefix;
	};

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

	inline StringBuilder &operator <<(char ch)
	{
		*(end++) = ch;
		*end = '\0';

		return *this;
	}

	inline StringBuilder &operator <<(const char *src)
	{
		size_t len = strlen(src);
		memcpy(end, src, len + 1);

		end += len;

		return *this;
	}

	template<unsigned Radix, typename T, typename Prefix>
	inline StringBuilder &operator <<(const IntInfo<Radix, T, Prefix> intInfo)
	{
		*this << intInfo.prefix;
		return appendInt<Radix>(intInfo.x, intInfo.minLen, intInfo.leadChar, intInfo.upperCase);
	}

protected:
	inline StringBuilder &operator <<(nullptr_t)
	{
		return *this;
	}

	template<unsigned Radix, typename T>
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

	char *start;
	char *end;
};

// Formatters for numbers

// Generic integer with any radius
template<unsigned Radix, typename T>
inline StringBuilder::IntInfo<Radix, T> sb_int(T x, int minLen = 0, char leadChar = ' ', bool upperCase = false)
{
	return { x, minLen, leadChar, upperCase, nullptr };
}

// A decimal number
template <typename T>
inline StringBuilder::IntInfo<10, T> sb_dec(T x, int minLen = 0, char leadChar = ' ')
{
	return { x, minLen, leadChar, false, nullptr };
}

// A hex number
template <typename T>
inline StringBuilder::IntInfo<16, T> sb_hex(T x, int minLen = 0, bool upperCase = true, char leadChar = '0')
{
	return { x, minLen, leadChar, upperCase, nullptr };
}

// An address of the basic form $%x
template <typename T>
inline StringBuilder::IntInfo<16, T, char> sb_addr(T x, int minLen = 4, bool upperCase = true)
{
	return { x, minLen, '0', upperCase, '$' };
}

// A literal value of the form #$%x
template <typename T>
inline StringBuilder::IntInfo<16, T, const char *> sb_lit(T x, int minLen = 2, bool upperCase = true)
{
	return { x, minLen, '0', upperCase, "#$" };
}
