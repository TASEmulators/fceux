// WindowLayoutManager.h
//
// Saves and recalls the geometry (and optional internal state) of tool,
// config, and debug windows into named layout profiles.
//
#pragma once

#include <list>
#include <map>
#include <functional>

#include <QString>
#include <QStringList>
#include <QPointer>
#include <QWidget>
#include <QDialog>
#include <QSettings>
#include <QListWidget>
#include <QPushButton>

class QMenu;

// Optional interface for windows that have internal state (beyond geometry)
// that should be stored in a layout profile. The settings object passed in is
// already positioned in a group that is private to that window instance.
class WindowLayoutClient
{
	public:
		virtual void saveLayoutState( const QString &profileName, QSettings &settings ){}
		virtual void loadLayoutState( const QString &profileName, QSettings &settings ){}

	protected:
		// Not virtual: clients are never deleted through this interface.
		~WindowLayoutClient(void) = default;
};

class WindowLayoutManager
{
	public:
		// Opens a new window of a type when a profile being loaded needs one
		// that is not currently open. The created window must call
		// registerWindow + restorePendingLayout during its construction.
		typedef std::function<void(void)> CreateFunc;

		static WindowLayoutManager *instance(void);

		void registerType( const QString &typeId, const QString &displayName,
				bool singleInstance, CreateFunc createFunc = nullptr );

		// Windows call this at construction. Instances are tracked in
		// registration order, which is used for multi-instance matching.
		void registerWindow( QWidget *window, const QString &typeId,
				WindowLayoutClient *client = nullptr );

		// Windows call this at destruction.
		void unregisterWindow( QWidget *window );

		// Returns true if a profile load is in progress and has a layout entry
		// for this window. The window should then skip restoring its own
		// previously saved geometry/state.
		bool hasPendingLayout( QWidget *window ) const;

		// Applies the pending layout entry (geometry + client state) to the
		// window. Returns false if there was no pending layout for it, in which
		// case the window should restore its own previous state.
		bool restorePendingLayout( QWidget *window );

		bool saveProfile( const QString &profileName );
		bool loadProfile( const QString &profileName );
		bool deleteProfile( const QString &profileName );
		bool renameProfile( const QString &oldName, const QString &newName );
		bool profileExists( const QString &profileName ) const;
		QStringList profileList(void) const;

		QString currentProfile(void) const { return curProfile; }
		bool loadInProgress(void) const { return loadActive; }

		// Rebuilds the given menu with profile save/load/delete/manage actions.
		void populateMenu( QMenu *menu, QWidget *parent );

		// UI helpers that prompt the user as needed.
		bool promptSaveProfileAs( QWidget *parent );
		bool promptOverwriteProfile( QWidget *parent, const QString &profileName );
		bool promptDeleteProfile( QWidget *parent, const QString &profileName );
		QString promptRenameProfile( QWidget *parent, const QString &profileName );
		void openManageDialog( QWidget *parent );

	private:
		WindowLayoutManager(void);

		struct TypeInfo
		{
			QString    typeId;
			QString    displayName;
			bool       singleInstance;
			CreateFunc createFunc;
		};

		struct WindowEntry
		{
			QPointer<QWidget>   window;
			QString             typeId;
			WindowLayoutClient *client;
		};

		struct PendingLayout
		{
			bool     active;
			bool     consumed;
			QString  profileName;
			QString  typeId;
			int      index;
			QWidget *window;
		};

		std::list <TypeInfo>    typeList;
		std::list <WindowEntry> windowList;
		PendingLayout           pending;
		QString                 curProfile;
		bool                    loadActive;

		const TypeInfo *findType( const QString &typeId ) const;
		WindowEntry *findWindow( QWidget *window );
		std::list <WindowEntry*> liveWindowsOfType( const QString &typeId );

		void applyLayout( QSettings &settings, const QString &group,
				const QString &profileName, QWidget *window, WindowLayoutClient *client );

		static QString profileGroup( const QString &profileName );
		static QString instanceGroup( const QString &profileName,
				const QString &typeId, int index );
};

class WindowLayoutManageDialog : public QDialog
{
	Q_OBJECT

	public:
		WindowLayoutManageDialog( QWidget *parent = nullptr );
		~WindowLayoutManageDialog(void);

	private:
		QListWidget *profileListBox;
		QPushButton *loadButton;
		QPushButton *overwriteButton;
		QPushButton *renameButton;
		QPushButton *deleteButton;

		QString selectedProfile(void) const;
		void refreshList( const QString &selectName = QString() );
		void updateButtons(void);

	private slots:
		void loadClicked(void);
		void saveAsClicked(void);
		void overwriteClicked(void);
		void renameClicked(void);
		void deleteClicked(void);
};
