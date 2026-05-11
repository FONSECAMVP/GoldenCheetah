/*
 * Copyright (c) 2024 GoldenCheetah Contributor
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

#include "CoachChatWidget.h"
#include "OpenAIClient.h"
#include "AnthropicClient.h"
#include "GeminiClient.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QScrollBar>
#include <QFrame>
#include <QFont>
#include <QColor>
#include <QPalette>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QSettings>

CoachChatWidget::CoachChatWidget(Context* context, QWidget *parent)
    : QDialog(parent)
    , context_(context)
    , currentPhase_(PromptBuilder::Assessment)
    , isStreaming_(false)
    , systemPromptSent_(false)
{

    // Initialize LLM clients
    openaiClient_ = new OpenAIClient(this);
    anthropicClient_ = new AnthropicClient(this);
    geminiClient_ = new GeminiClient(this);
    contextAggregator_ = new AthleteContextAggregator(this);
    promptBuilder_ = new PromptBuilder();
    toolExecutor_ = new GCToolExecutor(context_, this);

    setupUi();

    toolExecutor_->setChatLayout(chatLayout_);

    // Load saved provider and per-provider API keys
    QSettings settings;
    currentProvider_ = settings.value("coach/provider", "OpenAI").toString();

    QString openaiKey = settings.value("coach/openai_apikey", "").toString();
    QString anthropicKey = settings.value("coach/anthropic_apikey", "").toString();
    QString geminiKey = settings.value("coach/gemini_apikey", "").toString();

    if (!openaiKey.isEmpty()) openaiClient_->setApiKey(openaiKey);
    if (!anthropicKey.isEmpty()) anthropicClient_->setApiKey(anthropicKey);
    if (!geminiKey.isEmpty()) geminiClient_->setApiKey(geminiKey);

    // Set the active provider
    switchProvider(currentProvider_);

    setupConnections();

    // Update context on load
    contextAggregator_->updateContext(context);
}

void CoachChatWidget::setupUi()
{
    mainLayout_ = new QVBoxLayout(this);
    mainLayout_->setSpacing(0);
    mainLayout_->setContentsMargins(10, 10, 10, 10);

    // Header
    headerWidget_ = new QWidget();
    headerWidget_->setObjectName("coachHeader");
    headerLayout_ = new QHBoxLayout(headerWidget_);
    headerLayout_->setSpacing(10);

    phaseLabel_ = new QLabel(tr("Coaching Phase:"), headerWidget_);
    phaseComboBox_ = new QComboBox(headerWidget_);
    phaseComboBox_->addItem(tr("Assessment"), QVariant::fromValue(PromptBuilder::Assessment));
    phaseComboBox_->addItem(tr("Analysis"), QVariant::fromValue(PromptBuilder::Analysis));
    phaseComboBox_->addItem(tr("Planning"), QVariant::fromValue(PromptBuilder::Planning));
    phaseComboBox_->addItem(tr("Daily Coaching"), QVariant::fromValue(PromptBuilder::Coaching));
    phaseComboBox_->setCurrentIndex(0);

    QSpacerItem* spacer = new QSpacerItem(40, 20, QSizePolicy::Expanding);

    clearButton_ = new QToolButton(headerWidget_);
    clearButton_->setText(tr("Clear"));
    clearButton_->setToolTip(tr("Clear conversation"));

    settingsButton_ = new QToolButton(headerWidget_);
    settingsButton_->setText(tr("Settings"));
    settingsButton_->setToolTip(tr("Configure LLM settings"));

    headerLayout_->addWidget(phaseLabel_);
    headerLayout_->addWidget(phaseComboBox_);
    headerLayout_->addItem(spacer);
    headerLayout_->addWidget(clearButton_);
    headerLayout_->addWidget(settingsButton_);

    mainLayout_->addWidget(headerWidget_);

    // Chat scroll area
    chatScrollArea_ = new QScrollArea();
    chatScrollArea_->setWidgetResizable(true);
    chatScrollArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    chatWidget_ = new QWidget();
    chatLayout_ = new QVBoxLayout(chatWidget_);
    chatLayout_->setSpacing(10);
    chatLayout_->addStretch();

    chatScrollArea_->setWidget(chatWidget_);
    mainLayout_->addWidget(chatScrollArea_, 1);

    // Suggestions
    suggestionsWidget_ = new QWidget();
    suggestionsLayout_ = new QVBoxLayout(suggestionsWidget_);
    suggestionsLayout_->setSpacing(5);
    suggestionsLayout_->setContentsMargins(10, 5, 10, 5);

    QLabel* suggestionsLabel = new QLabel(tr("Quick Questions:"), suggestionsWidget_);
    suggestionsLabel->setStyleSheet("font-weight: bold;");
    suggestionsLayout_->addWidget(suggestionsLabel);

    updateSuggestions();
    mainLayout_->addWidget(suggestionsWidget_);

    // Loading indicator
    loadingWidget_ = new QWidget();
    QHBoxLayout* loadingLayout = new QHBoxLayout(loadingWidget_);
    loadingBar_ = new QProgressBar(loadingWidget_);
    loadingBar_->setRange(0, 0);
    loadingLabel_ = new QLabel(tr("Generating response..."), loadingWidget_);
    loadingLayout->addWidget(loadingBar_);
    loadingLayout->addWidget(loadingLabel_);
    loadingWidget_->hide();
    mainLayout_->addWidget(loadingWidget_);

    // Input area
    inputWidget_ = new QWidget();
    inputLayout_ = new QHBoxLayout(inputWidget_);
    inputLayout_->setSpacing(10);

    inputTextEdit_ = new QTextEdit(inputWidget_);
    inputTextEdit_->setPlaceholderText(tr("Ask your coach..."));
    inputTextEdit_->setMaximumHeight(100);

    sendButton_ = new QPushButton(tr("Send"), inputWidget_);
    sendButton_->setDefault(true);

    inputLayout_->addWidget(inputTextEdit_, 1);
    inputLayout_->addWidget(sendButton_);
    mainLayout_->addWidget(inputWidget_);

    // Initial welcome message
    appendMessage("assistant", tr("Hi! I'm your GoldenCheetah AI Coach. "
        "I'm here to help you optimize your training and performance. "
        "How can I assist you today?"));
}

void CoachChatWidget::setupConnections()
{
    connect(sendButton_, &QPushButton::clicked, this, &CoachChatWidget::onSendMessage);
    connect(inputTextEdit_, &QTextEdit::textChanged, this, [this]() {
        sendButton_->setEnabled(!inputTextEdit_->toPlainText().trimmed().isEmpty());
    });

    // LLM signals are connected in switchProvider()/connectLLMSignals()

    connect(clearButton_, &QToolButton::clicked, this, &CoachChatWidget::onClearConversation);
    connect(settingsButton_, &QToolButton::clicked, this, &CoachChatWidget::onShowSettings);
    connect(phaseComboBox_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CoachChatWidget::onPhaseChanged);

    sendButton_->setEnabled(false);
}

void CoachChatWidget::onSendMessage()
{
    QString message = inputTextEdit_->toPlainText().trimmed();
    if (message.isEmpty()) return;

    appendMessage("user", message);
    inputTextEdit_->clear();
    sendButton_->setEnabled(false);

    // Refresh athlete context before sending
    contextAggregator_->updateContext(context_);

    // Build comprehensive system prompt with all available training data
    AthleteProfile profile = contextAggregator_->getProfile();
    QString systemPrompt = promptBuilder_->buildSystemPrompt(currentPhase_, profile);

    // Determine adaptive lookback: find how many days back to the most recent ride,
    // then use at least that + 90 days so we always capture meaningful data.
    // This ensures the coach sees data even during training breaks.
    int adaptiveDays = 90; // minimum
    QList<RideSummary> probe = contextAggregator_->getRecentRides(365 * 7); // up to 7 years
    if (!probe.isEmpty()) {
        // probe is sorted descending; first entry is most recent ride
        int daysSinceLastRide = probe.first().date.date().daysTo(QDate::currentDate());
        if (daysSinceLastRide > adaptiveDays)
            adaptiveDays = daysSinceLastRide + 90;
        // Cap at 7 years to avoid excessive data
        if (adaptiveDays > 365 * 7)
            adaptiveDays = 365 * 7;
    }

    // === SECTION 1: Full training history summary ===
    // Use all available rides for the complete history overview
    QList<RideSummary> allRides = probe; // already fetched above
    if (!allRides.isEmpty()) {
        // Show the most recent 30 rides with full detail
        systemPrompt += "\n\n--- RECENT RIDE HISTORY (last 30 activities) ---\n";
        systemPrompt += promptBuilder_->formatRideHistory(allRides, 30);

        // Add a full historical summary across all data
        systemPrompt += "\n--- COMPLETE TRAINING HISTORY SUMMARY ---\n";
        systemPrompt += QString("Total activities on record: %1\n").arg(allRides.size());
        systemPrompt += QString("Date range: %1 to %2\n")
            .arg(allRides.last().date.toString("yyyy-MM-dd"))
            .arg(allRides.first().date.toString("yyyy-MM-dd"));

        int daysSinceLast = allRides.first().date.date().daysTo(QDate::currentDate());
        if (daysSinceLast > 0)
            systemPrompt += QString("Days since last activity: %1\n").arg(daysSinceLast);

        // Per-year summary
        QMap<int, int> ridesByYear;
        QMap<int, double> tssByYear;
        QMap<int, double> hoursByYear;
        for (const RideSummary& ride : allRides) {
            int year = ride.date.date().year();
            ridesByYear[year]++;
            tssByYear[year] += ride.tss;
            hoursByYear[year] += ride.duration / 3600.0;
        }
        systemPrompt += "\nYearly Summary:\n";
        systemPrompt += "Year | Activities | Hours | Total TSS\n";
        for (auto it = ridesByYear.constBegin(); it != ridesByYear.constEnd(); ++it) {
            systemPrompt += QString("%1 | %2 | %3 | %4\n")
                .arg(it.key())
                .arg(it.value())
                .arg(hoursByYear[it.key()], 0, 'f', 1)
                .arg(tssByYear[it.key()], 0, 'f', 0);
        }

        // Monthly summary for last 12 months
        QMap<QString, int> ridesByMonth;
        QMap<QString, double> tssByMonth;
        QMap<QString, double> hoursByMonth;
        QDate twelveMonthsAgo = QDate::currentDate().addMonths(-12);
        for (const RideSummary& ride : allRides) {
            if (ride.date.date() < twelveMonthsAgo) continue;
            QString monthKey = ride.date.toString("yyyy-MM");
            ridesByMonth[monthKey]++;
            tssByMonth[monthKey] += ride.tss;
            hoursByMonth[monthKey] += ride.duration / 3600.0;
        }
        if (!ridesByMonth.isEmpty()) {
            systemPrompt += "\nMonthly Summary (last 12 months):\n";
            systemPrompt += "Month | Activities | Hours | TSS\n";
            QStringList sortedMonths = ridesByMonth.keys();
            sortedMonths.sort();
            for (const QString& month : sortedMonths) {
                systemPrompt += QString("%1 | %2 | %3 | %4\n")
                    .arg(month)
                    .arg(ridesByMonth[month])
                    .arg(hoursByMonth[month], 0, 'f', 1)
                    .arg(tssByMonth[month], 0, 'f', 0);
            }
        }

        systemPrompt += "\n" + promptBuilder_->summarizeRecentRides(allRides, adaptiveDays);
    }

    // === SECTION 2: Activity metadata summary (devices, routes, indoor/outdoor, RPE) ===
    if (!allRides.isEmpty()) {
        QString metadataText = PromptBuilder::formatRideMetadata(allRides, allRides.size());
        if (!metadataText.isEmpty()) {
            systemPrompt += "\n--- ACTIVITY METADATA ---\n" + metadataText;
        }
    }

    // === SECTION 3: Sport breakdown (multi-sport support) ===
    QMap<QString, int> sportBreakdown = contextAggregator_->getSportBreakdown(adaptiveDays);
    if (!sportBreakdown.isEmpty()) {
        systemPrompt += "\n--- SPORT BREAKDOWN ---\n" +
            PromptBuilder::formatSportBreakdown(sportBreakdown);
    }

    // === SECTION 4: Power curve personal bests (all-time) ===
    PowerCurveBests powerBests = contextAggregator_->getPowerCurveBests(365 * 7);
    QString powerCurveText = PromptBuilder::formatPowerCurveBests(powerBests);
    if (!powerCurveText.isEmpty()) {
        systemPrompt += "\n--- POWER PROFILE ---\n" + powerCurveText;
    }

    // === SECTION 5: Training zone distribution ===
    ZoneDistribution zoneDist = contextAggregator_->getZoneDistribution(adaptiveDays);
    QString zoneText = PromptBuilder::formatZoneDistribution(zoneDist);
    if (!zoneText.isEmpty()) {
        systemPrompt += "\n--- TRAINING ZONES ---\n" + zoneText;
    }

    // === SECTION 6: Weekly training progression (up to 52 weeks) ===
    int weeksToShow = qMin(52, adaptiveDays / 7);
    QList<WeeklyTrainingSummary> weeklySummaries = contextAggregator_->getWeeklySummaries(weeksToShow);
    QString weeklyText = PromptBuilder::formatWeeklySummaries(weeklySummaries);
    if (!weeklyText.isEmpty()) {
        systemPrompt += "\n--- WEEKLY PROGRESSION ---\n" + weeklyText;
    }

    // === SECTION 7: Workout type breakdown ===
    QList<WorkoutClassification> workoutTypes = contextAggregator_->getWorkoutTypeBreakdown(adaptiveDays);
    QString typeText = PromptBuilder::formatWorkoutTypeBreakdown(workoutTypes);
    if (!typeText.isEmpty()) {
        systemPrompt += "\n--- WORKOUT MIX ---\n" + typeText;
    }

    // === SECTION 8: Key intervals (efforts, tests, climbs) ===
    QList<IntervalSummary> keyIntervals = contextAggregator_->getKeyIntervals(adaptiveDays);
    if (!keyIntervals.isEmpty()) {
        systemPrompt += "\n--- KEY INTERVALS ---\n" +
            PromptBuilder::formatKeyIntervals(keyIntervals);
    }

    // === SECTION 9: Season plan (phases & events) ===
    SeasonPlanSummary seasonPlan = contextAggregator_->getSeasonPlan();
    QString seasonText = PromptBuilder::formatSeasonPlan(seasonPlan);
    if (!seasonText.isEmpty()) {
        systemPrompt += "\n--- SEASON PLAN ---\n" + seasonText;
    }

    // === SECTION 10: Year-over-year comparison ===
    AthleteContextAggregator::PeriodComparison yoyComp =
        contextAggregator_->getYearOverYearComparison(adaptiveDays);
    systemPrompt += "\n--- YEAR-OVER-YEAR ---\n" +
        PromptBuilder::formatYearOverYearComparison(yoyComp, adaptiveDays);

    // === SECTION 11: PMC history & interpretation ===
    QVector<double> pmcHistory = contextAggregator_->getPMCHistory(adaptiveDays);
    if (!pmcHistory.isEmpty()) {
        systemPrompt += "\n--- PMC ANALYSIS ---\n" +
            promptBuilder_->summarizePMCData(profile, pmcHistory);
    }

    // === SECTION 12: HRV data ===
    QVector<double> hrvHistory = contextAggregator_->getHRVHistory(adaptiveDays);
    if (!hrvHistory.isEmpty()) {
        systemPrompt += "\n--- HRV DATA ---\n" +
            promptBuilder_->summarizeHRVData(profile, hrvHistory);
    }

    // === SECTION 13: Body composition ===
    AthleteContextAggregator::BodyCompTrend bodyTrend = contextAggregator_->getBodyCompositionTrend();
    QString bodyText = PromptBuilder::formatBodyComposition(bodyTrend, profile);
    if (!bodyText.isEmpty()) {
        systemPrompt += "\n--- BODY COMPOSITION ---\n" + bodyText;
    }

    // === SECTION 14: Device performance estimates (VO2max, Training Effect) ===
    if (!allRides.isEmpty()) {
        QString deviceText = PromptBuilder::formatDevicePerformanceEstimates(allRides);
        if (!deviceText.isEmpty()) {
            systemPrompt += "\n--- DEVICE ESTIMATES ---\n" + deviceText;
        }
    }

    // === SECTION 15: Personalized insights & pattern analysis ===
    QStringList insights = contextAggregator_->getPersonalizedInsights();
    if (!insights.isEmpty()) {
        systemPrompt += "\n--- KEY INSIGHTS ---\n";
        for (const QString& insight : insights) {
            systemPrompt += "- " + insight + "\n";
        }
    }

    AthleteContextAggregator::PatternAnalysis patterns = contextAggregator_->analyzePatterns();
    if (!patterns.overallAssessment.isEmpty()) {
        systemPrompt += "\nOverall Assessment: " + patterns.overallAssessment + "\n";
        if (!patterns.strengths.isEmpty())
            systemPrompt += "Strengths: " + patterns.strengths.join(", ") + "\n";
        if (!patterns.weaknesses.isEmpty())
            systemPrompt += "Weaknesses: " + patterns.weaknesses.join(", ") + "\n";
        if (!patterns.fatigueIndicators.isEmpty())
            systemPrompt += "Fatigue indicators: " + patterns.fatigueIndicators.join(", ") + "\n";
        if (!patterns.improvementAreas.isEmpty())
            systemPrompt += "Areas to improve: " + patterns.improvementAreas.join(", ") + "\n";
    }

    // === SECTION 16: Training recommendations ===
    QList<TrainingRecommendation> recs = contextAggregator_->getDailyRecommendations();
    if (!recs.isEmpty()) {
        systemPrompt += "\n--- TODAY'S RECOMMENDATIONS ---\n" +
            PromptBuilder::formatTrainingPlan(recs);
    }

    // Append tool use guidance when provider supports actions (REQ-018)
    if (llmService_->supportsTools()) {
        systemPrompt += PromptBuilder::buildToolUseSection();
    }

    // Preserve conversation history across messages within the same session.
    // Only reset conversation when system prompt changes (phase change, new data).
    if (!systemPromptSent_ || systemPrompt != lastSystemPrompt_) {
        llmService_->clearConversation();
        llmService_->addSystemMessage(systemPrompt);
        systemPromptSent_ = true;
        lastSystemPrompt_ = systemPrompt;
    }

    showLoadingIndicator();
    llmService_->sendMessageStream(message);
}

void CoachChatWidget::onResponseReceived(const LLMService::Response& response)
{
    hideLoadingIndicator();

    if (!response.success) {
        inputTextEdit_->setEnabled(true);
        sendButton_->setEnabled(!inputTextEdit_->toPlainText().trimmed().isEmpty());
        appendMessage("assistant", tr("I encountered an error: %1").arg(response.error));
        return;
    }

    inputTextEdit_->setEnabled(true);
    sendButton_->setEnabled(!inputTextEdit_->toPlainText().trimmed().isEmpty());

    if (!isStreaming_) {
        appendStreamingMessage("assistant", response.text);
    }
}

void CoachChatWidget::onStreamingChunk(const QString& chunk)
{
    if (!isStreaming_) {
        isStreaming_ = true;
        currentStreamingMessage_.clear();
        hideLoadingIndicator();
        // Create a new assistant message label for streaming content
        appendMessage("assistant", "");
    }

    currentStreamingMessage_ += chunk;

    // Update the last assistant message label (inserted before the stretch)
    for (int i = chatLayout_->count() - 1; i >= 0; --i) {
        QLayoutItem* item = chatLayout_->itemAt(i);
        if (item && item->widget()) {
            QLabel* label = qobject_cast<QLabel*>(item->widget());
            if (label && label->property("role").toString() == "assistant") {
                label->setText(currentStreamingMessage_);
                break;
            }
        }
    }

    scrollToBottom();
}

void CoachChatWidget::onStreamingFinished()
{
    if (!currentStreamingMessage_.isEmpty()) {
        llmService_->addAssistantMessage(currentStreamingMessage_);
    }

    isStreaming_ = false;
    hideLoadingIndicator();
    inputTextEdit_->setEnabled(true);
    sendButton_->setEnabled(!inputTextEdit_->toPlainText().trimmed().isEmpty());
}

void CoachChatWidget::onStreamingError(const QString& error)
{
    isStreaming_ = false;
    hideLoadingIndicator();
    inputTextEdit_->setEnabled(true);
    sendButton_->setEnabled(!inputTextEdit_->toPlainText().trimmed().isEmpty());
    appendMessage("assistant", tr("Stream interrupted: %1").arg(error));
}

void CoachChatWidget::onClearConversation()
{
    // Clear chat display
    while (chatLayout_->count() > 1) {
        QLayoutItem* item = chatLayout_->takeAt(0);
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    // Re-add stretch
    chatLayout_->addStretch();

    llmService_->clearConversation();
    isStreaming_ = false;
    systemPromptSent_ = false;
    lastSystemPrompt_.clear();

    // Re-show welcome
    appendMessage("assistant", tr("Conversation cleared. How can I help you?"));
}

void CoachChatWidget::onShowSettings()
{
    QSettings settings;

    QDialog dlg(this);
    dlg.setWindowTitle(tr("AI Coach Settings"));
    dlg.resize(500, 320);

    QFormLayout *form = new QFormLayout(&dlg);

    // Provider selector
    QComboBox *providerCombo = new QComboBox(&dlg);
    providerCombo->addItems({"OpenAI", "Anthropic", "Google Gemini"});
    providerCombo->setCurrentText(currentProvider_);
    form->addRow(tr("Provider:"), providerCombo);

    // Per-provider API key fields
    QLineEdit *openaiKeyEdit = new QLineEdit(&dlg);
    openaiKeyEdit->setEchoMode(QLineEdit::Password);
    openaiKeyEdit->setText(settings.value("coach/openai_apikey", "").toString());
    openaiKeyEdit->setPlaceholderText(tr("Enter OpenAI API key"));
    form->addRow(tr("OpenAI API Key:"), openaiKeyEdit);

    QLineEdit *anthropicKeyEdit = new QLineEdit(&dlg);
    anthropicKeyEdit->setEchoMode(QLineEdit::Password);
    anthropicKeyEdit->setText(settings.value("coach/anthropic_apikey", "").toString());
    anthropicKeyEdit->setPlaceholderText(tr("Enter Anthropic API key"));
    form->addRow(tr("Anthropic API Key:"), anthropicKeyEdit);

    QLineEdit *geminiKeyEdit = new QLineEdit(&dlg);
    geminiKeyEdit->setEchoMode(QLineEdit::Password);
    geminiKeyEdit->setText(settings.value("coach/gemini_apikey", "").toString());
    geminiKeyEdit->setPlaceholderText(tr("Enter Google Gemini API key"));
    form->addRow(tr("Gemini API Key:"), geminiKeyEdit);

    // Model selector (updates based on provider)
    QComboBox *modelCombo = new QComboBox(&dlg);
    modelCombo->setEditable(true);

    auto updateModels = [&](const QString& provider) {
        modelCombo->clear();
        if (provider == "OpenAI") {
            modelCombo->addItems({"gpt-4o", "gpt-4o-mini", "gpt-4-turbo", "gpt-3.5-turbo"});
        } else if (provider == "Anthropic") {
            modelCombo->addItems({"claude-sonnet-4-20250514", "claude-haiku-4-20250414",
                                  "claude-opus-4-20250514"});
        } else {
            modelCombo->addItems({"gemini-2.5-pro", "gemini-2.0-flash", "gemini-1.5-pro"});
        }
        // Restore saved model for this provider
        QString savedModel = settings.value("coach/" + provider.toLower() + "_model", "").toString();
        if (!savedModel.isEmpty()) modelCombo->setCurrentText(savedModel);
    };
    updateModels(providerCombo->currentText());
    connect(providerCombo, &QComboBox::currentTextChanged, &dlg, updateModels);

    form->addRow(tr("Model:"), modelCombo);

    QDialogButtonBox *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    form->addRow(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted) {
        // Save all API keys
        QString openaiKey = openaiKeyEdit->text().trimmed();
        QString anthropicKey = anthropicKeyEdit->text().trimmed();
        QString geminiKey = geminiKeyEdit->text().trimmed();

        settings.setValue("coach/openai_apikey", openaiKey);
        settings.setValue("coach/anthropic_apikey", anthropicKey);
        settings.setValue("coach/gemini_apikey", geminiKey);

        if (!openaiKey.isEmpty()) openaiClient_->setApiKey(openaiKey);
        if (!anthropicKey.isEmpty()) anthropicClient_->setApiKey(anthropicKey);
        if (!geminiKey.isEmpty()) geminiClient_->setApiKey(geminiKey);

        // Save provider and model
        QString provider = providerCombo->currentText();
        QString model = modelCombo->currentText().trimmed();
        settings.setValue("coach/provider", provider);
        settings.setValue("coach/" + provider.toLower() + "_model", model);

        // Switch provider if changed
        if (provider != currentProvider_) {
            switchProvider(provider);
        }

        // Update model on current provider
        llmService_->configure({{"model", model}});
    }
}

void CoachChatWidget::onPhaseChanged(int index)
{
    currentPhase_ = phaseComboBox_->itemData(index).value<PromptBuilder::CoachingPhase>();
    // Phase change means different system prompt instructions, so reset conversation
    systemPromptSent_ = false;
    lastSystemPrompt_.clear();
    updateSuggestions();
    emit coachingPhaseChanged(currentPhase_);
}

void CoachChatWidget::appendMessage(const QString& role, const QString& content)
{
    QLabel* messageLabel = new QLabel(chatWidget_);
    messageLabel->setWordWrap(true);
    messageLabel->setTextFormat(Qt::RichText);
    messageLabel->setText(content);
    messageLabel->setProperty("role", role);

    // Style based on role
    if (role == "user") {
        messageLabel->setStyleSheet(
            "background-color: #e3f2fd;"
            "border-radius: 10px;"
            "padding: 10px;"
            "margin-left: 50px;"
        );
    } else {
        messageLabel->setStyleSheet(
            "background-color: #f5f5f5;"
            "border-radius: 10px;"
            "padding: 10px;"
            "margin-right: 50px;"
        );
    }

    // Insert before the stretch
    chatLayout_->insertWidget(chatLayout_->count() - 1, messageLabel);
    scrollToBottom();
}

void CoachChatWidget::appendStreamingMessage(const QString& role, const QString& content)
{
    appendMessage(role, content);
}

void CoachChatWidget::showLoadingIndicator()
{
    loadingWidget_->show();
    chatScrollArea_->setEnabled(false);
}

void CoachChatWidget::hideLoadingIndicator()
{
    loadingWidget_->hide();
    chatScrollArea_->setEnabled(true);
}

void CoachChatWidget::scrollToBottom()
{
    QScrollBar* scrollBar = chatScrollArea_->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

void CoachChatWidget::updateSuggestions()
{
    // Clear existing suggestion buttons
    QLayoutItem* item;
    while ((item = suggestionsLayout_->takeAt(1)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    QStringList questions = promptBuilder_->getSuggestedFollowups(currentPhase_);

    for (const QString& question : questions) {
        QPushButton* button = new QPushButton(question, suggestionsWidget_);
        button->setStyleSheet(
            "text-align: left;"
            "padding: 8px;"
            "border: 1px solid #ccc;"
            "border-radius: 5px;"
            "background-color: white;"
        );
        connect(button, &QPushButton::clicked, this, [this, question]() {
            inputTextEdit_->setText(question);
            onSendMessage();
        });
        suggestionsLayout_->addWidget(button);
    }
}

void CoachChatWidget::updateConnectionStatus()
{
    // Check if API key is configured
}

void CoachChatWidget::onSuggestedQuestionClicked(const QString& question)
{
    inputTextEdit_->setText(question);
    onSendMessage();
}

void CoachChatWidget::onSettingsButtonClicked()
{
    onShowSettings();
}

void CoachChatWidget::onApiKeyChanged(const QString& apiKey)
{
    llmService_->setApiKey(apiKey);
}

void CoachChatWidget::onModelChanged(const QString& model)
{
    llmService_->configure({{"model", model}});
}

void CoachChatWidget::onTemperatureChanged(double temp)
{
    llmService_->setTemperature(temp);
}

void CoachChatWidget::switchProvider(const QString& providerName)
{
    // Disconnect old signals
    if (llmService_) {
        disconnect(llmService_, nullptr, this, nullptr);
        disconnect(llmService_, nullptr, toolExecutor_, nullptr);
    }
    disconnect(toolExecutor_, nullptr, this, nullptr);

    if (providerName == "Anthropic") {
        llmService_ = anthropicClient_;
    } else if (providerName == "Google Gemini") {
        llmService_ = geminiClient_;
    } else {
        llmService_ = openaiClient_;
    }

    currentProvider_ = providerName;
    systemPromptSent_ = false;
    lastSystemPrompt_.clear();
    connectLLMSignals(llmService_);
}

void CoachChatWidget::connectLLMSignals(LLMService* service)
{
    connect(service, &LLMService::responseReady,
            this, &CoachChatWidget::onResponseReceived);
    connect(service, &LLMService::streamingChunk,
            this, &CoachChatWidget::onStreamingChunk);
    connect(service, &LLMService::streamingFinished,
            this, &CoachChatWidget::onStreamingFinished);
    connect(service, &LLMService::streamingError,
            this, &CoachChatWidget::onStreamingError);

    connect(service, &LLMService::toolCallRequested,
            toolExecutor_, &GCToolExecutor::onToolCallRequested);
    connect(service, &LLMService::toolCallRequested,
            this, [this](const QString&, const QString&, const QJsonObject&) {
                hideLoadingIndicator();
                inputTextEdit_->setEnabled(false);
                sendButton_->setEnabled(false);
            });
    connect(toolExecutor_, &GCToolExecutor::toolResultReady,
            this, [this, service](const QString& callId, const QString& toolName, const QJsonObject& result) {
                showLoadingIndicator();
                service->sendToolResult(callId, toolName, result);
            });
    connect(toolExecutor_, &GCToolExecutor::actionMessage,
            this, [this](const QString& msg) { appendMessage("system", msg); });

    if (service->supportsTools()) {
        service->setTools(GCToolExecutor::v1Tools());
    } else {
        appendMessage("system", tr("Note: %1 does not support tool use. "
            "Workout creation and scheduling are unavailable with this provider.")
            .arg(service->providerName()));
    }
}
