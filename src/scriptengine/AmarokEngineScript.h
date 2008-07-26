/******************************************************************************
 * Copyright (C) 2008 Peter ZHOU <peterzhoulei@gmail.com>                     *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License as             *
 * published by the Free Software Foundation; either version 2 of             *
 * the License, or (at your option) any later version.                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
 ******************************************************************************/

#ifndef AMAROK_ENGINE_SCRIPT_H
#define AMAROK_ENGINE_SCRIPT_H

#include <QObject>
#include <QtScript>

namespace AmarokScript
{

    class AmarokEngineScript : public QObject
    {
        Q_OBJECT

        Q_PROPERTY ( bool randomMode READ randomMode WRITE setRandomMode )
        Q_PROPERTY ( bool dynamicMode READ dynamicMode WRITE setDynamicMode )
        Q_PROPERTY ( bool repeatPlaylist READ repeatPlaylist WRITE setRepeatPlaylist )
        Q_PROPERTY ( bool repeatTrack READ repeatTrack WRITE setRepeatTrack )
        Q_PROPERTY ( int volume READ volume WRITE setVolume );

        public:
            AmarokEngineScript( QScriptEngine* ScriptEngine );
            ~AmarokEngineScript();

        public slots:
            void Play();
            void Stop( bool forceInstant = false );
            void Pause();
            void Next();
            void Prev();
            void PlayPause();
            void PlayAudioCD();
            void Seek( int ms );
            void SeekRelative( int ms );
            void SeekForward( int ms = 10000 );
            void SeekBackward( int ms = 10000 );
            int  IncreaseVolume( int ticks = 100/25 );
            int  DecreaseVolume( int ticks = 100/25 );
            void Mute();
            int  trackPosition();
            int  engineState();

        signals:
            void trackFinished();
            void trackChanged();
            void trackSeeked( int ); //return relative time in million second
            void volumeChanged( int );
            void trackPlayPause( int );  //Playing: 0, Paused: 1

        private:
            bool randomMode();
            bool dynamicMode();
            bool repeatPlaylist();
            bool repeatTrack();
            void setRandomMode( bool enable );
            void setDynamicMode( bool enable );
            void setRepeatPlaylist( bool enable );
            void setRepeatTrack( bool enable );
            int  volume();
            void setVolume( int percent );
    };
}

#endif
