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

#ifndef _GC_ToolConfirmCard_h
#define _GC_ToolConfirmCard_h

#include <QFrame>
#include <QString>
#include <QJsonObject>

class QVBoxLayout;
class QLabel;
class QPushButton;

class ToolConfirmCard : public QFrame
{
    Q_OBJECT

public:
    ToolConfirmCard(const QString& callId, const QString& toolName,
                    const QJsonObject& args, QWidget* parent = nullptr);

signals:
    void confirmed(const QString& callId, const QString& toolName, const QJsonObject& args);
    void cancelled(const QString& callId, const QString& toolName);

private slots:
    void onConfirm();
    void onCancel();

private:
    QString buildSummary() const;

    QString callId_;
    QString toolName_;
    QJsonObject args_;
};

#endif // _GC_ToolConfirmCard_h
