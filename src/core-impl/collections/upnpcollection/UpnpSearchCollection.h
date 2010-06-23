/****************************************************************************************
 * Copyright (c) 2010 Nikhil Marathe <nsm.nikhil@gmail.com>                             *
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

#ifndef UPNPSEARCHCOLLECTION_H
#define UPNPSEARCHCOLLECTION_H

#include "UpnpCollectionBase.h"

#include <QMap>
#include <QHash>
#include <QHostInfo>
#include <QPointer>
#include <QtGlobal>
#include <QSharedPointer>

#include <KIcon>
#include <KDirNotify>

namespace KIO {
  class Job;
  class ListJob;
}
class KJob;

class QTimer;

namespace Collections {

class UpnpQueryMaker;

class UpnpSearchCollection : public UpnpCollectionBase
{
  Q_OBJECT
  public:
    UpnpSearchCollection( const DeviceInfo &info );
    virtual ~UpnpSearchCollection();

    virtual void startIncrementalScan( const QString &directory = QString() );
    virtual QueryMaker* queryMaker();

    virtual KIcon icon() const { return KIcon("network-server"); }

    bool possiblyContainsTrack( const KUrl &url ) const;
  signals:

  public slots:
    virtual void startFullScan();

  private slots:
    void slotFilesChanged(const QStringList &);

  private:
    QTimer *m_fullScanTimer;
    bool m_fullScanInProgress;

};

} //namespace Collections

#endif
