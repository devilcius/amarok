// Jeff Mitchell <kde-dev@emailgoeshere.com>, (C) 2006
// License: GNU General Public License V2

#ifndef METABUNDLESAVER_H
#define METABUNDLESAVER_H

#include <QObject>
#include <QStringList>
//Added by qt3to4:
#include <QByteArray>
#include <kurl.h>    //inline functions
#include <klocale.h> //inline functions
#include <audioproperties.h>
#include "expression.h"
#include "atomicstring.h"
#include "atomicurl.h"

#include "amarok_export.h"

namespace TagLib {
    class FileRef;
}

/**
 * @class MetaBundleSaver
 * @author Jeff Mitchell <kde-dev@emailgoeshere.com>
 */

class AMAROK_EXPORT MetaBundleSaver : public QObject
{
    Q_OBJECT
public:
     MetaBundleSaver( MetaBundle *bundle );
    ~MetaBundleSaver();

    //bool scannerSafeSave( TagLib::File* file );
    TagLib::FileRef* prepareToSave();
    bool doSave();
    bool cleanupSave();
    void abortSave( const QString message );

private:
    MetaBundle *m_bundle;
    QString m_tempSavePath;
    QString m_origRenamedSavePath;
    QByteArray m_tempSaveDigest;
    TagLib::FileRef* m_saveFileref;
    char m_databuf[8192];
    Q_ULONG m_maxlen;
    bool m_cleanupNeeded;
};

#endif
