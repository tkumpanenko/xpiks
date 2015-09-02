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

#ifndef LOGSMODEL
#define LOGSMODEL

#include <QObject>
#include <QString>
#include "../Common/defines.h"

namespace Models {
    class LogsModel : public QObject {
        Q_OBJECT
    public:
        LogsModel(QObject *parent=NULL);
        ~LogsModel() { emit stopLogsSignal(); }
    public:
        Q_INVOKABLE QString getAllLogsText(bool moreLogs=false);
        Q_INVOKABLE void clearLogs();
        Q_INVOKABLE bool canClearLogs() const {
#ifdef WITH_LOGS
            return true;
#else
            return false;
#endif
        }

    signals:
        void stopLogsSignal();
    };
}

#endif // LOGSMODEL

