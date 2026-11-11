// RetroAchievementsToast.cpp
//
#ifdef FCEUX_HAS_RETRO_ACHIEVEMENTS

#include <QFontMetrics>
#include <QPainter>
#include <QPropertyAnimation>
#include <QTimer>

#include "Qt/AudioDriverInterface.h"
#include "Qt/config.h"
#include "Qt/fceuWrapper.h"
#include "Qt/RetroAchievements/RetroAchievements.h"
#include "Qt/RetroAchievements/RetroAchievementsImages.h"
#include "Qt/RetroAchievements/RetroAchievementsToast.h"

static const int toastWidth = 380;
static const int toastHeight = 88;
static const int toastMargin = 16;
static const int toastShowMs = 5000;
static const int toastFadeInMs = 200;
static const int toastFadeOutMs = 400;
static const int toastMaxQueued = 8;

//----------------------------------------------------------------------------
// RaToastWidget
//----------------------------------------------------------------------------
RaToastWidget::RaToastWidget(QWidget *parent, const QString &titleText, const QString &bodyText, bool highlightBorder)
	: QWidget(parent, Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus)
	, title(titleText)
	, text(bodyText)
	, highlight(highlightBorder)
{
	setAttribute(Qt::WA_ShowWithoutActivating);
	setAttribute(Qt::WA_TranslucentBackground);
	setAttribute(Qt::WA_TransparentForMouseEvents);
	setFixedSize(toastWidth, toastHeight);
}
//----------------------------------------------------------------------------
void RaToastWidget::setImage(const QPixmap &pixmap)
{
	image = pixmap;
	update();
}
//----------------------------------------------------------------------------
void RaToastWidget::paintEvent(QPaintEvent *)
{
	QPainter p(this);
	const QRect box = rect().adjusted(1, 1, -1, -1);
	const QRect imageRect(12, (toastHeight - 64) / 2, 64, 64);
	const int textLeft = imageRect.right() + 12;
	QFont titleFont = font();
	QFont textFont = font();

	p.setRenderHint(QPainter::Antialiasing);
	p.setRenderHint(QPainter::SmoothPixmapTransform);

	p.setPen(QPen(highlight ? QColor(255, 200, 0) : QColor(120, 120, 130), 2));
	p.setBrush(QColor(24, 24, 28, 235));
	p.drawRoundedRect(box, 10, 10);

	if (!image.isNull())
	{
		p.drawPixmap(imageRect, image);
	}
	titleFont.setBold(true);
	titleFont.setPointSizeF(titleFont.pointSizeF() + 1.0);

	const QRect titleRect(textLeft, 10, toastWidth - textLeft - 12, 24);
	const QRect textRect(textLeft, 36, toastWidth - textLeft - 12, toastHeight - 36 - 10);

	p.setFont(titleFont);
	p.setPen(Qt::white);
	p.drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter,
		QFontMetrics(titleFont).elidedText(title, Qt::ElideRight, titleRect.width()));

	p.setFont(textFont);
	p.setPen(QColor(200, 200, 205));
	p.drawText(textRect, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, text);
}
//----------------------------------------------------------------------------
// RaToastManager
//----------------------------------------------------------------------------
RaToastManager::RaToastManager(QWidget *mainWindow)
	: QObject(mainWindow)
	, window(mainWindow)
{
	RetroAchievements *ra = RetroAchievements::instance();

	if (ra == nullptr)
	{
		return;
	}
	// Always queued: these can be emitted from the GUI thread while it holds the emulator lock,
	// and creating windows there would stall emulation.
	connect(ra, &RetroAchievements::achievementUnlocked, this, &RaToastManager::achievementUnlocked, Qt::QueuedConnection);
	connect(ra, &RetroAchievements::gameCompleted, this, &RaToastManager::gameCompleted, Qt::QueuedConnection);
	connect(ra, &RetroAchievements::notification, this, &RaToastManager::notification, Qt::QueuedConnection);
	connect(ra->images(), &RaImageCache::imageReady, this, &RaToastManager::imageReady);
}
//----------------------------------------------------------------------------
RaToastManager::~RaToastManager(void)
{
}
//----------------------------------------------------------------------------
void RaToastManager::achievementUnlocked(const QString &title, const QString &description,
	unsigned int points, const QString &badgeUrl)
{
	Toast toast;
	bool playSound = true;

	toast.title = tr("Achievement Unlocked: %1").arg(title);
	toast.text = tr("%1 (%2 points)").arg(description).arg(points);
	toast.imageUrl = badgeUrl;
	toast.highlight = true;

	enqueue(toast);

	g_config->getOption("SDL.RetroAchSound", &playSound);

	if (playSound)
	{
		AudioDriverBase::playChime();
	}
}
//----------------------------------------------------------------------------
void RaToastManager::gameCompleted(const QString &title)
{
	Toast toast;

	toast.title = tr("Game Completed");
	toast.text = tr("All achievements for %1 have been earned.").arg(title);
	toast.highlight = true;

	enqueue(toast);
}
//----------------------------------------------------------------------------
void RaToastManager::notification(const QString &title, const QString &text, const QString &imageUrl)
{
	Toast toast;

	toast.title = title;
	toast.text = text;
	toast.imageUrl = imageUrl;

	enqueue(toast);
}
//----------------------------------------------------------------------------
void RaToastManager::enqueue(const Toast &toast)
{
	if (queue.size() < toastMaxQueued)
	{
		queue.enqueue(toast);
	}
	if (current.isNull())
	{
		showNext();
	}
}
//----------------------------------------------------------------------------
void RaToastManager::showNext(void)
{
	if (queue.isEmpty())
	{
		return;
	}
	const Toast toast = queue.dequeue();
	RaToastWidget *widget = new RaToastWidget(window, toast.title, toast.text, toast.highlight);
	RetroAchievements *ra = RetroAchievements::instance();

	current = widget;
	currentImageUrl = toast.imageUrl;

	if (ra != nullptr)
	{
		// Messages without a badge of their own get the RetroAchievements icon.
		widget->setImage(toast.imageUrl.isEmpty() ? QPixmap(":/retroachievements/ra-icon.png")
			: ra->images()->get(toast.imageUrl));
	}
	widget->move(window->mapToGlobal(QPoint(window->width() - toastWidth - toastMargin, 40)));
	widget->setWindowOpacity(0.0);
	widget->show();

	QPropertyAnimation *fadeIn = new QPropertyAnimation(widget, "windowOpacity", widget);

	fadeIn->setDuration(toastFadeInMs);
	fadeIn->setStartValue(0.0);
	fadeIn->setEndValue(1.0);
	fadeIn->start(QAbstractAnimation::DeleteWhenStopped);

	QTimer::singleShot(toastShowMs, widget, [this, widget]()
	{
		QPropertyAnimation *fadeOut = new QPropertyAnimation(widget, "windowOpacity", widget);

		fadeOut->setDuration(toastFadeOutMs);
		fadeOut->setStartValue(1.0);
		fadeOut->setEndValue(0.0);

		connect(fadeOut, &QPropertyAnimation::finished, this, [this, widget]()
		{
			widget->deleteLater();
			current.clear();
			showNext();
		});
		fadeOut->start(QAbstractAnimation::DeleteWhenStopped);
	});
}
//----------------------------------------------------------------------------
void RaToastManager::imageReady(const QString &url)
{
	RetroAchievements *ra = RetroAchievements::instance();

	if (!current.isNull() && (ra != nullptr) && (url == currentImageUrl))
	{
		current->setImage(ra->images()->get(url));
	}
}
//----------------------------------------------------------------------------

#endif // FCEUX_HAS_RETRO_ACHIEVEMENTS
