/***************************************************************************
 *   Copyright (C) 2012 by Peter Penz <peter.penz19@gmail.com>             *
 *   Copyright (C) 2013 by Vishesh Handa <me@vhanda.in>                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#include "kbaloorolesprovider.h"

#include <Baloo/File>
#include <KFileMetaData/PropertyInfo>
#include <KFileMetaData/UserMetaData>
#include <KFormat>
#include <KLocalizedString>

#include <QCollator>
#include <QDebug>
#include <QTime>

struct KBalooRolesProviderSingleton
{
    KBalooRolesProvider instance;
};
Q_GLOBAL_STATIC(KBalooRolesProviderSingleton, s_balooRolesProvider)


KBalooRolesProvider& KBalooRolesProvider::instance()
{
    return s_balooRolesProvider->instance;
}

KBalooRolesProvider::~KBalooRolesProvider()
{
}

QSet<QByteArray> KBalooRolesProvider::roles() const
{
    return m_roles;
}

QHash<QByteArray, QVariant> KBalooRolesProvider::roleValues(const Baloo::File& file,
                                                            const QSet<QByteArray>& roles) const
{
    QHash<QByteArray, QVariant> values;

    QMapIterator<KFileMetaData::Property::Property, QVariant> it(file.properties());
    while (it.hasNext()) {
        it.next();

        const KFileMetaData::PropertyInfo pi(it.key());
        const QString property = pi.name();
        const QByteArray role = roleForProperty(property);
        if (role.isEmpty() || !roles.contains(role)) {
            continue;
        }

        values.insert(role, pi.formatAsDisplayString(it.value()));
    }

    KFileMetaData::UserMetaData md(file.path());
    if (roles.contains("tags")) {
        values.insert("tags", tagsFromValues(md.tags()));
    }
    if (roles.contains("rating")) {
        values.insert("rating", QString::number(md.rating()));
    }
    if (roles.contains("comment")) {
        values.insert("comment", md.userComment());
    }
    if (roles.contains("originUrl")) {
        values.insert("originUrl", md.originUrl());
    }

    return values;
}

QByteArray KBalooRolesProvider::roleForProperty(const QString& property) const
{
    return m_roleForProperty.value(property);
}

KBalooRolesProvider::KBalooRolesProvider() :
    m_roles(),
    m_roleForProperty()
{
    struct PropertyInfo
    {
        const char* const property;
        const char* const role;
    };

    // Mapping from the URIs to the KFileItemModel roles. Note that this must not be
    // a 1:1 mapping: One role may contain several URI-values
    static const PropertyInfo propertyInfoList[] = {
        { "rating", "rating" },
        { "tag",        "tags" },
        { "comment",   "comment" },
        { "title",         "title" },
        { "wordCount",     "wordCount" },
        { "lineCount",     "lineCount" },
        { "width",         "width" },
        { "height",        "height" },
        { "imageDateTime",   "imageDateTime"},
        { "imageOrientation", "orientation", },
        { "artist",     "artist" },
        { "genre",	"genre"  },
        { "album",    "album" },
        { "duration",      "duration" },
        { "bitRate", "bitrate" },
        { "aspectRatio", "aspectRatio" },
        { "frameRate", "frameRate" },
        { "releaseYear",    "releaseYear" },
        { "trackNumber",   "track" },
        { "originUrl", "originUrl" }
    };

    for (unsigned int i = 0; i < sizeof(propertyInfoList) / sizeof(PropertyInfo); ++i) {
        m_roleForProperty.insert(propertyInfoList[i].property, propertyInfoList[i].role);
        m_roles.insert(propertyInfoList[i].role);
    }
}

QString KBalooRolesProvider::tagsFromValues(const QStringList& values) const
{
    QStringList alphabeticalOrderTags = values;
    QCollator coll;
    coll.setNumericMode(true);
    std::sort(alphabeticalOrderTags.begin(), alphabeticalOrderTags.end(), [&](const QString& s1, const QString& s2){ return coll.compare(s1, s2) < 0; });
    return alphabeticalOrderTags.join(QStringLiteral(", "));
}
