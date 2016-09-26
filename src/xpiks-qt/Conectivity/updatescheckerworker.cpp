/*
 * This file is a part of Xpiks - cross platform application for
 * keywording and uploading images for microstocks
 * Copyright (C) 2014-2016 Taras Kushnir <kushnirTV@gmail.com>
 *
 * Xpiks is distributed under the GNU General Public License, version 3.0
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "updatescheckerworker.h"

#if defined(Q_OS_DARWIN)
#define UPDATE_JSON_URL "https://ribtoks.github.io/xpiks/api/v1/update-osx.json"
#elif defined(Q_OS_WIN64)
#define UPDATE_JSON_URL "https://ribtoks.github.io/xpiks/api/v1/update-windows.json"
#elif defined(Q_OS_WIN32)
#define UPDATE_JSON_URL "https://ribtoks.github.io/xpiks/api/v1/update-windows-32.json"
#else
#define UPDATE_JSON_URL "https://ribtoks.github.io/xpiks/api/v1/update.json"
#endif

#define DEFAULT_UPDATE_URL "https://ribtoks.github.io/xpiks/downloads/"
#define UPDATE_JSON_MAJOR_VERSION "major_version"
#define UPDATE_JSON_MINOR_VERSION "minor_version"
#define UPDATE_JSON_FIX_VERSION "fix_version"
#define UPDATE_JSON_UPDATE_URL "update_link"
#define UPDATE_JSON_CHECKSUM "checksum"

#include <QString>
#include <QUrl>
#include <QDir>
#include <QFile>
#include <QThread>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtGlobal>
#include <QCryptographicHash>
#include "../Conectivity/simplecurlrequest.h"
#include "../Conectivity/simplecurldownloader.h"
#include "../Common/defines.h"
#include "../Common/version.h"
#include "../Common/defines.h"
#include "../Models/settingsmodel.h"
#include "../Models/proxysettings.h"
#include "../Helpers/constants.h"

QString fileChecksum(const QString &fileName, QCryptographicHash::Algorithm hashAlgorithm) {
    QString result;

    QFile f(fileName);
    if (f.open(QFile::ReadOnly)) {
        QCryptographicHash hash(hashAlgorithm);
        if (hash.addData(&f)) {
            result = QString::fromLatin1(hash.result().toHex()).toLower();
        }
    }

    return result;
}

bool moveFile(const QString &from, const QString &to) {
    bool success = false;

    QFile destination(to);
    if (destination.exists()) {
        if (!destination.remove()) {
            return success;
        }
    }

    QFile source(from);
    if (source.exists()) {
        success = source.rename(to);
    }

    return success;
}

namespace Conectivity {
    UpdatesCheckerWorker::UpdatesCheckerWorker(Models::SettingsModel *settingsModel):
        m_SettingsModel(settingsModel)
    {
        Q_ASSERT(settingsModel != nullptr);
    }

    UpdatesCheckerWorker::~UpdatesCheckerWorker() {
    }

    void UpdatesCheckerWorker::initWorker() {
        QString appDataPath = XPIKS_USERDATA_PATH;
        if (!appDataPath.isEmpty()) {
            m_UpdatesDirectory = QDir::cleanPath(appDataPath + QDir::separator() + Constants::UPDATES_DIRECTORY);
        } else {
            LOG_WARNING << "Can't get to the updates directory. Using temporary...";
            m_UpdatesDirectory = QDir::temp().absolutePath();
        }
    }

    void UpdatesCheckerWorker::processOneItem() {
        LOG_INFO << "Update service: checking for updates...";

        UpdateCheckResult updateCheckResult;
        if (checkForUpdates(updateCheckResult)) {
            if (m_SettingsModel->getAutoDownloadUpdates()) {
                Conectivity::SimpleCurlDownloader downloader(updateCheckResult.m_UpdateURL);
                Models::ProxySettings *proxySettings = m_SettingsModel->retrieveProxySettings();
                downloader.setProxySettings(proxySettings);

                if (downloader.downloadFileSync()) {
                    LOG_INFO << "Update downloaded" << downloader.getDownloadedPath();

                    QString downloadedPath = downloader.getDownloadedPath();
                    QString checksum = fileChecksum(downloadedPath, QCryptographicHash::Sha1);
                    if (checksum == updateCheckResult.m_Checksum) {
                        LOG_INFO << "Update checksum confirmed";

                        QDir updatesDir(m_UpdatesDirectory);
                        QString filename = QFileInfo(downloadedPath).fileName();
                        QString updatePath = updatesDir.filePath(filename);

                        if (moveFile(downloader.getDownloadedPath(), updatePath)) {
                            emit updateDownloaded(updatePath);
                        } else {
                            LOG_WARNING << "Failed to move the file";
                        }
                    } else {
                        LOG_INFO << "Checksum does not match:" << "expected" << updateCheckResult.m_Checksum << "actual" << checksum;
                    }
                } else {
                    LOG_WARNING << "Update download failed";
                }

            } else {
                emit updateAvailable(updateCheckResult.m_UpdateURL);
            }
        }

        emit stopped();
        emit requestFinished();

        LOG_INFO << "Updates checking loop finished";
    }

    bool UpdatesCheckerWorker::checkForUpdates(UpdateCheckResult &result) {
        bool anyUpdateAvailable = false;
        QString queryString = QString(UPDATE_JSON_URL);

        Models::ProxySettings *proxySettings = m_SettingsModel->retrieveProxySettings();

        Conectivity::SimpleCurlRequest request(queryString);
        request.setProxySettings(proxySettings);

        if (request.sendRequestSync()) {
            QJsonDocument document = QJsonDocument::fromJson(request.getResponseData());

            QJsonObject jsonObject = document.object();

            if (jsonObject.contains(UPDATE_JSON_MAJOR_VERSION) &&
                    jsonObject.contains(UPDATE_JSON_MINOR_VERSION) &&
                    jsonObject.contains(UPDATE_JSON_FIX_VERSION)) {

                int majorVersion = jsonObject.value(UPDATE_JSON_MAJOR_VERSION).toInt();
                int minorVersion = jsonObject.value(UPDATE_JSON_MINOR_VERSION).toInt();
                int fixVersion = jsonObject.value(UPDATE_JSON_FIX_VERSION).toInt();

                int availableVersion = majorVersion*100 + minorVersion*10 + fixVersion;
                int currVersion = XPIKS_MAJOR_VERSION*100 + XPIKS_MINOR_VERSION*10 + XPIKS_FIX_VERSION;

                LOG_INFO << "Update service: available =" << availableVersion << "current =" << currVersion;

                if (availableVersion > currVersion) {
                    QString updateUrl = DEFAULT_UPDATE_URL;
                    if (jsonObject.contains(UPDATE_JSON_UPDATE_URL)) {
                        updateUrl = jsonObject.value(UPDATE_JSON_UPDATE_URL).toString();
                    }

                    QString checksum;
                    if (jsonObject.contains(UPDATE_JSON_CHECKSUM)) {
                        checksum = jsonObject.value(UPDATE_JSON_CHECKSUM).toString();
                    }

                    result.m_UpdateURL = updateUrl;
                    result.m_Checksum = checksum;
                    anyUpdateAvailable = true;
                }
            }
        }

        return anyUpdateAvailable;
    }

    void UpdatesCheckerWorker::process() {
        initWorker();
        processOneItem();
    }
}
