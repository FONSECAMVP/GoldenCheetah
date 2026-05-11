/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef _GC_PlanPreviewCard_h
#define _GC_PlanPreviewCard_h

#include <QFrame>
#include <QString>
#include <QJsonObject>

class QTreeWidget;

class PlanPreviewCard : public QFrame
{
    Q_OBJECT

public:
    explicit PlanPreviewCard(const QString& callId, const QJsonObject& plan,
                             QWidget* parent = nullptr);

signals:
    void planApplied(const QString& callId, const QJsonObject& plan);
    void planCancelled(const QString& callId);

private slots:
    void onApply();
    void onCancel();

private:
    void populateTree();

    QString callId_;
    QJsonObject plan_;
    QTreeWidget* treeWidget_;
};

#endif // _GC_PlanPreviewCard_h
