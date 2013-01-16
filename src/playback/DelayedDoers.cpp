/***************************************************************************************
 * Copyright (c) 2013 Matěj Laitl <matej@laitl.cz>                                      *
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

#include "DelayedDoers.h"

#include "core/support/Debug.h"

#include <Phonon/MediaController>
#include <Phonon/MediaObject>

DelayedDoer::DelayedDoer( Phonon::MediaObject *mediaObject,
                          const QSet<Phonon::State> &applicableStates )
    : m_mediaObject( mediaObject )
    , m_applicableStates( applicableStates )
{
    Q_ASSERT( mediaObject );
    connect( mediaObject, SIGNAL(stateChanged(Phonon::State,Phonon::State)),
                          SLOT(slotStateChanged(Phonon::State)) );
    connect( mediaObject, SIGNAL(destroyed(QObject*)), SLOT(deleteLater()) );
}

void
DelayedDoer::slotStateChanged( Phonon::State newState )
{
    if( m_applicableStates.contains( newState ) )
    {
        // don't let be called twice, deleteLater() may fire really LATER
        disconnect( m_mediaObject, 0, this, 0 );
        performAction();
        deleteLater();
    }
    else
        debug() << __PRETTY_FUNCTION__ << "newState" << newState << "not applicable, waiting...";
}

DelayedSeeker::DelayedSeeker( Phonon::MediaObject *mediaObject, qint64 seekTo )
    : DelayedDoer( mediaObject, QSet<Phonon::State>() << Phonon::PlayingState
                                                      << Phonon::BufferingState
                                                      << Phonon::PausedState )
    , m_seekTo( seekTo )
{
}

void
DelayedSeeker::performAction()
{
    m_mediaObject->seek( m_seekTo );
    emit trackPositionChanged( m_seekTo, /* userSeek */ true );
    m_mediaObject->play();
}

DelayedTrackChanger::DelayedTrackChanger( Phonon::MediaObject *mediaObject,
                                          Phonon::MediaController *mediaController,
                                          int trackNumber, qint64 seekTo )
    : DelayedSeeker( mediaObject, seekTo )
    , m_mediaController( mediaController )
    , m_trackNumber( trackNumber )
{
    Q_ASSERT( mediaController );
    connect( mediaController, SIGNAL(destroyed(QObject*)), SLOT(deleteLater()) );
    Q_ASSERT( trackNumber > 0 );
}

void
DelayedTrackChanger::performAction()
{
    m_mediaController->setCurrentTitle( m_trackNumber );
    if( m_seekTo )
    {
        m_mediaObject->seek( m_seekTo );
        emit trackPositionChanged( m_seekTo, /* userSeek */ true );
    }
    m_mediaObject->play();
}