/*
 * This file is a part of Xpiks - cross platform application for
 * keywording and uploading images for microstocks
 * Copyright (C) 2014-2015 Taras Kushnir <kushnirTV@gmail.com>
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

#include "logsmodel.h"

#include <QThread>
#include <QString>
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QDateTime>
#include <QTextStream>
#include <QStandardPaths>
#include "../Helpers/constants.h"
#include "../Helpers/stringhelper.h"
#include "../Helpers/logger.h"
#include "../Helpers/loggingworker.h"

namespace Models {

    LogsModel::LogsModel(QObject *parent) :
        QObject(parent)
    {
#ifdef WITH_LOGS
        Helpers::LoggingWorker *loggerWorker = new Helpers::LoggingWorker();
        QThread *loggingThread = new QThread();
        loggerWorker->moveToThread(loggingThread);

        QObject::connect(loggingThread, SIGNAL(started()), loggerWorker, SLOT(process()));
        QObject::connect(loggerWorker, SIGNAL(stopped()), loggingThread, SLOT(quit()));

        QObject::connect(loggerWorker, SIGNAL(stopped()), loggerWorker, SLOT(deleteLater()));
        QObject::connect(loggingThread, SIGNAL(finished()), loggingThread, SLOT(deleteLater()));

        QObject::connect(this, SIGNAL(stopLogsSignal()), loggerWorker, SLOT(cancel()));

        loggingThread->start();
#endif
    }

    QString LogsModel::getAllLogsText(bool moreLogs) {
        QString result;
#ifdef WITH_LOGS
        Helpers::Logger &logger = Helpers::Logger::getInstance();
        QString logFilePath = logger.getLogFilePath();
        QFile f(logFilePath);

        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            // 1000 - do not load the UI
            // advanced users will open logs it notepad
            int numberOfLines = moreLogs ? 1000 : 100;
            QString text = f.readAll();
            result = Helpers::getLastNLines(text, numberOfLines);
            f.close();
        }
#else
        Q_UNUSED(moreLogs);
        result = QString::fromLatin1("Logs are not available in this version");
#endif
        return result;

    }

    void LogsModel::clearLogs() {
#ifdef WITH_LOGS
        Helpers::Logger &logger = Helpers::Logger::getInstance();
        logger.clear();
#endif
    }
}
