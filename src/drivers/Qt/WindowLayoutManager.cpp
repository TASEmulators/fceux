/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2026 thor
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
// WindowLayoutManager.cpp
//
// Profiles are stored in QSettings using the following structure:
//
//   windowLayouts/<encodedProfile>/name                         = profile display name
//   windowLayouts/<encodedProfile>/version                      = 1
//   windowLayouts/<encodedProfile>/windows/<typeId>/count       = number of instances
//   windowLayouts/<encodedProfile>/windows/<typeId>/<i>/geometry
//   windowLayouts/<encodedProfile>/windows/<typeId>/<i>/state/... = window specific state
//
#include <QUrl>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QMessageBox>
#include <QInputDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "Qt/WindowLayoutManager.h"

static const char *layoutRootGroup = "windowLayouts";
static const int   layoutVersion   = 1;

//----------------------------------------------------------------------------
//---- Window Layout Manager
//----------------------------------------------------------------------------
WindowLayoutManager::WindowLayoutManager(void)
{
	pending.active   = false;
	pending.consumed = false;
	pending.index    = 0;
	pending.window   = nullptr;
	loadActive       = false;
}
//----------------------------------------------------------------------------
WindowLayoutManager *WindowLayoutManager::instance(void)
{
	static WindowLayoutManager mgr;

	return &mgr;
}
//----------------------------------------------------------------------------
QString WindowLayoutManager::profileGroup( const QString &profileName )
{
	// Percent encode so that names containing slashes or other special
	// characters do not create unintended QSettings sub-groups.
	return QString(layoutRootGroup) + "/" + QString::fromLatin1( QUrl::toPercentEncoding( profileName ) );
}
//----------------------------------------------------------------------------
QString WindowLayoutManager::instanceGroup( const QString &profileName, const QString &typeId, int index )
{
	return profileGroup(profileName) + "/windows/" + typeId + "/" + QString::number(index);
}
//----------------------------------------------------------------------------
void WindowLayoutManager::registerType( const QString &typeId, const QString &displayName,
		bool singleInstance, CreateFunc createFunc )
{
	for (auto it = typeList.begin(); it != typeList.end(); it++)
	{
		if ( it->typeId == typeId )
		{
			it->displayName    = displayName;
			it->singleInstance = singleInstance;
			it->createFunc     = createFunc;
			return;
		}
	}
	TypeInfo info;

	info.typeId         = typeId;
	info.displayName    = displayName;
	info.singleInstance = singleInstance;
	info.createFunc     = createFunc;

	typeList.push_back(info);
}
//----------------------------------------------------------------------------
const WindowLayoutManager::TypeInfo *WindowLayoutManager::findType( const QString &typeId ) const
{
	for (auto it = typeList.begin(); it != typeList.end(); it++)
	{
		if ( it->typeId == typeId )
		{
			return &(*it);
		}
	}
	return nullptr;
}
//----------------------------------------------------------------------------
WindowLayoutManager::WindowEntry *WindowLayoutManager::findWindow( QWidget *window )
{
	for (auto it = windowList.begin(); it != windowList.end(); it++)
	{
		if ( it->window == window )
		{
			return &(*it);
		}
	}
	return nullptr;
}
//----------------------------------------------------------------------------
void WindowLayoutManager::registerWindow( QWidget *window, const QString &typeId,
		WindowLayoutClient *client )
{
	if ( window == nullptr )
	{
		return;
	}
	WindowEntry *existing = findWindow(window);

	if ( existing != nullptr )
	{
		existing->typeId = typeId;
		existing->client = client;
		return;
	}
	WindowEntry entry;

	entry.window = window;
	entry.typeId = typeId;
	entry.client = client;

	windowList.push_back(entry);

	// Claim the pending layout entry if this window is being created
	// as part of a profile load.
	if ( pending.active && !pending.consumed &&
			(pending.window == nullptr) && (pending.typeId == typeId) )
	{
		pending.window = window;
	}
}
//----------------------------------------------------------------------------
void WindowLayoutManager::unregisterWindow( QWidget *window )
{
	for (auto it = windowList.begin(); it != windowList.end(); )
	{
		if ( (it->window == window) || it->window.isNull() )
		{
			it = windowList.erase(it);
		}
		else
		{
			it++;
		}
	}
	if ( pending.window == window )
	{
		pending.window   = nullptr;
		pending.consumed = true;
	}
}
//----------------------------------------------------------------------------
std::list <WindowLayoutManager::WindowEntry*> WindowLayoutManager::liveWindowsOfType( const QString &typeId )
{
	std::list <WindowEntry*> list;

	for (auto it = windowList.begin(); it != windowList.end(); it++)
	{
		// Hidden windows are either not yet shown or are closing (deleteLater pending).
		if ( !it->window.isNull() && (it->typeId == typeId) && it->window->isVisible() )
		{
			list.push_back( &(*it) );
		}
	}
	return list;
}
//----------------------------------------------------------------------------
bool WindowLayoutManager::hasPendingLayout( QWidget *window ) const
{
	return pending.active && !pending.consumed && (window != nullptr) && (pending.window == window);
}
//----------------------------------------------------------------------------
bool WindowLayoutManager::restorePendingLayout( QWidget *window )
{
	if ( !hasPendingLayout(window) )
	{
		return false;
	}
	WindowEntry *entry = findWindow(window);
	QSettings settings;

	pending.consumed = true;

	applyLayout( settings, instanceGroup( pending.profileName, pending.typeId, pending.index ),
			pending.profileName, window, entry ? entry->client : nullptr );

	return true;
}
//----------------------------------------------------------------------------
void WindowLayoutManager::applyLayout( QSettings &settings, const QString &group,
		const QString &profileName, QWidget *window, WindowLayoutClient *client )
{
	settings.beginGroup(group);

	QByteArray geometry = settings.value("geometry").toByteArray();

	// Leave fullscreen windows alone, restoring would kick them out of fullscreen.
	// Note: restoreGeometry clamps windows onto the available screens.
	if ( !geometry.isEmpty() && !window->isFullScreen() )
	{
		window->restoreGeometry( geometry );
	}

	if ( client != nullptr )
	{
		settings.beginGroup("state");
		client->loadLayoutState( profileName, settings );
		settings.endGroup();
	}
	settings.endGroup();
}
//----------------------------------------------------------------------------
bool WindowLayoutManager::profileExists( const QString &profileName ) const
{
	QSettings settings;

	return settings.contains( profileGroup(profileName) + "/name" );
}
//----------------------------------------------------------------------------
QStringList WindowLayoutManager::profileList(void) const
{
	QSettings settings;
	QStringList list;

	settings.beginGroup( layoutRootGroup );

	QStringList groups = settings.childGroups();

	for (int i=0; i<groups.size(); i++)
	{
		QString name = settings.value( groups[i] + "/name" ).toString();

		if ( !name.isEmpty() )
		{
			list.append(name);
		}
	}
	settings.endGroup();

	list.sort( Qt::CaseInsensitive );

	return list;
}
//----------------------------------------------------------------------------
bool WindowLayoutManager::saveProfile( const QString &profileName )
{
	if ( profileName.isEmpty() || loadActive )
	{
		return false;
	}
	QSettings settings;
	QString group = profileGroup(profileName);

	settings.remove(group);
	settings.beginGroup(group);
	settings.setValue("name", profileName);
	settings.setValue("version", layoutVersion);

	for (auto t = typeList.begin(); t != typeList.end(); t++)
	{
		std::list <WindowEntry*> live = liveWindowsOfType( t->typeId );

		if ( live.empty() )
		{
			continue;
		}
		if ( t->singleInstance )
		{
			live.resize(1);
		}
		settings.setValue( "windows/" + t->typeId + "/count", (int)live.size() );

		int idx = 0;
		for (auto w = live.begin(); w != live.end(); w++)
		{
			settings.beginGroup( "windows/" + t->typeId + "/" + QString::number(idx) );
			settings.setValue( "geometry", (*w)->window->saveGeometry() );

			if ( (*w)->client != nullptr )
			{
				settings.beginGroup("state");
				(*w)->client->saveLayoutState( profileName, settings );
				settings.endGroup();
			}
			settings.endGroup();
			idx++;
		}
	}
	settings.endGroup();
	settings.sync();

	curProfile = profileName;

	return settings.status() == QSettings::NoError;
}
//----------------------------------------------------------------------------
bool WindowLayoutManager::loadProfile( const QString &profileName )
{
	if ( loadActive || !profileExists(profileName) )
	{
		return false;
	}
	QSettings settings;

	loadActive = true;

	for (auto t = typeList.begin(); t != typeList.end(); t++)
	{
		int count = settings.value( profileGroup(profileName) + "/windows/" + t->typeId + "/count", 0 ).toInt();

		if ( t->singleInstance && (count > 1) )
		{
			count = 1;
		}
		if ( count <= 0 )
		{
			continue;
		}
		std::list <WindowEntry*> live = liveWindowsOfType( t->typeId );

		int idx = 0;

		// Existing windows are matched in creation order and just moved/resized.
		for (auto w = live.begin(); (w != live.end()) && (idx < count); w++)
		{
			applyLayout( settings, instanceGroup( profileName, t->typeId, idx ),
					profileName, (*w)->window, (*w)->client );
			idx++;
		}

		// Create any additional windows required by the profile.
		while ( (idx < count) && t->createFunc )
		{
			pending.active      = true;
			pending.consumed    = false;
			pending.profileName = profileName;
			pending.typeId      = t->typeId;
			pending.index       = idx;
			pending.window      = nullptr;

			t->createFunc();

			// Window registered itself but did not query for its layout.
			if ( (pending.window != nullptr) && !pending.consumed )
			{
				restorePendingLayout( pending.window );
			}
			pending.active = false;
			pending.window = nullptr;
			idx++;
		}
	}
	loadActive = false;

	curProfile = profileName;

	return true;
}
//----------------------------------------------------------------------------
bool WindowLayoutManager::deleteProfile( const QString &profileName )
{
	if ( !profileExists(profileName) )
	{
		return false;
	}
	QSettings settings;

	settings.remove( profileGroup(profileName) );

	if ( curProfile == profileName )
	{
		curProfile.clear();
	}
	return true;
}
//----------------------------------------------------------------------------
bool WindowLayoutManager::renameProfile( const QString &oldName, const QString &newName )
{
	if ( newName.isEmpty() || (oldName == newName) || !profileExists(oldName) )
	{
		return false;
	}
	QSettings settings;
	QString oldGroup = profileGroup(oldName);
	QString newGroup = profileGroup(newName);

	settings.remove(newGroup);

	settings.beginGroup(oldGroup);
	QStringList keys = settings.allKeys();
	std::map <QString, QVariant> values;

	for (int i=0; i<keys.size(); i++)
	{
		values[ keys[i] ] = settings.value( keys[i] );
	}
	settings.endGroup();

	settings.remove(oldGroup);

	settings.beginGroup(newGroup);
	for (auto it = values.begin(); it != values.end(); it++)
	{
		settings.setValue( it->first, it->second );
	}
	settings.setValue("name", newName);
	settings.endGroup();

	if ( curProfile == oldName )
	{
		curProfile = newName;
	}
	return true;
}
//----------------------------------------------------------------------------
//---- UI Helpers
//----------------------------------------------------------------------------
bool WindowLayoutManager::promptSaveProfileAs( QWidget *parent )
{
	bool ok = false;
	QString name = QInputDialog::getText( parent, QObject::tr("Save Window Layout"),
			QObject::tr("Layout Name:"), QLineEdit::Normal, curProfile, &ok ).trimmed();

	if ( !ok || name.isEmpty() )
	{
		return false;
	}
	if ( profileExists(name) )
	{
		if ( QMessageBox::question( parent, QObject::tr("Save Window Layout"),
				QObject::tr("Window layout '%1' already exists. Overwrite it?").arg(name),
				QMessageBox::Yes | QMessageBox::No, QMessageBox::No ) != QMessageBox::Yes )
		{
			return false;
		}
	}
	if ( !saveProfile(name) )
	{
		QMessageBox::warning( parent, QObject::tr("Save Window Layout"),
				QObject::tr("Failed to save window layout '%1'.").arg(name) );
		return false;
	}
	return true;
}
//----------------------------------------------------------------------------
bool WindowLayoutManager::promptOverwriteProfile( QWidget *parent, const QString &profileName )
{
	if ( QMessageBox::question( parent, QObject::tr("Overwrite Window Layout"),
			QObject::tr("Overwrite window layout '%1' with the current window layout?").arg(profileName),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::No ) != QMessageBox::Yes )
	{
		return false;
	}
	return saveProfile(profileName);
}
//----------------------------------------------------------------------------
bool WindowLayoutManager::promptDeleteProfile( QWidget *parent, const QString &profileName )
{
	if ( QMessageBox::question( parent, QObject::tr("Delete Window Layout"),
			QObject::tr("Delete window layout '%1'?").arg(profileName),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::No ) != QMessageBox::Yes )
	{
		return false;
	}
	return deleteProfile(profileName);
}
//----------------------------------------------------------------------------
QString WindowLayoutManager::promptRenameProfile( QWidget *parent, const QString &profileName )
{
	bool ok = false;
	QString name = QInputDialog::getText( parent, QObject::tr("Rename Window Layout"),
			QObject::tr("New Layout Name:"), QLineEdit::Normal, profileName, &ok ).trimmed();

	if ( !ok || name.isEmpty() || (name == profileName) )
	{
		return QString();
	}
	if ( profileExists(name) )
	{
		if ( QMessageBox::question( parent, QObject::tr("Rename Window Layout"),
				QObject::tr("Window layout '%1' already exists. Overwrite it?").arg(name),
				QMessageBox::Yes | QMessageBox::No, QMessageBox::No ) != QMessageBox::Yes )
		{
			return QString();
		}
	}
	return renameProfile( profileName, name ) ? name : QString();
}
//----------------------------------------------------------------------------
void WindowLayoutManager::openManageDialog( QWidget *parent )
{
	WindowLayoutManageDialog dialog(parent);

	dialog.exec();
}
//----------------------------------------------------------------------------
void WindowLayoutManager::populateMenu( QMenu *menu, QWidget *parent )
{
	QAction *act;
	QMenu *loadMenu, *deleteMenu;
	QStringList profiles = profileList();

	if ( !curProfile.isEmpty() && !profileExists(curProfile) )
	{
		curProfile.clear();
	}

	// Sub-menus are children of this menu and are not deleted by clear().
	QList <QMenu*> oldSubMenus = menu->findChildren<QMenu*>( QString(), Qt::FindDirectChildrenOnly );
	for (int i=0; i<oldSubMenus.size(); i++)
	{
		oldSubMenus[i]->deleteLater();
	}
	menu->clear();

	act = menu->addAction( QObject::tr("&Save Layout As...") );
	act->setStatusTip( QObject::tr("Save the current window layout as a named profile.") );
	QObject::connect( act, &QAction::triggered, [this, parent]{ promptSaveProfileAs(parent); } );

	if ( curProfile.isEmpty() )
	{
		act = menu->addAction( QObject::tr("&Update Current Layout") );
		act->setEnabled(false);
	}
	else
	{
		QString name = curProfile;
		act = menu->addAction( QObject::tr("&Update Layout '%1'").arg(name) );
		QObject::connect( act, &QAction::triggered, [this, name]{ saveProfile(name); } );
	}
	act->setStatusTip( QObject::tr("Overwrite the most recently loaded/saved layout profile with the current window layout.") );

	menu->addSeparator();

	loadMenu   = menu->addMenu( QObject::tr("&Load Layout") );
	deleteMenu = menu->addMenu( QObject::tr("&Delete Layout") );

	loadMenu->setEnabled( !profiles.empty() );
	deleteMenu->setEnabled( !profiles.empty() );

	for (int i=0; i<profiles.size(); i++)
	{
		QString name = profiles[i];

		act = loadMenu->addAction( name );
		act->setCheckable(true);
		act->setChecked( name == curProfile );
		QObject::connect( act, &QAction::triggered, [this, name]{ loadProfile(name); } );

		act = deleteMenu->addAction( name );
		QObject::connect( act, &QAction::triggered, [this, parent, name]{ promptDeleteProfile(parent, name); } );
	}

	menu->addSeparator();

	act = menu->addAction( QObject::tr("&Manage Layouts...") );
	act->setStatusTip( QObject::tr("Load, rename, and delete window layout profiles.") );
	QObject::connect( act, &QAction::triggered, [this, parent]{ openManageDialog(parent); } );
}
//----------------------------------------------------------------------------
//---- Window Layout Manage Dialog
//----------------------------------------------------------------------------
WindowLayoutManageDialog::WindowLayoutManageDialog( QWidget *parent )
	: QDialog(parent)
{
	QHBoxLayout *mainLayout;
	QVBoxLayout *btnLayout;
	QPushButton *saveAsButton, *closeButton;

	setWindowTitle( tr("Window Layouts") );

	mainLayout = new QHBoxLayout();
	btnLayout  = new QVBoxLayout();

	profileListBox = new QListWidget();
	profileListBox->setSelectionMode( QAbstractItemView::SingleSelection );

	loadButton      = new QPushButton( tr("&Load") );
	saveAsButton    = new QPushButton( tr("&Save Current As...") );
	overwriteButton = new QPushButton( tr("&Overwrite with Current") );
	renameButton    = new QPushButton( tr("&Rename...") );
	deleteButton    = new QPushButton( tr("&Delete") );
	closeButton     = new QPushButton( tr("&Close") );

	btnLayout->addWidget( loadButton );
	btnLayout->addWidget( saveAsButton );
	btnLayout->addWidget( overwriteButton );
	btnLayout->addWidget( renameButton );
	btnLayout->addWidget( deleteButton );
	btnLayout->addStretch();
	btnLayout->addWidget( closeButton );

	mainLayout->addWidget( profileListBox );
	mainLayout->addLayout( btnLayout );

	setLayout( mainLayout );

	connect( loadButton     , SIGNAL(clicked(void)), this, SLOT(loadClicked(void)) );
	connect( saveAsButton   , SIGNAL(clicked(void)), this, SLOT(saveAsClicked(void)) );
	connect( overwriteButton, SIGNAL(clicked(void)), this, SLOT(overwriteClicked(void)) );
	connect( renameButton   , SIGNAL(clicked(void)), this, SLOT(renameClicked(void)) );
	connect( deleteButton   , SIGNAL(clicked(void)), this, SLOT(deleteClicked(void)) );
	connect( closeButton    , SIGNAL(clicked(void)), this, SLOT(accept(void)) );

	connect( profileListBox, &QListWidget::itemSelectionChanged, this, [this]{ updateButtons(); } );
	connect( profileListBox, &QListWidget::itemDoubleClicked   , this, [this]{ loadClicked(); } );

	refreshList( WindowLayoutManager::instance()->currentProfile() );
}
//----------------------------------------------------------------------------
WindowLayoutManageDialog::~WindowLayoutManageDialog(void)
{
}
//----------------------------------------------------------------------------
QString WindowLayoutManageDialog::selectedProfile(void) const
{
	QListWidgetItem *item = profileListBox->currentItem();

	if ( (item == nullptr) || !item->isSelected() )
	{
		return QString();
	}
	return item->text();
}
//----------------------------------------------------------------------------
void WindowLayoutManageDialog::refreshList( const QString &selectName )
{
	QStringList profiles = WindowLayoutManager::instance()->profileList();

	profileListBox->clear();
	profileListBox->addItems( profiles );

	for (int i=0; i<profileListBox->count(); i++)
	{
		if ( profileListBox->item(i)->text() == selectName )
		{
			profileListBox->setCurrentRow(i);
			break;
		}
	}
	updateButtons();
}
//----------------------------------------------------------------------------
void WindowLayoutManageDialog::updateButtons(void)
{
	bool hasSel = !selectedProfile().isEmpty();

	loadButton->setEnabled( hasSel );
	overwriteButton->setEnabled( hasSel );
	renameButton->setEnabled( hasSel );
	deleteButton->setEnabled( hasSel );
}
//----------------------------------------------------------------------------
void WindowLayoutManageDialog::loadClicked(void)
{
	QString name = selectedProfile();

	if ( !name.isEmpty() )
	{
		WindowLayoutManager::instance()->loadProfile(name);
	}
}
//----------------------------------------------------------------------------
void WindowLayoutManageDialog::saveAsClicked(void)
{
	WindowLayoutManager *mgr = WindowLayoutManager::instance();

	if ( mgr->promptSaveProfileAs(this) )
	{
		refreshList( mgr->currentProfile() );
	}
}
//----------------------------------------------------------------------------
void WindowLayoutManageDialog::overwriteClicked(void)
{
	QString name = selectedProfile();

	if ( !name.isEmpty() )
	{
		WindowLayoutManager::instance()->promptOverwriteProfile( this, name );
	}
}
//----------------------------------------------------------------------------
void WindowLayoutManageDialog::renameClicked(void)
{
	QString name = selectedProfile();

	if ( name.isEmpty() )
	{
		return;
	}
	QString newName = WindowLayoutManager::instance()->promptRenameProfile( this, name );

	if ( !newName.isEmpty() )
	{
		refreshList( newName );
	}
}
//----------------------------------------------------------------------------
void WindowLayoutManageDialog::deleteClicked(void)
{
	QString name = selectedProfile();

	if ( name.isEmpty() )
	{
		return;
	}
	if ( WindowLayoutManager::instance()->promptDeleteProfile( this, name ) )
	{
		refreshList();
	}
}
//----------------------------------------------------------------------------
