/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWebEngine module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QCoreApplication>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <QWebEngineDownloadItem>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineView>
#include <httpserver.h>
#include <waitforsignal.h>

class tst_QWebEngineDownloads : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void downloadLink_data();
    void downloadLink();
};

enum DownloadTestUserAction {
    SaveLink,
    Navigate,
};

enum DownloadTestFileAction {
    FileIsDownloaded,
    FileIsDisplayed,
};

Q_DECLARE_METATYPE(DownloadTestUserAction);
Q_DECLARE_METATYPE(DownloadTestFileAction);

void tst_QWebEngineDownloads::downloadLink_data()
{
    QTest::addColumn<DownloadTestUserAction>("userAction");
    QTest::addColumn<bool>("anchorHasDownloadAttribute");
    QTest::addColumn<QByteArray>("fileName");
    QTest::addColumn<QByteArray>("fileContents");
    QTest::addColumn<QByteArray>("fileMimeTypeDeclared");
    QTest::addColumn<QByteArray>("fileMimeTypeDetected");
    QTest::addColumn<QByteArray>("fileDisposition");
    QTest::addColumn<bool>("fileHasReferer");
    QTest::addColumn<DownloadTestFileAction>("fileAction");
    QTest::addColumn<QWebEngineDownloadItem::DownloadType>("downloadType");

    // SaveLink should always trigger a download, even for empty files.
    QTest::newRow("save link to empty file")
        /* userAction                 */ << SaveLink
        /* anchorHasDownloadAttribute */ << false
        /* fileName                   */ << QByteArrayLiteral("foo.txt")
        /* fileContents               */ << QByteArrayLiteral("")
        /* fileMimeTypeDeclared       */ << QByteArrayLiteral("")
        /* fileMimeTypeDetected       */ << QByteArrayLiteral("")
        /* fileDisposition            */ << QByteArrayLiteral("")
        /* fileHasReferer             */ << false
        /* fileAction                 */ << FileIsDownloaded
        /* downloadType               */ << QWebEngineDownloadItem::DownloadAttribute;

    // SaveLink should always trigger a download, also for text files.
    QTest::newRow("save link to text file")
        /* userAction                 */ << SaveLink
        /* anchorHasDownloadAttribute */ << false
        /* fileName                   */ << QByteArrayLiteral("foo.txt")
        /* fileContents               */ << QByteArrayLiteral("bar")
        /* fileMimeTypeDeclared       */ << QByteArrayLiteral("text/plain")
        /* fileMimeTypeDetected       */ << QByteArrayLiteral("text/plain")
        /* fileDisposition            */ << QByteArrayLiteral("")
        /* fileHasReferer             */ << false
        /* fileAction                 */ << FileIsDownloaded
        /* downloadType               */ << QWebEngineDownloadItem::DownloadAttribute;

    // ... adding the "download" attribute should have no effect.
    QTest::newRow("save link to text file (attribute)")
        /* userAction                 */ << SaveLink
        /* anchorHasDownloadAttribute */ << true
        /* fileName                   */ << QByteArrayLiteral("foo.txt")
        /* fileContents               */ << QByteArrayLiteral("bar")
        /* fileMimeTypeDeclared       */ << QByteArrayLiteral("text/plain")
        /* fileMimeTypeDetected       */ << QByteArrayLiteral("text/plain")
        /* fileDisposition            */ << QByteArrayLiteral("")
        /* fileHasReferer             */ << false
        /* fileAction                 */ << FileIsDownloaded
        /* downloadType               */ << QWebEngineDownloadItem::DownloadAttribute;

    // ... adding the "attachment" content disposition should also have no effect.
    QTest::newRow("save link to text file (attachment)")
        /* userAction                 */ << SaveLink
        /* anchorHasDownloadAttribute */ << false
        /* fileName                   */ << QByteArrayLiteral("foo.txt")
        /* fileContents               */ << QByteArrayLiteral("bar")
        /* fileMimeTypeDeclared       */ << QByteArrayLiteral("text/plain")
        /* fileMimeTypeDetected       */ << QByteArrayLiteral("text/plain")
        /* fileDisposition            */ << QByteArrayLiteral("attachment")
        /* fileHasReferer             */ << false
        /* fileAction                 */ << FileIsDownloaded
        /* downloadType               */ << QWebEngineDownloadItem::UserRequested;

    // ... even adding both should have no effect.
    QTest::newRow("save link to text file (attribute+attachment)")
        /* userAction                 */ << SaveLink
        /* anchorHasDownloadAttribute */ << true
        /* fileName                   */ << QByteArrayLiteral("foo.txt")
        /* fileContents               */ << QByteArrayLiteral("bar")
        /* fileMimeTypeDeclared       */ << QByteArrayLiteral("text/plain")
        /* fileMimeTypeDetected       */ << QByteArrayLiteral("text/plain")
        /* fileDisposition            */ << QByteArrayLiteral("attachment")
        /* fileHasReferer             */ << false
        /* fileAction                 */ << FileIsDownloaded
        /* downloadType               */ << QWebEngineDownloadItem::UserRequested;

    // Navigating to an empty file should show an empty page.
    QTest::newRow("navigate to empty file")
        /* userAction                 */ << Navigate
        /* anchorHasDownloadAttribute */ << false
        /* fileName                   */ << QByteArrayLiteral("foo.txt")
        /* fileContents               */ << QByteArrayLiteral("")
        /* fileMimeTypeDeclared       */ << QByteArrayLiteral("")
        /* fileMimeTypeDetected       */ << QByteArrayLiteral("")
        /* fileDisposition            */ << QByteArrayLiteral("")
        /* fileHasReferer             */ << true
        /* fileAction                 */ << FileIsDisplayed
        /* downloadType               */ << /* unused */ QWebEngineDownloadItem::DownloadAttribute;

    // Navigating to a text file should show the text file.
    QTest::newRow("navigate to text file")
        /* userAction                 */ << Navigate
        /* anchorHasDownloadAttribute */ << false
        /* fileName                   */ << QByteArrayLiteral("foo.txt")
        /* fileContents               */ << QByteArrayLiteral("bar")
        /* fileMimeTypeDeclared       */ << QByteArrayLiteral("text/plain")
        /* fileMimeTypeDetected       */ << QByteArrayLiteral("text/plain")
        /* fileDisposition            */ << QByteArrayLiteral("")
        /* fileHasReferer             */ << true
        /* fileAction                 */ << FileIsDisplayed
        /* downloadType               */ << /* unused */ QWebEngineDownloadItem::DownloadAttribute;

    // ... unless the link has the "download" attribute: then the file should be downloaded.
    QTest::newRow("navigate to text file (attribute)")
        /* userAction                 */ << Navigate
        /* anchorHasDownloadAttribute */ << true
        /* fileName                   */ << QByteArrayLiteral("foo.txt")
        /* fileContents               */ << QByteArrayLiteral("bar")
        /* fileMimeTypeDeclared       */ << QByteArrayLiteral("text/plain")
        /* fileMimeTypeDetected       */ << QByteArrayLiteral("text/plain")
        /* fileDisposition            */ << QByteArrayLiteral("")
        /* fileHasReferer             */ << false
        /* fileAction                 */ << FileIsDownloaded
        /* downloadType               */ << QWebEngineDownloadItem::DownloadAttribute;

    // ... same with the content disposition header save for the download type.
    QTest::newRow("navigate to text file (attachment)")
        /* userAction                 */ << Navigate
        /* anchorHasDownloadAttribute */ << false
        /* fileName                   */ << QByteArrayLiteral("foo.txt")
        /* fileContents               */ << QByteArrayLiteral("bar")
        /* fileMimeTypeDeclared       */ << QByteArrayLiteral("text/plain")
        /* fileMimeTypeDetected       */ << QByteArrayLiteral("text/plain")
        /* fileDisposition            */ << QByteArrayLiteral("attachment")
        /* fileHasReferer             */ << true
        /* fileAction                 */ << FileIsDownloaded
        /* downloadType               */ << QWebEngineDownloadItem::Attachment;

    // ... and both.
    QTest::newRow("navigate to text file (attribute+attachment)")
        /* userAction                 */ << Navigate
        /* anchorHasDownloadAttribute */ << true
        /* fileName                   */ << QByteArrayLiteral("foo.txt")
        /* fileContents               */ << QByteArrayLiteral("bar")
        /* fileMimeTypeDeclared       */ << QByteArrayLiteral("text/plain")
        /* fileMimeTypeDetected       */ << QByteArrayLiteral("text/plain")
        /* fileDisposition            */ << QByteArrayLiteral("attachment")
        /* fileHasReferer             */ << false
        /* fileAction                 */ << FileIsDownloaded
        /* downloadType               */ << QWebEngineDownloadItem::Attachment;

    // The file's extension has no effect.
    QTest::newRow("navigate to supposed zip file")
        /* userAction                 */ << Navigate
        /* anchorHasDownloadAttribute */ << false
        /* fileName                   */ << QByteArrayLiteral("foo.zip")
        /* fileContents               */ << QByteArrayLiteral("bar")
        /* fileMimeTypeDeclared       */ << QByteArrayLiteral("")
        /* fileMimeTypeDetected       */ << QByteArrayLiteral("")
        /* fileDisposition            */ << QByteArrayLiteral("")
        /* fileHasReferer             */ << true
        /* fileAction                 */ << FileIsDisplayed
        /* downloadType               */ << /* unused */ QWebEngineDownloadItem::DownloadAttribute;

    // ... the file's mime type however does.
    QTest::newRow("navigate to supposed zip file (application/zip)")
        /* userAction                 */ << Navigate
        /* anchorHasDownloadAttribute */ << false
        /* fileName                   */ << QByteArrayLiteral("foo.zip")
        /* fileContents               */ << QByteArrayLiteral("bar")
        /* fileMimeTypeDeclared       */ << QByteArrayLiteral("application/zip")
        /* fileMimeTypeDetected       */ << QByteArrayLiteral("application/zip")
        /* fileDisposition            */ << QByteArrayLiteral("")
        /* fileHasReferer             */ << true
        /* fileAction                 */ << FileIsDownloaded
        /* downloadType               */ << QWebEngineDownloadItem::DownloadAttribute;

    // ... but we're not very picky about the exact type.
    QTest::newRow("navigate to supposed zip file (application/octet-stream)")
        /* userAction                 */ << Navigate
        /* anchorHasDownloadAttribute */ << false
        /* fileName                   */ << QByteArrayLiteral("foo.zip")
        /* fileContents               */ << QByteArrayLiteral("bar")
        /* fileMimeTypeDeclared       */ << QByteArrayLiteral("application/octet-stream")
        /* fileMimeTypeDetected       */ << QByteArrayLiteral("application/octet-stream")
        /* fileDisposition            */ << QByteArrayLiteral("")
        /* fileHasReferer             */ << true
        /* fileAction                 */ << FileIsDownloaded
        /* downloadType               */ << QWebEngineDownloadItem::DownloadAttribute;

    // empty zip file (consisting only of "end of central directory record")
    QByteArray zipFile = QByteArrayLiteral("PK\x05\x06") + QByteArray(18, 0);

    // The mime type is guessed automatically if not provided by the server.
    QTest::newRow("navigate to actual zip file")
        /* userAction                 */ << Navigate
        /* anchorHasDownloadAttribute */ << false
        /* fileName                   */ << QByteArrayLiteral("foo.zip")
        /* fileContents               */ << zipFile
        /* fileMimeTypeDeclared       */ << QByteArrayLiteral("")
        /* fileMimeTypeDetected       */ << QByteArrayLiteral("application/octet-stream")
        /* fileDisposition            */ << QByteArrayLiteral("")
        /* fileHasReferer             */ << true
        /* fileAction                 */ << FileIsDownloaded
        /* downloadType               */ << QWebEngineDownloadItem::DownloadAttribute;

    // The mime type is not guessed automatically if provided by the server.
    QTest::newRow("navigate to actual zip file (application/zip)")
        /* userAction                 */ << Navigate
        /* anchorHasDownloadAttribute */ << false
        /* fileName                   */ << QByteArrayLiteral("foo.zip")
        /* fileContents               */ << zipFile
        /* fileMimeTypeDeclared       */ << QByteArrayLiteral("application/zip")
        /* fileMimeTypeDetected       */ << QByteArrayLiteral("application/zip")
        /* fileDisposition            */ << QByteArrayLiteral("")
        /* fileHasReferer             */ << true
        /* fileAction                 */ << FileIsDownloaded
        /* downloadType               */ << QWebEngineDownloadItem::DownloadAttribute;
}

