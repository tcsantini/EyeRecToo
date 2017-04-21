#ifndef REFERENCE_H
#define REFERENCE_H

#include <QList>
#include <QString>

class Reference
{
public:
    Reference(QString authors, QString title, QString venue, QString year, QString url)
    : authors(authors),
      title(title),
      venue(venue),
      year(year),
      url(url) {}

    bool operator==(const Reference &other) const {
        return ( authors == other.authors
                 && title == other.title
                 && venue == other.venue
                 && year == other.year );
    }

    QString toQString() {
        return QString( "<b>[%2 %5]</b> <a href=\"%1\">%3</a> (%4)").arg(url
            ).arg(authors
            ).arg(title
            ).arg(venue
            ).arg(year);
    }

private:
    QString authors;
    QString title;
    QString venue;
    QString year;
    QString url;
};

class ReferenceList
{
public:
    static bool add(QString authors, QString title, QString venue, QString year, QString url)
    {
        Reference newRef(authors, title, venue, year, url);
        for (int i=0; i<refList.size(); i++)
            if (newRef == refList[i])
                return false;
        refList.append(newRef);
        return true;
    }

    static QString text()
    {
        QString text;
        for (int i = 0; i < refList.size(); i++)
            text.append( refList[i].toQString() ).append("<br><br>");
        return text;
    }

private:
    static QList<Reference> refList;
};

#endif // REFERENCE_H
