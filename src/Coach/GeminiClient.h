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

#ifndef _GC_GeminiClient_h
#define _GC_GeminiClient_h

#include "LLMService.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QAuthenticator>
#include <QTimer>

class GeminiClient : public LLMService
{
    Q_OBJECT

public:
    explicit GeminiClient(QObject* parent = nullptr);
    ~GeminiClient() override = default;

    // LLMService interface
    void configure(const QVariantMap& settings) override;
    Provider provider() const override { return GoogleGemini; }
    QString providerName() const override { return "Google Gemini"; }
    QString model() const override { return currentModel_; }

    bool isAvailable() const override;
    QString apiKey() const override { return apiKey_; }
    void setApiKey(const QString& key) override { apiKey_ = key; }

    void addSystemMessage(const QString& content) override;
    void addUserMessage(const QString& content) override;
    void addAssistantMessage(const QString& content) override;
    void clearConversation() override;

    void sendMessage(const QString& content) override;
    void sendMessageStream(const QString& content) override;
    void stopStreaming() override;

    double temperature() const override { return temperature_; }
    void setTemperature(double t) override { temperature_ = t; }
    int maxTokens() const override { return maxTokens_; }
    void setMaxTokens(int tokens) override { maxTokens_ = tokens; }

    // Model selection
    QStringList availableModels() const;
    void setModel(const QString& model);

    // Tool use overrides (DES-006)
    void setTools(const QList<LLMService::ToolDef>& tools) override;
    bool supportsTools() const override { return true; }
    void sendToolResult(const QString& callId, const QString& toolName, const QJsonObject& result) override;

private slots:
    void onRequestFinished(QNetworkReply* reply);
    void onAuthenticationRequired(QNetworkReply*, QAuthenticator* authenticator);
    void onSSLErrors(QNetworkReply* reply, const QList<QSslError>& errors);
    void onTimeout();

private:
    void buildRequest(const QString& content, QJsonObject& requestBody, bool streaming);
    void processResponse(const QJsonObject& response);
    void processStreamResponse(const QByteArray& data);
    QString buildEndpointUrl(bool streaming) const;
    void appendToolsToRequest(QJsonObject& requestBody);
    void sendContinuation();

    struct PendingToolCall { QString name; QJsonObject args; };
    QList<LLMService::ToolDef> tools_;
    QList<PendingToolCall> pendingToolCalls_;
    QJsonArray fullMessages_;
    bool useFullMessages_ = false;

    QNetworkAccessManager* networkManager_;
    QNetworkReply* currentReply_ = nullptr;
    QString apiKey_;
    QString baseUrl_ = "https://generativelanguage.googleapis.com/v1beta";
    QString currentModel_ = "gemini-3.1-pro-preview";
    double temperature_ = 0.7;
    int maxTokens_ = 2000;

    QList<Message> conversation_;
    QString systemPrompt_;

    bool streaming_ = false;
    bool streamingRequest_ = false;
    QByteArray streamBuffer_;
    QTimer* timeoutTimer_ = nullptr;

    static constexpr int TIMEOUT_MS = 60000; // 60 seconds
};

#endif // _GC_GeminiClient_h
