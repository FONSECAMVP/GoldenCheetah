/*
 * Copyright (c) 2024 Golden
 *
 * ThisCheetah Contributor program is free software; you can redistribute it and/or modify it
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

#include "OpenAIClient.h"
#include "Secrets.h" // For API key lookup

#include <QJsonDocument>
#include <QJsonArray>
#include <QHttpMultiPart>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QDebug>

OpenAIClient::OpenAIClient(QObject* parent)
    : LLMService(parent)
    , networkManager_(new QNetworkAccessManager(this))
{
    // Try to load API key from Secrets.h or environment
    apiKey_ = QString(GCOpenAIAPIKey);

    // Connect signals
    connect(networkManager_, &QNetworkAccessManager::finished,
            this, &OpenAIClient::onRequestFinished);
    connect(networkManager_, &QNetworkAccessManager::authenticationRequired,
            this, &OpenAIClient::onAuthenticationRequired);
    connect(networkManager_, &QNetworkAccessManager::sslErrors,
            this, &OpenAIClient::onSSLErrors);

    timeoutTimer_ = new QTimer(this);
    timeoutTimer_->setSingleShot(true);
    connect(timeoutTimer_, &QTimer::timeout, this, &OpenAIClient::onTimeout);
}

void OpenAIClient::configure(const QVariantMap& settings)
{
    if (settings.contains("apiKey")) {
        apiKey_ = settings["apiKey"].toString();
    }
    if (settings.contains("model")) {
        currentModel_ = settings["model"].toString();
    }
    if (settings.contains("temperature")) {
        temperature_ = settings["temperature"].toDouble();
    }
    if (settings.contains("maxTokens")) {
        maxTokens_ = settings["maxTokens"].toInt();
    }
    if (settings.contains("baseUrl")) {
        baseUrl_ = settings["baseUrl"].toString();
    }
}

bool OpenAIClient::isAvailable() const
{
    return !apiKey_.isEmpty();
}

QStringList OpenAIClient::availableModels() const
{
    return {
        "gpt-4o",
        "gpt-4o-mini",
        "gpt-4-turbo",
        "gpt-4",
        "gpt-3.5-turbo"
    };
}

void OpenAIClient::setModel(const QString& model)
{
    if (availableModels().contains(model)) {
        currentModel_ = model;
    }
}

void OpenAIClient::addSystemMessage(const QString& content)
{
    systemPrompt_ = content;
}

void OpenAIClient::addUserMessage(const QString& content)
{
    conversation_.append({ "user", content });
}

void OpenAIClient::addAssistantMessage(const QString& content)
{
    conversation_.append({ "assistant", content });
}

void OpenAIClient::clearConversation()
{
    conversation_.clear();
    useFullMessages_ = false;
    fullMessages_ = QJsonArray();
    pendingToolCalls_.clear();
}

void OpenAIClient::appendToolsToRequest(QJsonObject& requestBody)
{
    if (tools_.isEmpty()) return;
    QJsonArray toolsArr;
    for (const auto& t : tools_) {
        toolsArr.append(QJsonObject{
            {"type","function"},
            {"function", QJsonObject{
                {"name",t.name},
                {"description",t.description},
                {"parameters",t.inputSchema}
            }}
        });
    }
    requestBody["tools"] = toolsArr;
    requestBody["tool_choice"] = QString("auto");
}

void OpenAIClient::buildRequest(const QString& content, QJsonObject& requestBody, bool streaming)
{
    QJsonArray messages;

    if (useFullMessages_) {
        messages = fullMessages_;
    } else {
        if (!systemPrompt_.isEmpty()) {
            messages.append(QJsonObject{
                {"role", "system"},
                {"content", systemPrompt_}
            });
        }
        for (const auto& msg : conversation_) {
            messages.append(QJsonObject{
                {"role", msg.role},
                {"content", msg.content}
            });
        }
        if (!content.isEmpty()) {
            messages.append(QJsonObject{
                {"role", "user"},
                {"content", content}
            });
        }
    }

    requestBody["messages"] = messages;
    requestBody["model"] = currentModel_;
    requestBody["temperature"] = temperature_;
    requestBody["max_tokens"] = maxTokens_;

    if (streaming) {
        requestBody["stream"] = true;
        requestBody["stream_options"] = QJsonObject{{"include_usage", true}};
    }

    appendToolsToRequest(requestBody);
}

bool OpenAIClient::checkRateLimit()
{
    QDateTime now = QDateTime::currentDateTime();
    
    // Reset counter if more than a minute has passed
    if (lastRequestTime_.isValid() &&
        lastRequestTime_.secsTo(now) >= 60) {
        requestCount_ = 0;
    }
    
    // Check if we've exceeded the limit
    if (requestCount_ >= MAX_REQUESTS_PER_MINUTE) {
        emit rateLimitWarning("Rate limit reached. Please wait before sending more requests.");
        return false;
    }
    
    // Update counters
    lastRequestTime_ = now;
    requestCount_++;
    return true;
}

void OpenAIClient::sendMessage(const QString& content)
{
    if (!isAvailable()) {
        Response errorResponse;
        errorResponse.success = false;
        errorResponse.error = "API key not configured. Please set your OpenAI API key.";
        emit responseReady(errorResponse);
        return;
    }

    if (!checkRateLimit()) {
        Response errorResponse;
        errorResponse.success = false;
        errorResponse.error = "Rate limit exceeded. Please wait a moment before trying again.";
        emit responseReady(errorResponse);
        return;
    }

    if (!content.isEmpty()) {
        if (useFullMessages_) {
            fullMessages_.append(QJsonObject{{"role","user"},{"content",content}});
        } else {
            conversation_.append({ "user", content });
        }
    }

    QJsonObject requestBody;
    buildRequest("", requestBody, false);

    QNetworkRequest request(QUrl(baseUrl_ + "/chat/completions"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + apiKey_.toLocal8Bit());

    currentReply_ = networkManager_->post(request,
        QJsonDocument(requestBody).toJson(QJsonDocument::Compact));

    timeoutTimer_->start(TIMEOUT_MS);
}

void OpenAIClient::sendMessageStream(const QString& content)
{
    if (!tools_.isEmpty()) {
        sendMessage(content);
        return;
    }

    if (!isAvailable()) {
        Response errorResponse;
        errorResponse.success = false;
        errorResponse.error = "API key not configured. Please set your OpenAI API key.";
        emit responseReady(errorResponse);
        return;
    }

    if (!checkRateLimit()) {
        Response errorResponse;
        errorResponse.success = false;
        errorResponse.error = "Rate limit exceeded. Please wait a moment before trying again.";
        emit responseReady(errorResponse);
        return;
    }

    conversation_.append({ "user", content });
    streaming_ = true;
    streamingRequest_ = true;
    streamBuffer_.clear();

    QJsonObject requestBody;
    buildRequest("", requestBody, true);

    QNetworkRequest request(QUrl(baseUrl_ + "/chat/completions"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + apiKey_.toLocal8Bit());

    currentReply_ = networkManager_->post(request,
        QJsonDocument(requestBody).toJson(QJsonDocument::Compact));

    // For streaming, process chunks as they arrive via readyRead
    connect(currentReply_, &QNetworkReply::readyRead, this, [this]() {
        if (streaming_ && currentReply_) {
            QByteArray data = currentReply_->readAll();
            processStreamResponse(data);
        }
    });

    timeoutTimer_->start(TIMEOUT_MS);
}

void OpenAIClient::stopStreaming()
{
    if (currentReply_ && streaming_) {
        streaming_ = false;
        currentReply_->abort();
    }
}

void OpenAIClient::processResponse(const QJsonObject& response)
{
    QJsonArray choices = response["choices"].toArray();
    if (choices.isEmpty()) {
        Response errorResponse;
        errorResponse.success = false;
        errorResponse.error = "Empty choices in OpenAI response";
        emit responseReady(errorResponse);
        return;
    }

    QJsonObject choice = choices[0].toObject();
    QString finishReason = choice["finish_reason"].toString();
    QJsonObject message = choice["message"].toObject();

    if (finishReason == "tool_calls") {
        QJsonArray toolCalls = message["tool_calls"].toArray();
        QList<PendingToolCall> newCalls;
        for (const QJsonValue& tc : toolCalls) {
            QJsonObject tcObj = tc.toObject();
            QString id = tcObj["id"].toString();
            QString name = tcObj["function"].toObject()["name"].toString();
            QJsonDocument argsDoc = QJsonDocument::fromJson(
                tcObj["function"].toObject()["arguments"].toString().toUtf8());
            newCalls.append({id, name, argsDoc.isObject() ? argsDoc.object() : QJsonObject()});
        }

        if (!useFullMessages_) {
            useFullMessages_ = true;
            fullMessages_ = QJsonArray();
            if (!systemPrompt_.isEmpty()) {
                fullMessages_.append(QJsonObject{{"role","system"},{"content",systemPrompt_}});
            }
            for (const auto& msg : conversation_) {
                fullMessages_.append(QJsonObject{{"role",msg.role},{"content",msg.content}});
            }
        }
        fullMessages_.append(message);

        pendingToolCalls_ = newCalls;

        QString textContent = message["content"].toString();
        if (!textContent.isEmpty()) {
            Response textResp;
            textResp.text = textContent;
            textResp.model = currentModel_;
            textResp.success = true;
            emit responseReady(textResp);
        }

        PendingToolCall first = pendingToolCalls_.takeFirst();
        emit toolCallRequested(first.id, first.name, first.args);
        return;
    }

    QString content = message["content"].toString();

    if (useFullMessages_) {
        fullMessages_.append(QJsonObject{{"role","assistant"},{"content",content}});
    } else {
        conversation_.append({"assistant", content});
    }

    Response result;
    result.text = content;
    result.model = currentModel_;
    result.success = true;

    QJsonObject usage = response["usage"].toObject();
    result.tokensUsed = usage["completion_tokens"].toInt() + usage["prompt_tokens"].toInt();

    emit responseReady(result);
}

void OpenAIClient::setTools(const QList<LLMService::ToolDef>& tools)
{
    tools_ = tools;
    useFullMessages_ = false;
    fullMessages_ = QJsonArray();
    pendingToolCalls_.clear();
}

void OpenAIClient::sendToolResult(const QString& callId, const QString& toolName, const QJsonObject& result)
{
    Q_UNUSED(toolName)
    fullMessages_.append(QJsonObject{
        {"role","tool"},
        {"tool_call_id", callId},
        {"content", QString::fromUtf8(QJsonDocument(result).toJson(QJsonDocument::Compact))}
    });

    if (!pendingToolCalls_.isEmpty()) {
        PendingToolCall next = pendingToolCalls_.takeFirst();
        emit toolCallRequested(next.id, next.name, next.args);
        return;
    }

    sendContinuation();
}

void OpenAIClient::sendContinuation()
{
    QJsonObject requestBody;
    requestBody["messages"] = fullMessages_;
    requestBody["model"] = currentModel_;
    requestBody["temperature"] = temperature_;
    requestBody["max_tokens"] = maxTokens_;
    appendToolsToRequest(requestBody);

    QNetworkRequest request(QUrl(baseUrl_ + "/chat/completions"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + apiKey_.toLocal8Bit());

    currentReply_ = networkManager_->post(request,
        QJsonDocument(requestBody).toJson(QJsonDocument::Compact));
    timeoutTimer_->start(TIMEOUT_MS);
}

void OpenAIClient::processStreamResponse(const QByteArray& data)
{
    streamBuffer_ += data;
    QString chunk;

    // Process SSE format: "data: {...}\n\n"
    while (true) {
        int endPos = streamBuffer_.indexOf("\n\n");
        if (endPos == -1) break;

        QByteArray line = streamBuffer_.left(endPos);
        streamBuffer_ = streamBuffer_.mid(endPos + 2);

        if (line.startsWith("data: ")) {
            QByteArray jsonData = line.mid(6);
            if (jsonData == "[DONE]") {
                streaming_ = false;
                emit streamingFinished();
                return;
            }

            QJsonDocument doc = QJsonDocument::fromJson(jsonData);
            if (!doc.isNull() && doc.isObject()) {
                QJsonObject obj = doc.object();
                QJsonArray choices = obj["choices"].toArray();
                if (!choices.isEmpty()) {
                    QString delta = choices[0].toObject()["delta"].toObject()["content"].toString();
                    if (!delta.isEmpty()) {
                        emit streamingChunk(delta);
                    }
                }
            }
        }
    }
}

QString OpenAIClient::extractStreamChunk(const QByteArray& data)
{
    Q_UNUSED(data)
    return QString();
}

void OpenAIClient::onRequestFinished(QNetworkReply* reply)
{
    timeoutTimer_->stop();

    if (reply != currentReply_) return;

    if (reply->error() != QNetworkReply::NoError) {
        Response errorResponse;
        errorResponse.success = false;
        errorResponse.error = reply->errorString();

        if (reply->error() == QNetworkReply::AuthenticationRequiredError) {
            errorResponse.error = "OpenAI API authentication failed. Please check your API key.";
        }

        emit responseReady(errorResponse);
        reply->deleteLater();
        return;
    }

    if (streamingRequest_) {
        // Streaming data was already processed via readyRead signals;
        // just finalise in case the [DONE] marker was missed.
        streamingRequest_ = false;
        streaming_ = false;
        emit streamingFinished();
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    if (doc.isNull() || !doc.isObject()) {
        Response errorResponse;
        errorResponse.success = false;
        errorResponse.error = "Invalid JSON response from API: " + QString::fromUtf8(data.left(200));
        emit responseReady(errorResponse);
        reply->deleteLater();
        return;
    }

    QJsonObject response = doc.object();
    processResponse(response);

    reply->deleteLater();
}

void OpenAIClient::onAuthenticationRequired(QNetworkReply*, QAuthenticator* authenticator)
{
    Q_UNUSED(authenticator)
    // OpenAI uses Bearer token in header, not basic auth
}

void OpenAIClient::onSSLErrors(QNetworkReply* reply, const QList<QSslError>& errors)
{
    // In production, you should verify SSL certificates
    // For development, we allow all SSL errors
    Q_UNUSED(errors)
    reply->ignoreSslErrors();
}

void OpenAIClient::onTimeout()
{
    if (currentReply_) {
        currentReply_->abort();
        streaming_ = false;

        Response errorResponse;
        errorResponse.success = false;
        errorResponse.error = "Request timed out. Please try again.";
        emit streamingError(errorResponse.error);
        emit responseReady(errorResponse);
    }
}
