// RetroAchievementsToast.cpp
//
#ifdef FCEUX_HAS_RETRO_ACHIEVEMENTS

#include <cmath>
#include <vector>

#include <SDL.h>

#include <QFontMetrics>
#include <QPainter>
#include <QPropertyAnimation>
#include <QTimer>

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
// Short rising chime, synthesized so no sound asset has to be shipped.
//----------------------------------------------------------------------------
class RaChime
{
	public:
		RaChime(void) : device(0), failed(false), audioInit(false) {}

		~RaChime(void)
		{
			if (device != 0)
			{
				SDL_CloseAudioDevice(device);
			}
			if (audioInit)
			{
				SDL_QuitSubSystem(SDL_INIT_AUDIO);
			}
		}

		void play(void)
		{
			if (failed)
			{
				return;
			}
			if (device == 0 && !open())
			{
				failed = true;
				return;
			}
			SDL_ClearQueuedAudio(device);
			SDL_QueueAudio(device, samples.data(), static_cast<Uint32>(samples.size() * sizeof(int16_t)));
		}

	private:
		static const int sampleRate = 44100;

		bool open(void)
		{
			SDL_AudioSpec spec;

			if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
			{
				return false;
			}
			audioInit = true;

			SDL_zero(spec);
			spec.freq = sampleRate;
			spec.format = AUDIO_S16SYS;
			spec.channels = 1;
			spec.samples = 1024;

			device = SDL_OpenAudioDevice(nullptr, 0, &spec, nullptr, 0);

			if (device == 0)
			{
				return false;
			}
			synthesize();

			SDL_PauseAudioDevice(device, 0);
			return true;
		}

		void synthesize(void)
		{
			const double notes[] = { 1046.5, 1318.5, 1568.0, 2093.0 };
			const double noteStart = 0.09;
			const double noteLength = 0.35;
			const double pi = 3.14159265358979323846;
			const size_t total = static_cast<size_t>(sampleRate * (noteStart * 3 + noteLength));
			std::vector<double> mix(total, 0.0);

			for (int n = 0; n < 4; n++)
			{
				const size_t first = static_cast<size_t>(sampleRate * noteStart * n);
				const size_t count = static_cast<size_t>(sampleRate * noteLength);

				for (size_t i = 0; i < count && (first + i) < total; i++)
				{
					const double t = static_cast<double>(i) / sampleRate;
					const double attack = (t < 0.003) ? (t / 0.003) : 1.0;
					const double env = attack * std::exp(-t * 9.0);
					const double tone = std::sin(2.0 * pi * notes[n] * t) + 0.3 * std::sin(4.0 * pi * notes[n] * t);

					mix[first + i] += env * tone;
				}
			}
			samples.resize(total);

			for (size_t i = 0; i < total; i++)
			{
				double v = mix[i] * 0.2;

				if (v > 1.0) { v = 1.0; }
				if (v < -1.0) { v = -1.0; }

				samples[i] = static_cast<int16_t>(v * 32767.0);
			}
		}

		SDL_AudioDeviceID device;
		bool failed;
		bool audioInit;
		std::vector<int16_t> samples;
};
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
	, chime(new RaChime())
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
	delete chime;
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
		chime->play();
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
