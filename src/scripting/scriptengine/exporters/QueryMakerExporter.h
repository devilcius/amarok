/****************************************************************************************
 * Copyright (c) 2013 Anmol Ahuja <darthcodus@gmail.com>                                *
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

#ifndef QUERYMAKER_EXPORTER_H
#define QUERYMAKER_EXPORTER_H

#include "amarok_export.h"
#include "core/meta/Meta.h"

#include <QObject>
#include <QWeakPointer>

namespace Collections
{
    class QueryMaker;
}
class QScriptEngine;
class QScriptValue;

namespace AmarokScript
{
    // SCRIPTDOX PROTOTYPE Collections::QueryMaker QueryMaker
    #ifdef DEBUG
    class AMAROK_EXPORT
    #else
    class
    #endif
    QueryMakerPrototype : public QObject
    {
        Q_OBJECT

        /**
         * Indicates whether this query maker object is valid.
         */
        Q_PROPERTY( bool isValid READ isValid )

        /**
         * The current filter string.
         */
        Q_PROPERTY( QString filter READ filter )

    public:
        static void init( QScriptEngine *engine );
        QueryMakerPrototype( Collections::QueryMaker *collection );
        ~QueryMakerPrototype();
        Collections::QueryMaker *data() { return m_querymaker.data(); }

    public slots:

        /**
         *  Starts the query. This method returns immediately. All processing is done in one or more
         *  separate worker thread(s). One of the newResultReady signals will be emitted at least once,
         *  followed by the queryDone() signal exactly once.
         */
        void run();

        /**
         * Block until the query completes, returns the tracklist..
         */
        Meta::TrackList blockingRun();

        /**
         *  Aborts a running query.
         */
        void abort();

        /**
         * Add a filter query to this query maker.
         */
        void addFilter( const QString &filter );

    private:
        QWeakPointer<Collections::QueryMaker> m_querymaker;
        QString m_filter;
        Meta::TrackList m_result;

        bool isValid() const;
        QString filter() const;

    private slots:
        void slotResult( const Meta::TrackList &tracks );

    signals:
        /**
         * newResultReady will be emitted every time new results from the query maker are received.
         * This signal can be emitted zero times (in case of no results) one (the usual case) or multiple times
         * (e.g. in case when the result is received in several batches).
         * The results will be terminated by a queryDone signal.
         */
        void newResultReady( Meta::TrackList );

        /**
         * This signal is emitted after all the results have been submitted via zero or more newResultReady signals.
         */
        void queryDone();
    };
}

#endif
