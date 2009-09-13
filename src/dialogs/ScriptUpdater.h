/****************************************************************************************
 * Copyright (c) 2009 Jakob Kummerow <jakob.kummerow@gmail.com>                         *
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

#ifndef AMAROK_SCRIPTUPDATER_H
#define AMAROK_SCRIPTUPDATER_H

#include <string.h>

#include <KIO/Job>

#include <QThread>
#include <QSemaphore>
#include <QTemporaryFile>


class ScriptUpdater : public QThread
{

    Q_OBJECT

    public:
        explicit ScriptUpdater();
        virtual ~ScriptUpdater();
        void setScriptPath( const QString& scriptPath );

    public slots:
        void updateScript();

    signals:
        void finished( QString scriptPath );

    protected:
        virtual void run();

    private slots:
        void phase2( KJob * job );
        void phase3( KJob * job );
        void phase4( KJob * job );

    private:
        
        QString m_scriptPath;
        QSemaphore m_stayAlive;
        // static configuration
        QString m_updateBaseUrl;
        QString m_archiveFilename, m_versionFilename, m_signatureFilename;
        QString m_publicKey;
        // dynamic information about the script
        QString m_scriptname, m_scriptversion, m_fileName;
        QTemporaryFile m_archiveFile, m_sigFile;

};

#endif // AMAROK_SCRIPTUPDATER_H
