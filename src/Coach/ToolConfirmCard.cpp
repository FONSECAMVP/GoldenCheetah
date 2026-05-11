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

#include "ToolConfirmCard.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QJsonArray>

ToolConfirmCard::ToolConfirmCard(const QString& callId, const QString& toolName,
                                 const QJsonObject& args, QWidget* parent)
    : QFrame(parent)
    , callId_(callId)
    , toolName_(toolName)
    , args_(args)
{
    setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    setStyleSheet(
        "ToolConfirmCard {"
        "  background-color: #fff8e1;"
        "  border: 1px solid #f9a825;"
        "  border-radius: 8px;"
        "  margin: 4px;"
        "}"
    );

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(8);
    layout->setContentsMargins(12, 10, 12, 10);

    QString displayName = toolName_;
    displayName.replace('_', ' ');
    QLabel* titleLabel = new QLabel(QString("<b>Coach wants to: %1</b>").arg(displayName), this);
    titleLabel->setStyleSheet("color: #e65100;");
    layout->addWidget(titleLabel);

    QLabel* summaryLabel = new QLabel(buildSummary(), this);
    summaryLabel->setWordWrap(true);
    summaryLabel->setStyleSheet("color: #333;");
    layout->addWidget(summaryLabel);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    QPushButton* cancelBtn = new QPushButton(tr("Cancel"), this);
    cancelBtn->setStyleSheet(
        "padding: 5px 14px;"
        "border: 1px solid #bbb;"
        "border-radius: 4px;"
        "background: white;"
    );
    connect(cancelBtn, &QPushButton::clicked, this, &ToolConfirmCard::onCancel);
    btnLayout->addWidget(cancelBtn);

    QPushButton* confirmBtn = new QPushButton(tr("Confirm"), this);
    confirmBtn->setStyleSheet(
        "padding: 5px 14px;"
        "border: none;"
        "border-radius: 4px;"
        "background: #388e3c;"
        "color: white;"
        "font-weight: bold;"
    );
    connect(confirmBtn, &QPushButton::clicked, this, &ToolConfirmCard::onConfirm);
    btnLayout->addWidget(confirmBtn);

    layout->addLayout(btnLayout);
}

QString ToolConfirmCard::buildSummary() const
{
    if (toolName_ == "create_workout") {
        QString name = args_["name"].toString();
        QJsonArray intervals = args_["intervals"].toArray();
        return tr("Workout: <b>%1</b><br>Intervals: %2")
            .arg(name.toHtmlEscaped())
            .arg(intervals.size());
    }
    if (toolName_ == "schedule_workout") {
        QString wname = args_["workout_name"].toString();
        QString date = args_["date"].toString();
        return tr("Schedule <b>%1</b> on %2")
            .arg(wname.toHtmlEscaped(), date.toHtmlEscaped());
    }
    if (toolName_ == "create_season_event") {
        QString name = args_["name"].toString();
        QString date = args_["date"].toString();
        QString priority = args_["priority"].toString("A");
        return tr("Event: <b>%1</b> on %2 (Priority %3)")
            .arg(name.toHtmlEscaped(), date.toHtmlEscaped(), priority.toHtmlEscaped());
    }
    if (toolName_ == "create_training_plan") {
        QString name = args_["name"].toString();
        int weeks = args_["weeks"].toArray().size();
        return tr("Plan: <b>%1</b> — %2 weeks")
            .arg(name.toHtmlEscaped())
            .arg(weeks);
    }
    return tr("Review and confirm this action.");
}

void ToolConfirmCard::onConfirm()
{
    setEnabled(false);
    emit confirmed(callId_, toolName_, args_);
    deleteLater();
}

void ToolConfirmCard::onCancel()
{
    setEnabled(false);
    emit cancelled(callId_, toolName_);
    deleteLater();
}
