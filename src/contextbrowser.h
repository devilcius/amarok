// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// See COPYING file for licensing information


#ifndef AMAROK_CONTEXTBROWSER_H
#define AMAROK_CONTEXTBROWSER_H

#include "engineobserver.h"
#include <qvbox.h>
#include <klistview.h>
#include <kparts/browserextension.h>
#include <kurl.h>

class KHTMLPart;
class QPalette;
class MetaBundle;
class CollectionDB;

class ContextBrowser : public QVBox, public EngineObserver
{
    Q_OBJECT

    public:
        ContextBrowser( const char *name );
        ~ContextBrowser();

        virtual void setFont( const QFont& );

    public slots:
        void openURLRequest(const KURL &url );

    protected:
        void engineNewMetaData( const MetaBundle&, bool );
        void engineStateChanged( Engine::State );
        void paletteChange( const QPalette& );

    private slots:
        void showHome();
        void showCurrentTrack();

    private:
        void setStyleSheet();
        void showIntroduction();
        void showScanning();

        KHTMLPart *browser;
        MetaBundle *m_currentTrack;
        CollectionDB *m_db;
        QString m_styleSheet;
        KURL m_url;
        QString m_artist;
        QString m_album;
};

#endif /* AMAROK_CONTEXTBROWSER_H */
