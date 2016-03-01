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

#include "languagesmodel.h"
#include <QLocale>
#include <QString>
#include <QTranslator>
#include <QCoreApplication>
#include <QDebug>
#include <QLibraryInfo>
#include "../Common/defines.h"

namespace Models {
    LanguagesModel::LanguagesModel(QObject *parent):
        QAbstractListModel(parent),
        m_CurrentLanguageIndex(-1)
    {
        m_XpiksTranslator = new QTranslator(this);
        m_QtTranslator = new QTranslator(this);

        qInfo() << "LanguagesModel::LanguagesModel #" << QLibraryInfo::location(QLibraryInfo::TranslationsPath);
    }

    void LanguagesModel::loadLanguages() {
        //QCoreApplication *app = QCoreApplication::instance();
        QString translationsPath;
#if !defined(Q_OS_LINUX)
        translationsPath = QCoreApplication::applicationDirPath();
#if defined(Q_OS_MAC)
        translationsPath += "/../Resources/translations/";
#elif defined(Q_OS_WIN)
        translationsPath += "/translations/";
#endif
#endif

        m_LanguagesDirectory = translationsPath;
        loadTranslators(QDir(m_LanguagesDirectory));
    }

    void LanguagesModel::switchLanguage(int index) {
        if (index == m_CurrentLanguageIndex) { return; }

        const QPair<QString, QString> &langPair = m_LanguagesList.at(index);

        QDir languagesDir(m_LanguagesDirectory);

        QLocale locale = QLocale(langPair.first);
        QLocale::setDefault(locale);

        QCoreApplication *app = QCoreApplication::instance();
        app->removeTranslator(m_QtTranslator);
        app->removeTranslator(m_XpiksTranslator);

        QString qtTranslatorPath = languagesDir.filePath(QString("qt_%1.qm").arg(langPair.first));
        if (m_QtTranslator->load(qtTranslatorPath)) {
            qDebug() << "LanguagesModel::switchLanguage #" << "Loaded" << qtTranslatorPath;
            app->installTranslator(m_QtTranslator);
        }

        QString xpiksTranslatorPath = languagesDir.filePath(QString("xpiks_%1.qm").arg(langPair.first));
        if (m_XpiksTranslator->load(xpiksTranslatorPath)) {
            qDebug() << "LanguagesModel::switchLanguage #" << "Loaded" << xpiksTranslatorPath;
            app->installTranslator(m_XpiksTranslator);
        }

        qInfo() << "LanguagesModel::switchLanguage #" << "Switched to" << langPair.first;
        emit languageChanged();
    }
    
    QVariant LanguagesModel::data(const QModelIndex &index, int role) const {
        int row = index.row();
        if (row < 0 || row >= m_LanguagesList.length()) return QVariant();
        if (role == Qt::DisplayRole) { return m_LanguagesList.at(index.row()).second; }
        else if (role == IsCurrentRole) { return row == m_CurrentLanguageIndex; }
        return QVariant();
    }

    QHash<int, QByteArray> LanguagesModel::roleNames() const {
        QHash<int, QByteArray> roles;
        roles[IsCurrentRole] = "iscurrent";
        roles[Qt::DisplayRole] = "display";
        return roles;
    }

    void LanguagesModel::loadTranslators(const QDir &dir) {
        qDebug() << "LanguagesModel::loadTranslators #" << dir.absolutePath();

        QString defaultLocale = QLocale::system().name();
        qDebug() << "LanguagesModel::loadTranslators #" << "Current locale is" << defaultLocale;
        
        const QString filter = QLatin1String("xpiks_*.qm");
        QDir::Filters filters = QDir::Files | QDir::Readable;
        QDir::SortFlags sort = QDir::Name;
        QFileInfoList entries = dir.entryInfoList(QStringList() << filter, filters, sort);

        beginResetModel();
        m_LanguagesList.clear();
        endResetModel();
        
        int lastIndex = 0;
        
        int size = entries.size();
        for (int i = 0; i < size; ++i) {
            const QFileInfo &file = entries.at(i);
            QStringList parts = file.baseName().split(QChar('_'));
            QString language = parts.at(parts.count() - 2);
            QString country  = parts.at(parts.count() - 1);

            //QTranslator* translator = new QTranslator(this);
            //if (translator->load(file.absoluteFilePath())) {
            QString locale = language + "_" + country;

            if (defaultLocale == locale) { m_CurrentLanguageIndex = lastIndex; }

            QString lang = QLocale::languageToString(QLocale(locale).language());

            beginInsertRows(QModelIndex(), lastIndex, lastIndex);
            m_LanguagesList << qMakePair(locale, lang);
            endInsertRows();

            lastIndex++;
                
            qInfo() << "LanguagesModel::loadTranslators #" << "Found" << locale << "translation for language:" << lang;
        }
    }
}

