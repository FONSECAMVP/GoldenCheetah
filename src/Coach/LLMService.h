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

#ifndef _GC_LLMService_h
#define _GC_LLMService_h

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

// Abstract interface for LLM services
class LLMService : public QObject
{
    Q_OBJECT

public:
    enum Provider {
        OpenAI,
        Anthropic,
        GoogleGemini,
        LocalOllama
    };
    Q_ENUM(Provider)

    struct Response {
        QString text;
        QString model;
        int tokensUsed = 0;
        double responseTimeMs = 0.0;
        bool success = false;
        QString error;
    };

    struct Message {
        QString role; // "system", "user", "assistant"
        QString content;
    };

    struct ToolDef {
        QString name;
        QString description;
        QJsonObject inputSchema;
    };

    virtual void setTools(const QList<ToolDef>& tools) { Q_UNUSED(tools) }
    virtual bool supportsTools() const { return false; }
    virtual void sendToolResult(const QString& callId, const QString& toolName, const QJsonObject& result) {
        Q_UNUSED(callId) Q_UNUSED(toolName) Q_UNUSED(result)
    }

    explicit LLMService(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~LLMService() = default;

    // Configuration
    virtual void configure(const QVariantMap& settings) = 0;
    virtual Provider provider() const = 0;
    virtual QString providerName() const = 0;
    virtual QString model() const = 0;

    // Availability
    virtual bool isAvailable() const = 0;
    virtual QString apiKey() const = 0;
    virtual void setApiKey(const QString& key) = 0;

    // Conversation management
    virtual void addSystemMessage(const QString& content) = 0;
    virtual void addUserMessage(const QString& content) = 0;
    virtual void addAssistantMessage(const QString& content) = 0;
    virtual void clearConversation() = 0;

    // Streaming and non-streaming requests
    virtual void sendMessage(const QString& content) = 0;
    virtual void sendMessageStream(const QString& content) = 0;
    virtual void stopStreaming() = 0;

    // Settings
    virtual double temperature() const = 0;
    virtual void setTemperature(double t) = 0;
    virtual int maxTokens() const = 0;
    virtual void setMaxTokens(int tokens) = 0;

signals:
    void responseReady(const LLMService::Response& response);
    void streamingChunk(const QString& chunk);
    void streamingFinished();
    void streamingError(const QString& error);
    void rateLimitWarning(const QString& message);
    void toolCallRequested(const QString& callId, const QString& toolName, const QJsonObject& args);
};

#endif // _GC_LLMService_h
