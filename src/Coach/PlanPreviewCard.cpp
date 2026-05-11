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

#include "PlanPreviewCard.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QJsonArray>

PlanPreviewCard::PlanPreviewCard(const QString& callId, const QJsonObject& plan,
                                 QWidget* parent)
    : QFrame(parent)
    , callId_(callId)
    , plan_(plan)
{
    setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    setStyleSheet(
        "PlanPreviewCard {"
        "  background-color: #e8f5e9;"
        "  border: 1px solid #388e3c;"
        "  border-radius: 8px;"
        "  margin: 4px;"
        "}"
    );

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(8);
    layout->setContentsMargins(12, 10, 12, 10);

    QString planName = plan_["name"].toString("Training Plan");
    QString startDate = plan_["start_date"].toString();
    QLabel* titleLabel = new QLabel(
        QString("<b>Training Plan: %1</b>").arg(planName.toHtmlEscaped()), this);
    titleLabel->setStyleSheet("color: #1b5e20; font-size: 14px;");
    layout->addWidget(titleLabel);

    if (!startDate.isEmpty()) {
        QLabel* dateLabel = new QLabel(tr("Starts: %1").arg(startDate), this);
        dateLabel->setStyleSheet("color: #555;");
        layout->addWidget(dateLabel);
    }

    treeWidget_ = new QTreeWidget(this);
    treeWidget_->setColumnCount(2);
    treeWidget_->setHeaderLabels({tr("Week / Workout"), tr("Details")});
    treeWidget_->setMaximumHeight(200);
    treeWidget_->setRootIsDecorated(true);
    populateTree();
    layout->addWidget(treeWidget_);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    QPushButton* cancelBtn = new QPushButton(tr("Cancel"), this);
    cancelBtn->setStyleSheet(
        "padding: 5px 14px;"
        "border: 1px solid #bbb;"
        "border-radius: 4px;"
        "background: white;"
    );
    connect(cancelBtn, &QPushButton::clicked, this, &PlanPreviewCard::onCancel);
    btnLayout->addWidget(cancelBtn);

    QPushButton* applyBtn = new QPushButton(tr("Apply Plan"), this);
    applyBtn->setStyleSheet(
        "padding: 5px 14px;"
        "border: none;"
        "border-radius: 4px;"
        "background: #388e3c;"
        "color: white;"
        "font-weight: bold;"
    );
    connect(applyBtn, &QPushButton::clicked, this, &PlanPreviewCard::onApply);
    btnLayout->addWidget(applyBtn);

    layout->addLayout(btnLayout);
}

void PlanPreviewCard::populateTree()
{
    QJsonArray weeks = plan_["weeks"].toArray();
    int weekNum = 1;
    for (const QJsonValue& weekVal : weeks) {
        QJsonObject week = weekVal.toObject();
        QString weekLabel = week["name"].toString(tr("Week %1").arg(weekNum));
        QTreeWidgetItem* weekItem = new QTreeWidgetItem(treeWidget_);
        weekItem->setText(0, weekLabel);
        weekItem->setText(1, "");
        weekItem->setExpanded(weekNum <= 2); // auto-expand first two weeks

        QJsonArray workouts = week["workouts"].toArray();
        for (const QJsonValue& woVal : workouts) {
            QJsonObject wo = woVal.toObject();
            QString woName = wo["name"].toString();
            QString woDay = wo["day"].toString();
            QString detail = woDay.isEmpty() ? woName : QString("%1 (%2)").arg(woName, woDay);
            QTreeWidgetItem* woItem = new QTreeWidgetItem(weekItem);
            woItem->setText(0, woName);
            woItem->setText(1, woDay);
            Q_UNUSED(detail)
        }

        weekNum++;
    }
    treeWidget_->resizeColumnToContents(0);
}

void PlanPreviewCard::onApply()
{
    setEnabled(false);
    emit planApplied(callId_, plan_);
    deleteLater();
}

void PlanPreviewCard::onCancel()
{
    setEnabled(false);
    emit planCancelled(callId_);
    deleteLater();
}
