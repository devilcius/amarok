/****************************************************************************************
 * Copyright (c) 2007 Bart Cerneels <bart.cerneels@kde.org>                             *
 * Copyright (c) 2011 Lucas Lira Gomes <x8lucas8x@gmail.com>                            *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "PlaylistManager.h"

#include "amarokurls/AmarokUrl.h"
#include "amarokconfig.h"
#include "App.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core-impl/playlists/types/file/PlaylistFile.h"
#include "playlist/PlaylistModelStack.h"
#include "core-impl/playlists/types/file/PlaylistFileSupport.h"
#include "core/podcasts/PodcastProvider.h"
#include "file/PlaylistFileProvider.h"
#include "file/KConfigSyncRelStore.h"
#include "core-impl/podcasts/sql/SqlPodcastProvider.h"
#include "playlistmanager/sql/SqlUserPlaylistProvider.h"
#include "playlistmanager/SyncedPlaylist.h"
#include "core/support/Debug.h"
#include "core/support/Components.h"
#include "core/interfaces/Logger.h"
#include "browsers/playlistbrowser/UserPlaylistModel.h"

#include <kdirlister.h>
#include <kio/jobclasses.h>
#include <kio/job.h>
#include <KInputDialog>
#include <KLocale>
#include <KUrl>

#include <QFileInfo>

#include <typeinfo>

using namespace Meta;
using namespace Playlists;

PlaylistManager *PlaylistManager::s_instance = 0;

namespace The
{
    PlaylistManager *playlistManager() { return PlaylistManager::instance(); }
}

PlaylistManager *
PlaylistManager::instance()
{
    return s_instance ? s_instance : new PlaylistManager();
}

void
PlaylistManager::destroy()
{
    if (s_instance) {
        delete s_instance;
        s_instance = 0;
    }
}

PlaylistManager::PlaylistManager()
{
    s_instance = this;

    m_syncRelStore = new KConfigSyncRelStore();

    m_playlistFileProvider = new Playlists::PlaylistFileProvider();
    addProvider( m_playlistFileProvider, UserPlaylist );

    m_defaultPodcastProvider = new Podcasts::SqlPodcastProvider();
    addProvider( m_defaultPodcastProvider, PlaylistManager::PodcastChannel );
    CollectionManager::instance()->addTrackProvider( m_defaultPodcastProvider );

    m_defaultUserPlaylistProvider = new Playlists::SqlUserPlaylistProvider();
    addProvider( m_defaultUserPlaylistProvider, UserPlaylist );

}

PlaylistManager::~PlaylistManager()
{
    delete m_defaultPodcastProvider;
    delete m_defaultUserPlaylistProvider;
    delete m_playlistFileProvider;
    delete m_syncRelStore;
}

bool
PlaylistManager::hasToSync( Playlists::PlaylistPtr master, Playlists::PlaylistPtr slave )
{
    DEBUG_BLOCK
    debug() << "master: " << master->uidUrl();
    debug() << "slave: " << slave->uidUrl();

    if( !m_syncRelStore )
        return false;

    return m_syncRelStore->hasToSync( master, slave );
}

void
PlaylistManager::addProvider( Playlists::PlaylistProvider *provider, int category )
{
    bool newCategory = false;
    if( !m_providerMap.uniqueKeys().contains( category ) )
            newCategory = true;

    //disconnect all signals connected to this object to be sure.
    provider->disconnect( this, 0 );

    m_providerMap.insert( category, provider );
    connect( provider, SIGNAL(updated()), SLOT(slotUpdated()));
    connect( provider, SIGNAL(playlistAdded( Playlists::PlaylistPtr )),
             SLOT(slotPlaylistAdded( Playlists::PlaylistPtr )));
    connect( provider, SIGNAL(playlistRemoved( Playlists::PlaylistPtr )),
             SLOT(slotPlaylistRemoved( Playlists::PlaylistPtr )));

    if( newCategory )
        emit categoryAdded( category );

    emit providerAdded( provider, category );
    emit updated( category );

    loadPlaylists( provider, category );
}

void
PlaylistManager::loadPlaylists( Playlists::PlaylistProvider *provider, int category )
{
    foreach( Playlists::PlaylistPtr playlist, provider->playlists() )
        addPlaylist( playlist, category );
}

void
PlaylistManager::addPlaylist( Playlists::PlaylistPtr playlist, int category )
{
    SyncedPlaylistPtr syncedPlaylist = m_syncRelStore->asSyncedPlaylist( playlist );

    //NULL when not synced or a slave added before it's master copy ("early slave")
    if( syncedPlaylist )
    {
        if( !m_syncedPlaylistMap.keys().contains( syncedPlaylist ) )
        {
            //this can only happen when playlist == the master of the syncedPlaylist

            //Search for any slaves created before their master ("early slaves")
            //To set-up a sync between them
            //Only search in the category of the new playlist, i.e. no cross category syncing.
            foreach( Playlists::PlaylistPtr existingPlaylist, m_playlistMap.values( category ) )
            {
                //If this is a slave asSyncedPlaylist() will make it part of the syncedPlaylist
                if( m_syncRelStore->asSyncedPlaylist( existingPlaylist ) == syncedPlaylist )
                {
                    m_playlistMap.remove( category, existingPlaylist );

                    if( !m_syncedPlaylistMap.values( syncedPlaylist ).contains( existingPlaylist ) )
                        m_syncedPlaylistMap.insert( syncedPlaylist, existingPlaylist );
                }
            }
        }

        if( !m_syncedPlaylistMap.values( syncedPlaylist ).contains( playlist ) )
        {
            m_syncedPlaylistMap.insert( syncedPlaylist, playlist );

            //The synchronosation will be done in the next mainloop run
            m_syncNeeded.append( syncedPlaylist );
            QTimer::singleShot( 0, this, SLOT( slotSyncNeeded() ) );
        }

        //deliberatly reusing the passed argument
        playlist = PlaylistPtr::dynamicCast( syncedPlaylist );

        if( m_playlistMap.values( category ).contains( playlist ) )
        {
             //no need to add it again but do let the model know something changed.
             emit playlistUpdated( playlist, category );
             return;
        }
    }

    m_playlistMap.insert( category, playlist );
    //reemit so models know about new playlist in their category
    emit playlistAdded( playlist, category );
}

void
PlaylistManager::removeProvider( Playlists::PlaylistProvider *provider )
{
    DEBUG_BLOCK

    if( !provider )
        return;

    if( !m_providerMap.values( provider->category() ).contains( provider ) )
    {
        return;
    }

    removePlaylists( provider );

    m_providerMap.remove( provider->category(), provider );

    emit providerRemoved( provider, provider->category() );
    emit updated( provider->category() );
}

void
PlaylistManager::removePlaylists( Playlists::PlaylistProvider *provider )
{
    foreach( Playlists::PlaylistPtr playlist, m_playlistMap.values( provider->category() ) )
        if( playlist->provider() && playlist->provider() == provider )
        {
            foreach( SyncedPlaylistPtr syncedPlaylist, m_syncedPlaylistMap.keys( playlist ) )
                m_syncedPlaylistMap.remove( syncedPlaylist, playlist );

            removePlaylist( playlist, provider->category() );
        }
}

void
PlaylistManager::removePlaylist( Playlists::PlaylistPtr playlist, int category )
{
    if( typeid( *playlist.data() ) == typeid( SyncedPlaylist ) )
    {
        SyncedPlaylistPtr syncedPlaylist = SyncedPlaylistPtr::dynamicCast( playlist );
        //TODO: this might be wrong if there were multiple playlists from the same provider.
        //remove the specific child playlist, not all from same provider.
        syncedPlaylist->removePlaylistsFrom( playlist->provider() );
        if( syncedPlaylist->isEmpty() )
            m_playlistMap.remove( category, playlist );

        m_syncNeeded.removeAll( syncedPlaylist );
    }
    else
    {
        m_playlistMap.remove( category, playlist );
    }

    emit playlistRemoved( playlist, category );
}

void
PlaylistManager::slotUpdated()
{
    Playlists::PlaylistProvider *provider =
            dynamic_cast<Playlists::PlaylistProvider *>( QObject::sender() );
    if( !provider )
        return;

    //forcefull reload all the providers playlists.
    //This is an expensive operation, the provider should use playlistAdded/Removed signals instead.
    removePlaylists( provider );
    loadPlaylists( provider, provider->category() );
    emit updated( provider->category() );
}

void
PlaylistManager::slotPlaylistAdded( Playlists::PlaylistPtr playlist )
{
    addPlaylist( playlist, playlist->provider()->category() );
}

void
PlaylistManager::slotPlaylistRemoved( Playlists::PlaylistPtr playlist )
{
    removePlaylist( playlist, playlist->provider()->category() );
}

Playlists::PlaylistList
PlaylistManager::playlistsOfCategory( int playlistCategory )
{
    return m_playlistMap.values( playlistCategory );
}

PlaylistProviderList
PlaylistManager::providersForCategory( int playlistCategory )
{
    return m_providerMap.values( playlistCategory );
}

Playlists::PlaylistProvider *
PlaylistManager::playlistProvider(int category, QString name)
{
    QList<Playlists::PlaylistProvider *> providers( m_providerMap.values( category ) );

    QListIterator<Playlists::PlaylistProvider *> i(providers);
    while( i.hasNext() )
    {
        Playlists::PlaylistProvider * p = static_cast<Playlists::PlaylistProvider *>( i.next() );
        if( p->prettyName() == name )
            return p;
    }

    return 0;
}

void
PlaylistManager::downloadPlaylist( const KUrl &path, const Playlists::PlaylistFilePtr playlist )
{
    KIO::StoredTransferJob *downloadJob =  KIO::storedGet( path );

    m_downloadJobMap[downloadJob] = playlist;

    connect( downloadJob, SIGNAL( result( KJob * ) ),
             this, SLOT( downloadComplete( KJob * ) ) );

    Amarok::Components::logger()->newProgressOperation( downloadJob, i18n( "Downloading Playlist" ) );
}

void
PlaylistManager::downloadComplete( KJob *job )
{
    if( !job->error() == 0 )
    {
        //TODO: error handling here
        return ;
    }

    Playlists::PlaylistFilePtr playlist = m_downloadJobMap.take( job );

    QString contents = static_cast<KIO::StoredTransferJob *>(job)->data();
    QTextStream stream;
    stream.setString( &contents );

    playlist->load( stream );
}

bool
PlaylistManager::save( Meta::TrackList tracks, const QString &name,
                       Playlists::PlaylistProvider *toProvider, bool editName )
{
    //if toProvider is 0 use the default Playlists::UserPlaylistProvider (SQL)
    Playlists::UserPlaylistProvider *prov = toProvider
        ? qobject_cast<Playlists::UserPlaylistProvider *>( toProvider )
        : m_defaultUserPlaylistProvider;
    if( !prov )
        return false;

    Playlists::PlaylistPtr playlist = prov->save( tracks, name );
    if( playlist.isNull() )
        return false;

    if( editName )
    {
        AmarokUrl("amarok://navigate/playlists/user playlists").run();
        emit renamePlaylist( playlist );
    }

    return true;
}

bool
PlaylistManager::import( const QString& fromLocation )
{
    // used by: PlaylistBrowserNS::UserModel::dropMimeData()
    AMAROK_DEPRECATED
    DEBUG_BLOCK
    if( !m_playlistFileProvider )
    {
        debug() << "ERROR: m_playlistFileProvider was null";
        return false;
    }
    return m_playlistFileProvider->import( KUrl(fromLocation) );
}

void
PlaylistManager::rename( Playlists::PlaylistPtr playlist )
{
    if( playlist.isNull() )
        return;

    Playlists::UserPlaylistProvider *provider
            = qobject_cast<Playlists::UserPlaylistProvider *>( getProvidersForPlaylist( playlist ).first() );

    if( !provider )
        return;

    bool ok;
    const QString newName = KInputDialog::getText( i18n("Change playlist"),
                i18n("Enter new name for playlist:"), playlist->name(),
                                                   &ok );
    if( ok )
    {
        debug() << "Changing name from " << playlist->name() << " to " << newName.trimmed();
        provider->rename( playlist, newName.trimmed() );
    }
}

bool
PlaylistManager::deletePlaylists( Playlists::PlaylistList playlistlist )
{
    // Map the playlists to their respective providers
    QHash<Playlists::UserPlaylistProvider*, Playlists::PlaylistList> provLists;
    foreach( Playlists::PlaylistPtr playlist, playlistlist )
    {
        // Get the providers of the respective playlists
        Playlists::UserPlaylistProvider *prov = qobject_cast<Playlists::UserPlaylistProvider *>(
                getProvidersForPlaylist( playlist ).first() );

        if( prov )
        {
            Playlists::PlaylistList pllist;
            pllist << playlist;
            // If the provider already has at least one playlist to delete, add another to its list
            if( provLists.contains( prov ) )
            {
                provLists[ prov ] << pllist;

            }
            // If we are adding a new provider, put it in the hash, initialize its list
            else
                provLists.insert( prov, pllist );
        }
    }

    // Pass each list of playlists to the respective provider for deletion

    bool removedSuccess = true;
    foreach( Playlists::UserPlaylistProvider* prov, provLists.keys() )
    {
        removedSuccess = prov->deletePlaylists( provLists.value( prov ) ) && removedSuccess;
    }

    return removedSuccess;
}

QList<Playlists::PlaylistProvider*>
PlaylistManager::getProvidersForPlaylist( const Playlists::PlaylistPtr playlist )
{
    QList<Playlists::PlaylistProvider*> providers;

    if( playlist.isNull() )
        return providers;

    SyncedPlaylistPtr syncedPlaylist = SyncedPlaylistPtr::dynamicCast( playlist );
    if( syncedPlaylist && m_syncedPlaylistMap.keys().contains( syncedPlaylist ) )
    {
        foreach( Playlists::PlaylistPtr playlist, m_syncedPlaylistMap.values( syncedPlaylist ) )
            if( !providers.contains( playlist->provider() ) )
                providers << playlist->provider();

        return providers;
    }

    Playlists::PlaylistProvider* provider = playlist->provider();
    if( provider )
        return providers << provider;

    //Iteratively check all providers' playlists for ownership
    QList< Playlists::PlaylistProvider* > userPlaylists = m_providerMap.values( UserPlaylist );
    foreach( Playlists::PlaylistProvider* provider, userPlaylists )
    {
        if( provider->playlists().contains( playlist ) )
                return providers << provider;
    }

    return providers;
}


bool
PlaylistManager::isWritable( const Playlists::PlaylistPtr &playlist )
{
    Playlists::UserPlaylistProvider *provider
            = qobject_cast<Playlists::UserPlaylistProvider *>( getProvidersForPlaylist( playlist ).first() );

    if( provider )
        return provider->isWritable();
    else
        return false;
}

void
PlaylistManager::completePodcastDownloads()
{
    foreach( Playlists::PlaylistProvider *prov, providersForCategory( PodcastChannel ) )
    {
        Podcasts::PodcastProvider *podcastProvider = dynamic_cast<Podcasts::PodcastProvider *>( prov );
        if( !podcastProvider )
            continue;

        podcastProvider->completePodcastDownloads();
    }
}

void
PlaylistManager::setupSync( const Playlists::PlaylistPtr master, const Playlists::PlaylistPtr slave )
{
    DEBUG_BLOCK
    debug() << "master: " << master->uidUrl();
    debug() << "slave: " << slave->uidUrl();

    //If there is no sync relation established between these two, then we must setup a sync.
    if( hasToSync( master, slave ) )
       return;

    Playlists::PlaylistPtr tempMaster;
    Playlists::PlaylistPtr tempSlave;

    m_syncRelStore->addSync( master, slave );

    foreach( const Playlists::PlaylistPtr tempPlaylist, m_playlistMap )
    {
        if( master == tempPlaylist )
        {
            tempMaster = tempPlaylist;
            break;
        }
    }

    foreach( const Playlists::PlaylistPtr tempPlaylist, m_playlistMap )
    {
        if( slave == tempPlaylist )
        {
            tempSlave = tempPlaylist;
            break;
        }
    }

    if( tempMaster && tempSlave )
    {
        SyncedPlaylistPtr syncedPlaylist = m_syncRelStore->asSyncedPlaylist( tempMaster );

        m_syncRelStore->asSyncedPlaylist( tempSlave );

        Playlists::PlaylistPtr syncedPlaylistPtr =
                        Playlists::PlaylistPtr::dynamicCast( syncedPlaylist );

        int category = syncedPlaylist->master()->provider()->category();

        if( !m_playlistMap.values( category ).contains( syncedPlaylistPtr ) )
        {
            removePlaylist( tempMaster, tempMaster->provider()->category() );
            removePlaylist( tempSlave, tempSlave->provider()->category() );

            m_syncedPlaylistMap.insert( syncedPlaylist, tempMaster );
            m_syncedPlaylistMap.insert( syncedPlaylist, tempSlave );

            m_playlistMap.insert( category, syncedPlaylistPtr );
            //reemit so models know about new playlist in their category
            emit playlistAdded( syncedPlaylistPtr, category );
        }
    }
}

void PlaylistManager::slotSyncNeeded()
{
    foreach( SyncedPlaylistPtr syncedPlaylist, m_syncNeeded )
        if ( syncedPlaylist->syncNeeded() )
            syncedPlaylist->doSync();

    m_syncNeeded.clear();
}

#include "PlaylistManager.moc"