void tst_QWebEngineDownloads::downloadLink()
{
    QFETCH(DownloadTestUserAction, userAction);
    QFETCH(bool, anchorHasDownloadAttribute);
    QFETCH(QByteArray, fileName);
    QFETCH(QByteArray, fileContents);
    QFETCH(QByteArray, fileMimeTypeDeclared);
    QFETCH(QByteArray, fileMimeTypeDetected);
    QFETCH(QByteArray, fileDisposition);
    QFETCH(bool, fileHasReferer);
    QFETCH(DownloadTestFileAction, fileAction);
    QFETCH(QWebEngineDownloadItem::DownloadType, downloadType);

    HttpServer server;
    QWebEngineProfile profile;
    QWebEnginePage page(&profile);
    QWebEngineView view;
    view.setPage(&page);

    // 1. Load an HTML page with a link
    //
    // The only variation being whether the <a> element has a "download"
    // attribute or not.
    view.load(server.url());
    view.show();
    auto indexRR = waitForRequest(&server);
    QVERIFY(indexRR);
    QCOMPARE(indexRR->requestMethod(), QByteArrayLiteral("GET"));
    QCOMPARE(indexRR->requestPath(), QByteArrayLiteral("/"));
    indexRR->setResponseHeader(QByteArrayLiteral("content-type"), QByteArrayLiteral("text/html"));
    QByteArray html;
    html += QByteArrayLiteral("<html><body><a href=\"");
    html += fileName;
    html += QByteArrayLiteral("\" ");
    if (anchorHasDownloadAttribute)
        html += QByteArrayLiteral("download");
    html += QByteArrayLiteral(">Link</a></body></html>");
    indexRR->setResponseBody(html);
    indexRR->sendResponse();
    bool loadOk = false;
    QVERIFY(waitForSignal(&page, &QWebEnginePage::loadFinished, [&](bool ok) { loadOk = ok; }));
    QVERIFY(loadOk);

    // 1.1. Ignore favicon request
    auto favIconRR = waitForRequest(&server);
    QVERIFY(favIconRR);
    QCOMPARE(favIconRR->requestMethod(), QByteArrayLiteral("GET"));
    QCOMPARE(favIconRR->requestPath(), QByteArrayLiteral("/favicon.ico"));
    favIconRR->setResponseStatus(404);
    favIconRR->sendResponse();

    // 2. Simulate user action
    //
    // - Navigate: user left-clicks on link
    // - SaveLink: user right-clicks on link and chooses "save link as" from menu
    QWidget *renderWidget = view.focusWidget();
    QPoint linkPos(10, 10);
    if (userAction == SaveLink) {
        view.setContextMenuPolicy(Qt::CustomContextMenu);
        auto event1 = new QContextMenuEvent(QContextMenuEvent::Mouse, linkPos);
        auto event2 = new QMouseEvent(QEvent::MouseButtonPress, linkPos, Qt::RightButton, {}, {});
        auto event3 = new QMouseEvent(QEvent::MouseButtonRelease, linkPos, Qt::RightButton, {}, {});
        QCoreApplication::postEvent(renderWidget, event1);
        QCoreApplication::postEvent(renderWidget, event2);
        QCoreApplication::postEvent(renderWidget, event3);
        QVERIFY(waitForSignal(&view, &QWidget::customContextMenuRequested));
        page.triggerAction(QWebEnginePage::DownloadLinkToDisk);
    } else
        QTest::mouseClick(renderWidget, Qt::LeftButton, {}, linkPos);

    // 3. Deliver requested file
    //
    // Request/response headers vary.
    auto fileRR = waitForRequest(&server);
    QVERIFY(fileRR);
    QCOMPARE(fileRR->requestMethod(), QByteArrayLiteral("GET"));
    QCOMPARE(fileRR->requestPath(), QByteArrayLiteral("/") + fileName);
    if (fileHasReferer)
        QCOMPARE(fileRR->requestHeader(QByteArrayLiteral("referer")), server.url().toEncoded());
    else
        QCOMPARE(fileRR->requestHeader(QByteArrayLiteral("referer")), QByteArray());
    if (!fileDisposition.isEmpty())
        fileRR->setResponseHeader(QByteArrayLiteral("content-disposition"), fileDisposition);
    if (!fileMimeTypeDeclared.isEmpty())
        fileRR->setResponseHeader(QByteArrayLiteral("content-type"), fileMimeTypeDeclared);
    fileRR->setResponseBody(fileContents);
    fileRR->sendResponse();

    // 4a. File is displayed and not downloaded - end test
    if (fileAction == FileIsDisplayed) {
        QVERIFY(waitForSignal(&page, &QWebEnginePage::loadFinished, [&](bool ok) { loadOk = ok; }));
        QVERIFY(loadOk);
        return;
    }

    // 4b. File is downloaded - check QWebEngineDownloadItem attributes
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    QByteArray slashFileName = QByteArrayLiteral("/") + fileName;
    QString suggestedPath =
        QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) + slashFileName;
    QString downloadPath = tmpDir.path() + slashFileName;
    QUrl downloadUrl = server.url(slashFileName);
    QWebEngineDownloadItem *downloadItem = nullptr;
    QVERIFY(waitForSignal(&profile, &QWebEngineProfile::downloadRequested,
                          [&](QWebEngineDownloadItem *item) {
        QCOMPARE(item->state(), QWebEngineDownloadItem::DownloadRequested);
        QCOMPARE(item->isFinished(), false);
        QCOMPARE(item->totalBytes(), -1);
        QCOMPARE(item->receivedBytes(), 0);
        QCOMPARE(item->interruptReason(), QWebEngineDownloadItem::NoReason);
        QCOMPARE(item->type(), downloadType);
        QCOMPARE(item->mimeType(), QString(fileMimeTypeDetected));
        QCOMPARE(item->path(), suggestedPath);
        QCOMPARE(item->savePageFormat(), QWebEngineDownloadItem::UnknownSaveFormat);
        QCOMPARE(item->url(), downloadUrl);
        item->setPath(downloadPath);
        item->accept();
        downloadItem = item;
    }));
    QVERIFY(downloadItem);
    bool finishOk = false;
    QVERIFY(waitForSignal(downloadItem, &QWebEngineDownloadItem::finished, [&]() {
        auto item = downloadItem;
        QCOMPARE(item->state(), QWebEngineDownloadItem::DownloadCompleted);
        QCOMPARE(item->isFinished(), true);
        QCOMPARE(item->totalBytes(), fileContents.size());
        QCOMPARE(item->receivedBytes(), fileContents.size());
        QCOMPARE(item->interruptReason(), QWebEngineDownloadItem::NoReason);
        QCOMPARE(item->type(), downloadType);
        QCOMPARE(item->mimeType(), QString(fileMimeTypeDetected));
        QCOMPARE(item->path(), downloadPath);
        QCOMPARE(item->savePageFormat(), QWebEngineDownloadItem::UnknownSaveFormat);
        QCOMPARE(item->url(), downloadUrl);
        finishOk = true;
    }));
    QVERIFY(finishOk);

    // 5. Check actual file contents
    QFile file(downloadPath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), fileContents);
}

QTEST_MAIN(tst_QWebEngineDownloads)
#include "tst_qwebenginedownloads.moc"
