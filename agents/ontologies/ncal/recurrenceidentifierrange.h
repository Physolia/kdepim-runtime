#ifndef _NCAL_RECURRENCEIDENTIFIERRANGE_H_
#define _NCAL_RECURRENCEIDENTIFIERRANGE_H_

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QtCore/QDate>
#include <QtCore/QTime>
#include <QtCore/QDateTime>
#include <Soprano/Vocabulary/RDF>

#include <nepomuk2/simpleresource.h>

namespace Nepomuk2 {
namespace NCAL {
/**
 * Recurrence Identifier Range. This class has been created to 
 * provide means to express the limited set of values for the ncal:range 
 * property. See documentation for ncal:range for details. 
 */
class RecurrenceIdentifierRange
{
public:
    RecurrenceIdentifierRange(Nepomuk2::SimpleResource* res)
      : m_res(res)
    {}

    virtual ~RecurrenceIdentifierRange() {}

protected:
    virtual QUrl resourceType() const { return QUrl::fromEncoded("http://www.semanticdesktop.org/ontologies/2007/04/02/ncal#RecurrenceIdentifierRange", QUrl::StrictMode); }

private:
    Nepomuk2::SimpleResource* m_res;
};
}
}

#endif
