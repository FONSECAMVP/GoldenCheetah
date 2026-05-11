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

#ifndef _GC_CoachChatWidget_h
#define _GC_CoachChatWidget_h

#include "LLMService.h"
#include "AthleteContext.h"
#include "PromptBuilder.h"
#include "GCToolExecutor.h"

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QComboBox>
#include <QProgressBar>
#include <QScrollArea>
#include <QSplitter>
#include <QToolButton>
#include <QMenu>
#include <QActionGroup>

class Context;
class OpenAIClient;
class AnthropicClient;
class GeminiClient;

class CoachChatWidget : public QDialog
{
    Q_OBJECT

public:
    explicit CoachChatWidget(Context* context, QWidget *parent = nullptr);
    ~CoachChatWidget() override = default;

public slots:
    void onSendMessage();
    void onResponseReceived(const LLMService::Response& response);
    void onStreamingChunk(const QString& chunk);
    void onStreamingFinished();
    void onStreamingError(const QString& error);
    void onApiKeyChanged(const QString& apiKey);
    void onModelChanged(const QString& model);
    void onTemperatureChanged(double temp);
    void onClearConversation();
    void onShowSettings();
    void onPhaseChanged(int phase);
    void onSuggestedQuestionClicked(const QString& question);
    void onSettingsButtonClicked();

signals:
    void messageSent(const QString& message);
    void coachingPhaseChanged(PromptBuilder::CoachingPhase phase);

private:
    void setupUi();
    void setupConnections();
    void updateSuggestions();
    void appendMessage(const QString& role, const QString& content);
    void appendStreamingMessage(const QString& role, const QString& content);
    void showLoadingIndicator();
    void hideLoadingIndicator();
    void scrollToBottom();
    void updateConnectionStatus();

    // UI components
    QVBoxLayout* mainLayout_;
    QWidget* headerWidget_;
    QHBoxLayout* headerLayout_;
    QLabel* phaseLabel_;
    QComboBox* phaseComboBox_;
    QToolButton* settingsButton_;
    QToolButton* clearButton_;

    QScrollArea* chatScrollArea_;
    QWidget* chatWidget_;
    QVBoxLayout* chatLayout_;

    QWidget* inputWidget_;
    QHBoxLayout* inputLayout_;
    QTextEdit* inputTextEdit_;
    QPushButton* sendButton_;

    QWidget* suggestionsWidget_;
    QVBoxLayout* suggestionsLayout_;
    QList<QPushButton*> suggestionButtons_;

    QWidget* loadingWidget_;
    QProgressBar* loadingBar_;
    QLabel* loadingLabel_;

    void switchProvider(const QString& providerName);
    void connectLLMSignals(LLMService* service);

    // Core components
    Context* context_;
    LLMService* llmService_ = nullptr;
    OpenAIClient* openaiClient_ = nullptr;
    AnthropicClient* anthropicClient_ = nullptr;
    GeminiClient* geminiClient_ = nullptr;
    AthleteContextAggregator* contextAggregator_;
    PromptBuilder* promptBuilder_;
    GCToolExecutor* toolExecutor_;

    // State
    PromptBuilder::CoachingPhase currentPhase_;
    bool isStreaming_;
    QString currentStreamingMessage_;
    QString currentProvider_;
    bool systemPromptSent_;         // whether system prompt has been sent this session
    QString lastSystemPrompt_;      // cached system prompt to detect data changes
};

#endif // _GC_CoachChatWidget_h
