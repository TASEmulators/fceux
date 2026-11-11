// RetroAchievementsToast.h
//
// Popup notifications for RetroAchievements events (unlocks, leaderboards, connection
// problems), with the badge image and an optional unlock sound.
#pragma once

#ifdef FCEUX_HAS_RETRO_ACHIEVEMENTS

#include <QObject>
#include <QPixmap>
#include <QPointer>
#include <QQueue>
#include <QString>
#include <QWidget>

class RaChime;

// One popup. A frameless, translucent tool window that never takes focus or input.
class RaToastWidget : public QWidget
{
	Q_OBJECT

	public:
		RaToastWidget(QWidget *parent, const QString &title, const QString &text, bool highlight);

		void setImage(const QPixmap &pixmap);

	protected:
		void paintEvent(QPaintEvent *event) override;

	private:
		QString title;
		QString text;
		QPixmap image;
		bool highlight;
};

class RaToastManager : public QObject
{
	Q_OBJECT

	public:
		// window is the main window, which the popups are placed over.
		explicit RaToastManager(QWidget *window);
		~RaToastManager(void);

	private slots:
		void achievementUnlocked(const QString &title, const QString &description,
			unsigned int points, const QString &badgeUrl);
		void gameCompleted(const QString &title);
		void notification(const QString &title, const QString &text, const QString &imageUrl);
		void imageReady(const QString &url);

	private:
		struct Toast
		{
			QString title;
			QString text;
			QString imageUrl;
			bool    highlight = false;
		};

		void enqueue(const Toast &toast);
		void showNext(void);

		QWidget *window;
		QPointer<RaToastWidget> current;
		QString currentImageUrl;
		QQueue<Toast> queue;
		RaChime *chime;
};

#endif // FCEUX_HAS_RETRO_ACHIEVEMENTS
