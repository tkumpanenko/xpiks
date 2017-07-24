/*
 * This file is a part of Xpiks - cross platform application for
 * keywording and uploading images for microstocks
 * Copyright (C) 2014-2017 Taras Kushnir <kushnirTV@gmail.com>
 *
 * Xpiks is distributed under the GNU Lesser General Public License, version 3.0
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef AUTOCOMPLETEMODEL_H
#define AUTOCOMPLETEMODEL_H

#include <QAbstractListModel>
#include <QVector>
#include <QString>
#include <vector>
#include <memory>
#include "../Common/baseentity.h"
#include "completionitem.h"

namespace AutoComplete {
    class AutoCompleteModel : public QAbstractListModel, public Common::BaseEntity
    {
        Q_OBJECT
        Q_PROPERTY(int selectedIndex READ getSelectedIndex WRITE setSelectedIndex NOTIFY selectedIndexChanged)
        Q_PROPERTY(bool isActive READ getIsActive WRITE setIsActive NOTIFY isActiveChanged)
    public:
        AutoCompleteModel(QObject *parent=0);

    public:
        enum AutoCompleteModelRoles {
            IsPresetRole = Qt::UserRole + 1,
        };

    public:
        void setCompletions(std::vector<std::shared_ptr<CompletionItem> > &completions);

    public:
        int getSelectedIndex() const { return m_SelectedIndex; }
        bool getIsActive() const { return m_IsActive; }

    public:
        void setSelectedIndex(int value) {
            if (value != m_SelectedIndex) {
                m_SelectedIndex = value;
                emit selectedIndexChanged(value);
            }
        }

        void setIsActive(bool value) {
            if (value != m_IsActive) {
                m_IsActive = value;
                emit isActiveChanged(value);
            }
        }

    public:
        Q_INVOKABLE bool moveSelectionUp();
        Q_INVOKABLE bool moveSelectionDown();
        Q_INVOKABLE void cancelCompletion() { setIsActive(false); emit dismissPopupRequested(); }
        Q_INVOKABLE void acceptSelected(bool tryExpandPreset=false);
        Q_INVOKABLE int getCount() const { return qMax(m_CompletionList.size(), m_LastGeneratedCompletions.size()); }
        Q_INVOKABLE bool hasSelectedCompletion() const { return (0 <= m_SelectedIndex) && (m_SelectedIndex < rowCount()); }

    signals:
        void selectedIndexChanged(int index);
        void dismissPopupRequested();
        void completionAccepted(const QString &completion, bool expandPreset);
        void isActiveChanged(bool value);
#ifdef INTEGRATION_TESTS
        void completionsUpdated();
#endif

    public slots:
        void completionsArrived();

        // QAbstractItemModel interface
    public:
        virtual int rowCount(const QModelIndex &parent=QModelIndex()) const override { Q_UNUSED(parent); return (int)m_CompletionList.size(); }
        virtual QVariant data(const QModelIndex &index, int role) const override;
        virtual QHash<int, QByteArray> roleNames() const override;

#ifdef INTEGRATION_TESTS
        bool containsWord(const QString &word) const;
        QStringList getLastGeneratedCompletions() const;
#endif

    private:
        std::vector<std::shared_ptr<CompletionItem> > m_CompletionList;
        std::vector<std::shared_ptr<CompletionItem> > m_LastGeneratedCompletions;
        int m_SelectedIndex;
        bool m_IsActive;
    };
}

#endif // AUTOCOMPLETEMODEL_H
