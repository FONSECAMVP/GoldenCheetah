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

#include "AnthropicClient.h"
#include "Secrets.h" // For API key lookup

#include <QJsonDocument>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QUrl>
#include <QDebug>

AnthropicClient::AnthropicClient(QObject* parent)
    : LLMService(parent)
    , networkManager_(new QNetworkAccessManager(this))
{
    // Try to load API key from Secrets.h or environment
    apiKey_ = QString(GCAnthropicAPIKey);

    // Connect signals
    connect(networkManager_, &QNetworkAccessManager::finished,
            this, &AnthropicClient::onRequestFinished);
    connect(networkManager_, &QNetworkAccessManager::authenticationRequired,
            this, &AnthropicClient::onAuthenticationRequired);
    connect(networkManager_, &QNetworkAccessManager::sslErrors,
            this, &AnthropicClient::onSSLErrors);

    timeoutTimer_ = new QTimer(this);
    timeoutTimer_->setSingleShot(true);
    connect(timeoutTimer_, &QTimer::timeout, this, &AnthropicClient::onTimeout);
}

void AnthropicClient::configure(const QVariantMap& settings)
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

bool AnthropicClient::isAvailable() const
{
    return !apiKey_.isEmpty();
}

QStringList AnthropicClient::availableModels() const
{
    return {
        "claude-sonnet-4-20250514",
        "claude-haiku-4-20250414",
        "claude-3-5-sonnet-20241022",
        "claude-3-5-haiku-20241022",
        "claude-3-opus-20240229"
    };
}

void AnthropicClient::setModel(const QString& model)
{
    if (availableModels().contains(model)) {
        currentModel_ = model;
    }
}

void AnthropicClient::addSystemMessage(const QString& content)
{
    systemPrompt_ = content;
}

void AnthropicClient::addUserMessage(const QString& content)
{
    conversation_.append({ "user", content });
}

void AnthropicClient::addAssistantMessage(const QString& content)
{
    conversation_.append({ "assistant", content });
}

void AnthropicClient::clearConversation()
{
    conversation_.clear();
    useFullMessages_ = false;
    fullMessages_ = QJsonArray();
    pendingToolCalls_.clear();
}

void AnthropicClient::appendToolsToRequest(QJsonObject& requestBody)
{
    if (tools_.isEmpty()) return;
    QJsonArray toolsArr;
    for (const auto& t : tools_) {
        toolsArr.append(QJsonObject{{"name",t.name},{"description",t.description},{"input_schema",t.inputSchema}});
    }
    requestBody["tools"] = toolsArr;
    requestBody["tool_choice"] = QJsonObject{{"type","auto"}};
}

