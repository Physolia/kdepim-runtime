/*
    SPDX-FileCopyrightText: 2017 Krzysztof Nowicki <krissn@op.pl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QTest>
#include <QTimeZone>
#include <QXmlStreamReader>

#include "ewsattachment.h"
#include "fakehttppost.h"

class UtEwsAttachment : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void read();
    void read_data();
    void write();
    void write_data();
};

Q_DECLARE_METATYPE(EwsAttachment::Type)

static const QString xmlMsgNsUri = QStringLiteral("http://schemas.microsoft.com/exchange/services/2006/message");
static const QString xmlTypeNsUri = QStringLiteral("http://schemas.microsoft.com/exchange/services/2006/types");

static const QString xmlHead = QStringLiteral("<?xml version=\"1.0\"?>");
static const QString xmlDocHead = xmlHead + QLatin1String("<Test xmlns=\"") + xmlTypeNsUri + QLatin1String("\">");
static const QString xmlDocTail = QStringLiteral("</Test><Test2/>");
static const QString xmlItemAttHead = xmlDocHead + QStringLiteral("<ItemAttachment>");
static const QString xmlItemAttTail = QLatin1String("</ItemAttachment>") + xmlDocTail;
static const QString xmlFileAttHead = xmlDocHead + QStringLiteral("<FileAttachment>");
static const QString xmlFileAttTail = QLatin1String("</FileAttachment>") + xmlDocTail;

void UtEwsAttachment::read()
{
    QFETCH(QString, xml);
    QFETCH(bool, isValid);

    QXmlStreamReader reader(xml);

    reader.readNextStartElement();
    reader.readNextStartElement();

    EwsAttachment att(reader);

    QCOMPARE(att.isValid(), isValid);

    if (!isValid) {
        return;
    }

    QFETCH(EwsAttachment::Type, type);

    QCOMPARE(att.type(), type);

    QFETCH(bool, hasId);
    QFETCH(QString, id);
    QCOMPARE(att.hasId(), hasId);
    if (hasId) {
        QCOMPARE(att.id(), id);
    }

    QFETCH(bool, hasName);
    QFETCH(QString, name);
    QCOMPARE(att.hasName(), hasName);
    if (hasName) {
        QCOMPARE(att.name(), name);
    }

    QFETCH(bool, hasContentType);
    QFETCH(QString, contentType);
    QCOMPARE(att.hasContentType(), hasContentType);
    if (hasContentType) {
        QCOMPARE(att.contentType(), contentType);
    }

    QFETCH(bool, hasContentId);
    QFETCH(QString, contentId);
    QCOMPARE(att.hasContentId(), hasContentId);
    if (hasContentId) {
        QCOMPARE(att.contentId(), contentId);
    }

    QFETCH(bool, hasContentLocation);
    QFETCH(QString, contentLocation);
    QCOMPARE(att.hasContentLocation(), hasContentLocation);
    if (hasContentLocation) {
        QCOMPARE(att.contentLocation(), contentLocation);
    }

    QFETCH(bool, hasSize);
    QFETCH(long, size);
    QCOMPARE(att.hasSize(), hasSize);
    if (hasSize) {
        QCOMPARE(att.size(), size);
    }

    QFETCH(bool, hasLastModifiedTime);
    QFETCH(QDateTime, lastModifiedTime);
    QCOMPARE(att.hasLastModifiedTime(), hasLastModifiedTime);
    if (hasLastModifiedTime) {
        QCOMPARE(att.lastModifiedTime(), lastModifiedTime);
    }

    QFETCH(bool, hasIsInline);
    QFETCH(bool, isInline);
    QCOMPARE(att.hasIsInline(), hasIsInline);
    if (hasIsInline) {
        QCOMPARE(att.isInline(), isInline);
    }

    QFETCH(bool, hasIsContactPhoto);
    QFETCH(bool, isContactPhoto);
    QCOMPARE(att.hasIsContactPhoto(), hasIsContactPhoto);
    if (hasIsContactPhoto) {
        QCOMPARE(att.isContactPhoto(), isContactPhoto);
    }

    QFETCH(bool, hasContent);
    QFETCH(QByteArray, content);
    QCOMPARE(att.hasContent(), hasContent);
    if (hasContent) {
        QCOMPARE(att.content(), content);
    }

    QFETCH(bool, hasItem);
    QFETCH(EwsItem, item);
    QCOMPARE(att.hasItem(), hasItem);
    if (hasItem) {
        QCOMPARE(att.item(), item);
    }
    reader.skipCurrentElement();

    reader.readNextStartElement();
    QCOMPARE(reader.name().toString(), QStringLiteral("Test2"));
}

void UtEwsAttachment::read_data()
{
    QTest::addColumn<QString>("xml");
    QTest::addColumn<bool>("isValid");

    QTest::addColumn<EwsAttachment::Type>("type");

    QTest::addColumn<bool>("hasId");
    QTest::addColumn<QString>("id");

    QTest::addColumn<bool>("hasName");
    QTest::addColumn<QString>("name");

    QTest::addColumn<bool>("hasContentType");
    QTest::addColumn<QString>("contentType");

    QTest::addColumn<bool>("hasContentId");
    QTest::addColumn<QString>("contentId");

    QTest::addColumn<bool>("hasContentLocation");
    QTest::addColumn<QString>("contentLocation");

    QTest::addColumn<bool>("hasSize");
    QTest::addColumn<long>("size");

    QTest::addColumn<bool>("hasLastModifiedTime");
    QTest::addColumn<QDateTime>("lastModifiedTime");

    QTest::addColumn<bool>("hasIsInline");
    QTest::addColumn<bool>("isInline");

    QTest::addColumn<bool>("hasIsContactPhoto");
    QTest::addColumn<bool>("isContactPhoto");

    QTest::addColumn<bool>("hasContent");
    QTest::addColumn<QByteArray>("content");

    QTest::addColumn<bool>("hasItem");
    QTest::addColumn<EwsItem>("item");

    QTest::newRow("invalid namespace") << xmlDocHead + QLatin1String("<FileAttachment xmlns=\"") + xmlMsgNsUri + QLatin1String("\" />") + xmlDocTail << false
                                       << EwsAttachment::UnknownAttachment << false << QString() << false << QString() << false << QString() << false
                                       << QString() << false << QString() << false << 0l << false << QDateTime() << false << false << false << false << false
                                       << QByteArray() << false << EwsItem();

    QTest::newRow("invalid type") << xmlDocHead + QStringLiteral("<TestAttachment />") + xmlDocTail << false << EwsAttachment::UnknownAttachment << false
                                  << QString() << false << QString() << false << QString() << false << QString() << false << QString() << false << 0l << false
                                  << QDateTime() << false << false << false << false << false << QByteArray() << false << EwsItem();

    QTest::newRow("empty file attachment") << xmlDocHead + QStringLiteral("<FileAttachment />") + xmlDocTail << true << EwsAttachment::FileAttachment << false
                                           << QString() << false << QString() << false << QString() << false << QString() << false << QString() << false << 0l
                                           << false << QDateTime() << false << false << false << false << false << QByteArray() << false << EwsItem();

    QTest::newRow("empty item attachment") << xmlDocHead + QStringLiteral("<ItemAttachment />") + xmlDocTail << true << EwsAttachment::ItemAttachment << false
                                           << QString() << false << QString() << false << QString() << false << QString() << false << QString() << false << 0l
                                           << false << QDateTime() << false << false << false << false << false << QByteArray() << false << EwsItem();

    QTest::newRow("empty reference attachment") << xmlDocHead + QStringLiteral("<ReferenceAttachment />") + xmlDocTail << true
                                                << EwsAttachment::ReferenceAttachment << false << QString() << false << QString() << false << QString() << false
                                                << QString() << false << QString() << false << 0l << false << QDateTime() << false << false << false << false
                                                << false << QByteArray() << false << EwsItem();

    QTest::newRow("invalid attachment id - bad namespace")
        << xmlFileAttHead + QStringLiteral("<AttachmentId xmlns=\"") + xmlMsgNsUri + QStringLiteral("\" Id=\"JCPhyc4Kg73tIuurR3c0Pw==\" />") + xmlFileAttTail
        << false << EwsAttachment::UnknownAttachment << false << QString() << false << QString() << false << QString() << false << QString() << false
        << QString() << false << 0l << false << QDateTime() << false << false << false << false << false << QByteArray() << false << EwsItem();

    QTest::newRow("invalid attachment id - bad attribute")
        << xmlFileAttHead + QStringLiteral("<AttachmentId TestId=\"JCPhyc4Kg73tIuurR3c0Pw==\" />") + xmlFileAttTail << false << EwsAttachment::UnknownAttachment
        << false << QString() << false << QString() << false << QString() << false << QString() << false << QString() << false << 0l << false << QDateTime()
        << false << false << false << false << false << QByteArray() << false << EwsItem();

    QTest::newRow("valid attachment id") << xmlFileAttHead + QStringLiteral("<AttachmentId Id=\"JCPhyc4Kg73tIuurR3c0Pw==\" />") + xmlFileAttTail << true
                                         << EwsAttachment::FileAttachment << true << QStringLiteral("JCPhyc4Kg73tIuurR3c0Pw==") << false << QString() << false
                                         << QString() << false << QString() << false << QString() << false << 0l << false << QDateTime() << false << false
                                         << false << false << false << QByteArray() << false << EwsItem();

    QTest::newRow("invalid name") << xmlFileAttHead + QStringLiteral("<Name>Test <b>name</b></Name>") + xmlFileAttTail << false
                                  << EwsAttachment::UnknownAttachment << false << QString() << false << QString() << false << QString() << false << QString()
                                  << false << QString() << false << 0l << false << QDateTime() << false << false << false << false << false << QByteArray()
                                  << false << EwsItem();

    QTest::newRow("valid name") << xmlFileAttHead + QStringLiteral("<Name>Test name</Name>") + xmlFileAttTail << true << EwsAttachment::FileAttachment << false
                                << QString() << true << QStringLiteral("Test name") << false << QString() << false << QString() << false << QString() << false
                                << 0l << false << QDateTime() << false << false << false << false << false << QByteArray() << false << EwsItem();

    QTest::newRow("valid content type") << xmlFileAttHead + QStringLiteral("<ContentType>application/x-test</ContentType>") + xmlFileAttTail << true
                                        << EwsAttachment::FileAttachment << false << QString() << false << QString() << true
                                        << QStringLiteral("application/x-test") << false << QString() << false << QString() << false << 0l << false
                                        << QDateTime() << false << false << false << false << false << QByteArray() << false << EwsItem();

    QTest::newRow("valid content id") << xmlFileAttHead + QStringLiteral("<ContentId>FE938BD618330B9DA0C965A6077BB3FF20415531@1</ContentId>") + xmlFileAttTail
                                      << true << EwsAttachment::FileAttachment << false << QString() << false << QString() << false << QString() << true
                                      << QStringLiteral("FE938BD618330B9DA0C965A6077BB3FF20415531@1") << false << QString() << false << 0l << false
                                      << QDateTime() << false << false << false << false << false << QByteArray() << false << EwsItem();

    QTest::newRow("valid content location") << xmlFileAttHead + QStringLiteral("<ContentLocation>file:///foo/bar.txt</ContentLocation>") + xmlFileAttTail
                                            << true << EwsAttachment::FileAttachment << false << QString() << false << QString() << false << QString() << false
                                            << QString() << true << QStringLiteral("file:///foo/bar.txt") << false << 0l << false << QDateTime() << false
                                            << false << false << false << false << QByteArray() << false << EwsItem();

    QTest::newRow("invalid size - not a number") << xmlFileAttHead + QStringLiteral("<Size>foo</Size>") + xmlFileAttTail << false
                                                 << EwsAttachment::UnknownAttachment << false << QString() << false << QString() << false << QString() << false
                                                 << QString() << false << QString() << false << 0l << false << QDateTime() << false << false << false << false
                                                 << false << QByteArray() << false << EwsItem();

    QTest::newRow("valid size") << xmlFileAttHead + QStringLiteral("<Size>123</Size>") + xmlFileAttTail << true << EwsAttachment::FileAttachment << false
                                << QString() << false << QString() << false << QString() << false << QString() << false << QString() << true << 123l << false
                                << QDateTime() << false << false << false << false << false << QByteArray() << false << EwsItem();

    QTest::newRow("invalid last modified time - bad time")
        << xmlFileAttHead + QStringLiteral("<LastModifiedTime>2017-01-03T09:74:39</LastModifiedTime>") + xmlFileAttTail << false
        << EwsAttachment::UnknownAttachment << false << QString() << false << QString() << false << QString() << false << QString() << false << QString()
        << false << 0l << false << QDateTime() << false << false << false << false << false << QByteArray() << false << EwsItem();

    QTest::newRow("invalid last modified time - nested XML element")
        << xmlFileAttHead + QStringLiteral("<LastModifiedTime>2017-<b>01</b>-03T09:74:39</LastModifiedTime>") + xmlFileAttTail << false
        << EwsAttachment::UnknownAttachment << false << QString() << false << QString() << false << QString() << false << QString() << false << QString()
        << false << 0l << false << QDateTime() << false << false << false << false << false << QByteArray() << false << EwsItem();

    QTest::newRow("valid last modified time") << xmlFileAttHead + QStringLiteral("<LastModifiedTime>2017-01-03T08:24:39Z</LastModifiedTime>") + xmlFileAttTail
                                              << true << EwsAttachment::FileAttachment << false << QString() << false << QString() << false << QString()
                                              << false << QString() << false << QString() << false << 0l << true
                                              << QDateTime::fromSecsSinceEpoch(1483431879, QTimeZone::utc()) << false << false << false << false << false
                                              << QByteArray() << false << EwsItem();

    QTest::newRow("invalid is inline - bad value") << xmlFileAttHead + QStringLiteral("<IsInline>fake</IsInline>") + xmlFileAttTail << false
                                                   << EwsAttachment::UnknownAttachment << false << QString() << false << QString() << false << QString()
                                                   << false << QString() << false << QString() << false << 0l << false << QDateTime() << false << false << false
                                                   << false << false << QByteArray() << false << EwsItem();

    QTest::newRow("valid is inline") << xmlFileAttHead + QStringLiteral("<IsInline>true</IsInline>") + xmlFileAttTail << true << EwsAttachment::FileAttachment
                                     << false << QString() << false << QString() << false << QString() << false << QString() << false << QString() << false
                                     << 0l << false << QDateTime() << true << true << false << false << false << QByteArray() << false << EwsItem();

    QTest::newRow("invalid is contact photo - bad value")
        << xmlFileAttHead + QStringLiteral("<IsContactPhoto>fake</IsContactPhoto>") + xmlFileAttTail << false << EwsAttachment::UnknownAttachment << false
        << QString() << false << QString() << false << QString() << false << QString() << false << QString() << false << 0l << false << QDateTime() << false
        << false << false << false << false << QByteArray() << false << EwsItem();

    QTest::newRow("invalid is contact photo - inside ItemAttachment")
        << xmlItemAttHead + QStringLiteral("<IsContactPhoto>true</IsContactPhoto>") + xmlItemAttTail << false << EwsAttachment::UnknownAttachment << false
        << QString() << false << QString() << false << QString() << false << QString() << false << QString() << false << 0l << false << QDateTime() << false
        << false << false << false << false << QByteArray() << false << EwsItem();

    QTest::newRow("valid is contact photo") << xmlFileAttHead + QStringLiteral("<IsContactPhoto>true</IsContactPhoto>") + xmlFileAttTail << true
                                            << EwsAttachment::FileAttachment << false << QString() << false << QString() << false << QString() << false
                                            << QString() << false << QString() << false << 0l << false << QDateTime() << false << false << true << true << false
                                            << QByteArray() << false << EwsItem();

    QTest::newRow("invalid content - nested XML element")
        << xmlFileAttHead + QStringLiteral("<Content>djsaoij<b>sa</b></Content>") + xmlFileAttTail << false << EwsAttachment::UnknownAttachment << false
        << QString() << false << QString() << false << QString() << false << QString() << false << QString() << false << 0l << false << QDateTime() << false
        << false << false << false << false << QByteArray() << false << EwsItem();

    QTest::newRow("invalid content - inside ItemAttachment")
        << xmlItemAttHead + QStringLiteral("<Content>VGhpcyBpcyBhIHRlc3Q=</Content>") + xmlItemAttTail << false << EwsAttachment::UnknownAttachment << false
        << QString() << false << QString() << false << QString() << false << QString() << false << QString() << false << 0l << false << QDateTime() << false
        << false << false << false << false << QByteArray() << false << EwsItem();

    QTest::newRow("valid content") << xmlFileAttHead + QStringLiteral("<Content>VGhpcyBpcyBhIHRlc3Q=</Content>") + xmlFileAttTail << true
                                   << EwsAttachment::FileAttachment << false << QString() << false << QString() << false << QString() << false << QString()
                                   << false << QString() << false << 0l << false << QDateTime() << false << false << false << false << true
                                   << QByteArray("This is a test") << false << EwsItem();

    QTest::newRow("invalid item - bad data") << xmlItemAttHead + QStringLiteral("<Akonadi/Item><foo></foo></Item>") + xmlItemAttTail << false
                                             << EwsAttachment::UnknownAttachment << false << QString() << false << QString() << false << QString() << false
                                             << QString() << false << QString() << false << 0l << false << QDateTime() << false << false << false << false
                                             << false << QByteArray() << false << EwsItem();

    QTest::newRow("invalid item - inside FileAttachment")
        << xmlFileAttHead + QStringLiteral("<Akonadi/Item><ItemId Id=\"VGhpcyBpcyBhIHRlc3Q=\" ChangeKey=\"muKls0n8pUM=\" /></Item>") + xmlFileAttTail << false
        << EwsAttachment::UnknownAttachment << false << QString() << false << QString() << false << QString() << false << QString() << false << QString()
        << false << 0l << false << QDateTime() << false << false << false << false << false << QByteArray() << false << EwsItem();

    EwsItem item1;
    item1.setType(EwsItemTypeItem);
    item1.setField(EwsItemFieldItemId, QVariant::fromValue<EwsId>(EwsId(QStringLiteral("VGhpcyBpcyBhIHRlc3Q="), QStringLiteral("muKls0n8pUM="))));
    QTest::newRow("valid item") << xmlItemAttHead + QStringLiteral("<Akonadi/Item><ItemId Id=\"VGhpcyBpcyBhIHRlc3Q=\" ChangeKey=\"muKls0n8pUM=\" /></Item>")
            + xmlItemAttTail << true
                                << EwsAttachment::ItemAttachment << false << QString() << false << QString() << false << QString() << false << QString()
                                << false << QString() << false << 0l << false << QDateTime() << false << false << false << false << false << QByteArray()
                                << true << item1;
}

void UtEwsAttachment::write()
{
    QFETCH(EwsAttachment::Type, type);
    QFETCH(bool, isValid);

    EwsAttachment att;
    if (isValid) {
        att.setType(type);

        QFETCH(bool, hasId);
        QFETCH(QString, id);
        if (hasId) {
            att.setId(id);
        }

        QFETCH(bool, hasName);
        QFETCH(QString, name);
        if (hasName) {
            att.setName(name);
        }

        QFETCH(bool, hasContentType);
        QFETCH(QString, contentType);
        if (hasContentType) {
            att.setContentType(contentType);
        }

        QFETCH(bool, hasContentId);
        QFETCH(QString, contentId);
        if (hasContentId) {
            att.setContentId(contentId);
        }

        QFETCH(bool, hasContentLocation);
        QFETCH(QString, contentLocation);
        if (hasContentLocation) {
            att.setContentLocation(contentLocation);
        }

        QFETCH(bool, hasSize);
        QFETCH(long, size);
        if (hasSize) {
            att.setSize(size);
        }

        QFETCH(bool, hasLastModifiedTime);
        QFETCH(QDateTime, lastModifiedTime);
        if (hasLastModifiedTime) {
            att.setLastModifiedTime(lastModifiedTime);
        }

        QFETCH(bool, hasIsInline);
        QFETCH(bool, isInline);
        if (hasIsInline) {
            att.setIsInline(isInline);
        }

        QFETCH(bool, hasIsContactPhoto);
        QFETCH(bool, isContactPhoto);
        if (hasIsContactPhoto) {
            att.setIsContactPhoto(isContactPhoto);
        }

        QFETCH(bool, hasContent);
        QFETCH(QByteArray, content);
        if (hasContent) {
            att.setContent(content);
        }

        QFETCH(bool, hasItem);
        QFETCH(EwsItem, item);
        if (hasItem) {
            att.setItem(item);
        }
    }

    QFETCH(QString, xml);

    QString xmlData;
    QXmlStreamWriter writer(&xmlData);

    writer.setCodec("UTF-8");

    writer.writeStartDocument();

    writer.writeDefaultNamespace(xmlTypeNsUri);

    att.write(writer);

    writer.writeEndDocument();

    QCOMPARE(xmlData.trimmed(), xml);
}

void UtEwsAttachment::write_data()
{
    QTest::addColumn<QString>("xml");
    QTest::addColumn<bool>("isValid");

    QTest::addColumn<EwsAttachment::Type>("type");

    QTest::addColumn<bool>("hasId");
    QTest::addColumn<QString>("id");

    QTest::addColumn<bool>("hasName");
    QTest::addColumn<QString>("name");

    QTest::addColumn<bool>("hasContentType");
    QTest::addColumn<QString>("contentType");

    QTest::addColumn<bool>("hasContentId");
    QTest::addColumn<QString>("contentId");

    QTest::addColumn<bool>("hasContentLocation");
    QTest::addColumn<QString>("contentLocation");

    QTest::addColumn<bool>("hasSize");
    QTest::addColumn<long>("size");

    QTest::addColumn<bool>("hasLastModifiedTime");
    QTest::addColumn<QDateTime>("lastModifiedTime");

    QTest::addColumn<bool>("hasIsInline");
    QTest::addColumn<bool>("isInline");

    QTest::addColumn<bool>("hasIsContactPhoto");
    QTest::addColumn<bool>("isContactPhoto");

    QTest::addColumn<bool>("hasContent");
    QTest::addColumn<QByteArray>("content");

    QTest::addColumn<bool>("hasItem");
    QTest::addColumn<EwsItem>("item");

    QTest::newRow("invalid") << xmlHead << false << EwsAttachment::UnknownAttachment << false << QString() << false << QString() << false << QString() << false
                             << QString() << false << QString() << false << 0l << false << QDateTime() << false << false << false << false << false
                             << QByteArray() << false << EwsItem();

    QTest::newRow("unknown type") << xmlHead << true << EwsAttachment::UnknownAttachment << false << QString() << false << QString() << false << QString()
                                  << false << QString() << false << QString() << false << 0l << false << QDateTime() << false << false << false << false
                                  << false << QByteArray() << false << EwsItem();

    QTest::newRow("empty file attachment") << xmlHead + QStringLiteral("<FileAttachment xmlns=\"") + xmlTypeNsUri + QStringLiteral("\"/>") << true
                                           << EwsAttachment::FileAttachment << false << QString() << false << QString() << false << QString() << false
                                           << QString() << false << QString() << false << 0l << false << QDateTime() << false << false << false << false
                                           << false << QByteArray() << false << EwsItem();

    QTest::newRow("empty item attachment") << xmlHead + QStringLiteral("<ItemAttachment xmlns=\"") + xmlTypeNsUri + QStringLiteral("\"/>") << true
                                           << EwsAttachment::ItemAttachment << false << QString() << false << QString() << false << QString() << false
                                           << QString() << false << QString() << false << 0l << false << QDateTime() << false << false << false << false
                                           << false << QByteArray() << false << EwsItem();

    QTest::newRow("empty reference attachment") << xmlHead + QStringLiteral("<ReferenceAttachment xmlns=\"") + xmlTypeNsUri + QStringLiteral("\"/>") << true
                                                << EwsAttachment::ReferenceAttachment << false << QString() << false << QString() << false << QString() << false
                                                << QString() << false << QString() << false << 0l << false << QDateTime() << false << false << false << false
                                                << false << QByteArray() << false << EwsItem();

    QTest::newRow("non-empty reference attachment") << xmlHead + QStringLiteral("<ReferenceAttachment xmlns=\"") + xmlTypeNsUri + QStringLiteral("\"/>") << true
                                                    << EwsAttachment::ReferenceAttachment << true << QStringLiteral("5IaIqJVsJzamf2105wg4wQ==") << false
                                                    << QString() << false << QString() << false << QString() << false << QString() << false << 0l << false
                                                    << QDateTime() << false << false << false << false << false << QByteArray() << false << EwsItem();

    EwsItem item1;
    item1.setType(EwsItemTypeItem);
    item1.setField(EwsItemFieldItemId, QVariant::fromValue<EwsId>(EwsId(QStringLiteral("VGhpcyBpcyBhIHRlc3Q="), QStringLiteral("muKls0n8pUM="))));

    QDateTime testDt(QDateTime::fromSecsSinceEpoch(1483621243, QTimeZone::utc()));

    QTest::newRow("non-empty item attachment") << xmlHead + QStringLiteral("<ItemAttachment xmlns=\"") + xmlTypeNsUri
            + QStringLiteral(
                  "\">"
                  "<AttachmentId Id=\"5IaIqJVsJzamf2105wg4wQ==\"/>"
                  "<ContentType>application/x-test</ContentType>"
                  "<ContentLocation>file:///foo/bar</ContentLocation>"
                  "<LastModifiedTime>%1</LastModifiedTime>"
                  "<Akonadi/Item><ItemId Id=\"VGhpcyBpcyBhIHRlc3Q=\" ChangeKey=\"muKls0n8pUM=\"/></Item>"
                  "</ItemAttachment>")
                  .arg(testDt.toString(Qt::ISODate))
                                               << true << EwsAttachment::ItemAttachment << true << QStringLiteral("5IaIqJVsJzamf2105wg4wQ==") << false
                                               << QStringLiteral("Test attachment") << true << QStringLiteral("application/x-test") << false
                                               << QStringLiteral("FE938BD618330B9DA0C965A6077BB3FF20415531@1") << true << QStringLiteral("file:///foo/bar")
                                               << false << 123l << true << testDt << false << true << true << true << true << QByteArray("This is another test")
                                               << true << item1;

    QTest::newRow("non-empty file attachment") << xmlHead + QStringLiteral("<FileAttachment xmlns=\"") + xmlTypeNsUri
            + QStringLiteral("\">"
                             "<Name>Test attachment</Name>"
                             "<ContentId>FE938BD618330B9DA0C965A6077BB3FF20415531@1</ContentId>"
                             "<Size>123</Size>"
                             "<IsInline>true</IsInline>"
                             "<IsContactPhoto>true</IsContactPhoto>"
                             "<Content>VGhpcyBpcyBhbm90aGVyIHRlc3Q=</Content>"
                             "</FileAttachment>")
                                               << true << EwsAttachment::FileAttachment << false << QStringLiteral("5IaIqJVsJzamf2105wg4wQ==") << true
                                               << QStringLiteral("Test attachment") << false << QStringLiteral("application/x-test") << true
                                               << QStringLiteral("FE938BD618330B9DA0C965A6077BB3FF20415531@1") << false << QStringLiteral("file:///foo/bar")
                                               << true << 123l << false << testDt << true << true << true << true << true << QByteArray("This is another test")
                                               << true << item1;
}

QTEST_MAIN(UtEwsAttachment)

#include "ewsattachment_ut.moc"