void AnthropicClient::buildRequest(const QString& content, QJsonObject& requestBody, bool streaming)
{
    QJsonArray messages;

    if (useFullMessages_) {
        messages = fullMessages_;
    } else {
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

    requestBody["model"] = currentModel_;
    requestBody["messages"] = messages;
    requestBody["max_tokens"] = maxTokens_;
    requestBody["temperature"] = temperature_;

    if (!systemPrompt_.isEmpty()) {
        requestBody["system"] = systemPrompt_;
    }

    if (streaming) {
        requestBody["stream"] = true;
    }

    appendToolsToRequest(requestBody);
}

void AnthropicClient::sendMessage(const QString& content)
{
    if (!isAvailable()) {
        Response errorResponse;
        errorResponse.success = false;
        errorResponse.error = "API key not configured. Please set your Anthropic API key.";
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

    QNetworkRequest request(QUrl(baseUrl_ + "/messages"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("x-api-key", apiKey_.toUtf8());
    request.setRawHeader("anthropic-version", ANTHROPIC_VERSION);

    currentReply_ = networkManager_->post(request,
        QJsonDocument(requestBody).toJson(QJsonDocument::Compact));

    timeoutTimer_->start(TIMEOUT_MS);
}

void AnthropicClient::sendMessageStream(const QString& content)
{
    if (!tools_.isEmpty()) {
        sendMessage(content);
        return;
    }

    if (!isAvailable()) {
        Response errorResponse;
        errorResponse.success = false;
        errorResponse.error = "API key not configured. Please set your Anthropic API key.";
        emit responseReady(errorResponse);
        return;
    }

    conversation_.append({ "user", content });
    streaming_ = true;
    streamingRequest_ = true;
    streamBuffer_.clear();

    QJsonObject requestBody;
    buildRequest("", requestBody, true);

    QNetworkRequest request(QUrl(baseUrl_ + "/messages"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("x-api-key", apiKey_.toUtf8());
    request.setRawHeader("anthropic-version", ANTHROPIC_VERSION);

    currentReply_ = networkManager_->post(request,
        QJsonDocument(requestBody).toJson(QJsonDocument::Compact));

    // Connect to readyRead for streaming
    connect(currentReply_, &QNetworkReply::readyRead, this, [this]() {
        if (!currentReply_) return;

        // Check for HTTP errors before processing as stream
        int httpStatus = currentReply_->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (httpStatus >= 400) {
            QByteArray body = currentReply_->readAll();
            streaming_ = false;
            streamingRequest_ = false;

            QString errorMsg;
            QJsonDocument errDoc = QJsonDocument::fromJson(body);
            if (!errDoc.isNull() && errDoc.isObject()) {
                QJsonObject errObj = errDoc.object()["error"].toObject();
                errorMsg = QString("Anthropic API error (%1): %2")
                    .arg(errObj["type"].toString(), errObj["message"].toString());
            } else {
                errorMsg = QString("Anthropic API HTTP %1: %2").arg(httpStatus).arg(QString::fromUtf8(body.left(200)));
            }
            emit streamingError(errorMsg);
            return;
        }

        if (streaming_) {
            QByteArray data = currentReply_->readAll();
            processStreamResponse(data);
        }
    });

    timeoutTimer_->start(TIMEOUT_MS);
}

void AnthropicClient::stopStreaming()
{
    if (currentReply_ && streaming_) {
        streaming_ = false;
        currentReply_->abort();
    }
}

void AnthropicClient::processResponse(const QJsonObject& response)
{
    QString stopReason = response["stop_reason"].toString();
    QJsonArray contentBlocks = response["content"].toArray();

    QString textContent;
    QList<PendingToolCall> newCalls;

    for (const QJsonValue& item : contentBlocks) {
        QJsonObject obj = item.toObject();
        if (obj["type"].toString() == "text") {
            textContent += obj["text"].toString();
        } else if (obj["type"].toString() == "tool_use") {
            newCalls.append({obj["id"].toString(), obj["name"].toString(), obj["input"].toObject()});
        }
    }

    if (stopReason == "tool_use" && !newCalls.isEmpty()) {
        if (!useFullMessages_) {
            useFullMessages_ = true;
            fullMessages_ = QJsonArray();
            for (const auto& msg : conversation_) {
                fullMessages_.append(QJsonObject{{"role",msg.role},{"content",msg.content}});
            }
        }
        fullMessages_.append(QJsonObject{{"role","assistant"},{"content",contentBlocks}});

        pendingToolCalls_ = newCalls;

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

    if (useFullMessages_) {
        fullMessages_.append(QJsonObject{{"role","assistant"},{"content",textContent}});
    } else {
        conversation_.append({"assistant", textContent});
    }

    if (textContent.isEmpty() && contentBlocks.isEmpty()) {
        Response errorResponse;
        errorResponse.success = false;
        errorResponse.error = "Empty response from Anthropic API";
        emit responseReady(errorResponse);
        return;
    }

    Response result;
    result.text = textContent;
    result.model = currentModel_;
    result.success = true;

    QJsonObject usage = response["usage"].toObject();
    result.tokensUsed = usage["input_tokens"].toInt() + usage["output_tokens"].toInt();

    emit responseReady(result);
}

void AnthropicClient::setTools(const QList<LLMService::ToolDef>& tools)
{
    tools_ = tools;
    useFullMessages_ = false;
    fullMessages_ = QJsonArray();
    pendingToolCalls_.clear();
}

void AnthropicClient::sendToolResult(const QString& callId, const QString& toolName, const QJsonObject& result)
{
    Q_UNUSED(toolName)
    QJsonObject toolResultBlock{
        {"type", "tool_result"},
        {"tool_use_id", callId},
        {"content", QString::fromUtf8(QJsonDocument(result).toJson(QJsonDocument::Compact))}
    };
    fullMessages_.append(QJsonObject{
        {"role","user"},
        {"content", QJsonArray{toolResultBlock}}
    });

    if (!pendingToolCalls_.isEmpty()) {
        PendingToolCall next = pendingToolCalls_.takeFirst();
        emit toolCallRequested(next.id, next.name, next.args);
        return;
    }

    sendContinuation();
}

void AnthropicClient::sendContinuation()
{
    QJsonObject requestBody;
    requestBody["model"] = currentModel_;
    requestBody["messages"] = fullMessages_;
    requestBody["max_tokens"] = maxTokens_;
    requestBody["temperature"] = temperature_;
    if (!systemPrompt_.isEmpty()) requestBody["system"] = systemPrompt_;
    appendToolsToRequest(requestBody);

    QNetworkRequest request(QUrl(baseUrl_ + "/messages"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("x-api-key", apiKey_.toUtf8());
    request.setRawHeader("anthropic-version", ANTHROPIC_VERSION);

    currentReply_ = networkManager_->post(request,
        QJsonDocument(requestBody).toJson(QJsonDocument::Compact));
    timeoutTimer_->start(TIMEOUT_MS);
}

void AnthropicClient::processStreamResponse(const QByteArray& data)
{
    streamBuffer_ += data;

    // Process SSE format: "event: ...\ndata: {...}\n\n"
    while (true) {
        int endPos = streamBuffer_.indexOf("\n\n");
        if (endPos == -1) break;

        QByteArray block = streamBuffer_.left(endPos);
        streamBuffer_ = streamBuffer_.mid(endPos + 2);

        // Parse event and data
        QList<QByteArray> lines = block.split('\n');
        QString eventType;
        QByteArray jsonData;

        for (const QByteArray& line : lines) {
            if (line.startsWith("event: ")) {
                eventType = QString::fromUtf8(line.mid(7));
            } else if (line.startsWith("data: ")) {
                jsonData = line.mid(6);
            }
        }

        if (eventType == "content_block_delta" && !jsonData.isEmpty()) {
            QJsonDocument doc = QJsonDocument::fromJson(jsonData);
            if (!doc.isNull() && doc.isObject()) {
                QJsonObject obj = doc.object();
                QJsonObject delta = obj["delta"].toObject();
                QString text = delta["text"].toString();
                if (!text.isEmpty()) {
                    emit streamingChunk(text);
                }
            }
        } else if (eventType == "message_stop") {
            streaming_ = false;
            emit streamingFinished();
            return;
        } else if (eventType == "error") {
            streaming_ = false;
            QJsonDocument doc = QJsonDocument::fromJson(jsonData);
            if (!doc.isNull() && doc.isObject()) {
                QString error = doc.object()["error"].toObject()["message"].toString();
                emit streamingError(error);
            }
            return;
        }
    }
}

void AnthropicClient::onRequestFinished(QNetworkReply* reply)
{
    timeoutTimer_->stop();

    if (reply != currentReply_) return;

    if (reply->error() != QNetworkReply::NoError) {
        Response errorResponse;
        errorResponse.success = false;

        // Read the response body for the actual API error message
        QByteArray body = reply->readAll();
        QJsonDocument errDoc = QJsonDocument::fromJson(body);
        if (!errDoc.isNull() && errDoc.isObject()) {
            QJsonObject errObj = errDoc.object();
            if (errObj.contains("error")) {
                QString apiError = errObj["error"].toObject()["message"].toString();
                QString errType = errObj["error"].toObject()["type"].toString();
                errorResponse.error = QString("Anthropic API error (%1): %2").arg(errType, apiError);
            } else {
                errorResponse.error = reply->errorString();
            }
        } else {
            errorResponse.error = reply->errorString();
        }

        if (reply->error() == QNetworkReply::AuthenticationRequiredError) {
            errorResponse.error = "Anthropic API authentication failed. Please check your API key.";
        }

        if (streamingRequest_) {
            streamingRequest_ = false;
            streaming_ = false;
            emit streamingError(errorResponse.error);
        }

        emit responseReady(errorResponse);
        reply->deleteLater();
        return;
    }

    if (streamingRequest_) {
        // Streaming data was already processed via readyRead signals;
        // just finalise in case the message_stop event was missed.
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
        errorResponse.error = "Invalid JSON response from Anthropic API";
        emit responseReady(errorResponse);
        reply->deleteLater();
        return;
    }

    QJsonObject response = doc.object();
    processResponse(response);

    reply->deleteLater();
}

void AnthropicClient::onAuthenticationRequired(QNetworkReply*, QAuthenticator* authenticator)
{
    Q_UNUSED(authenticator)
    // Anthropic uses API key in header, not basic auth
}

void AnthropicClient::onSSLErrors(QNetworkReply* reply, const QList<QSslError>& errors)
{
    // In production, you should verify SSL certificates
    // For development, we allow all SSL errors
    Q_UNUSED(errors)
    reply->ignoreSslErrors();
}

void AnthropicClient::onTimeout()
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
