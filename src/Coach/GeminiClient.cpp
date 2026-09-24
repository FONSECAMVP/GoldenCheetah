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

#include "GeminiClient.h"
#include "Secrets.h" // For API key lookup

#include <QJsonDocument>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QDebug>

GeminiClient::GeminiClient(QObject* parent)
    : LLMService(parent)
    , networkManager_(new QNetworkAccessManager(this))
{
    // Try to load API key from Secrets.h or environment
    apiKey_ = QString(GCGeminiAPIKey);

    // Connect signals
    connect(networkManager_, &QNetworkAccessManager::finished,
            this, &GeminiClient::onRequestFinished);
    connect(networkManager_, &QNetworkAccessManager::authenticationRequired,
            this, &GeminiClient::onAuthenticationRequired);
    connect(networkManager_, &QNetworkAccessManager::sslErrors,
            this, &GeminiClient::onSSLErrors);

    timeoutTimer_ = new QTimer(this);
    timeoutTimer_->setSingleShot(true);
    connect(timeoutTimer_, &QTimer::timeout, this, &GeminiClient::onTimeout);
}

void GeminiClient::configure(const QVariantMap& settings)
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

bool GeminiClient::isAvailable() const
{
    return !apiKey_.isEmpty();
}

QStringList GeminiClient::availableModels() const
{
    return {
        "gemini-3.1-pro-preview",
        "gemini-3.5-flash",
        "gemini-3.1-flash-lite"
    };
}

void GeminiClient::setModel(const QString& model)
{
    if (availableModels().contains(model)) {
        currentModel_ = model;
    }
}

void GeminiClient::addSystemMessage(const QString& content)
{
    systemPrompt_ = content;
}

void GeminiClient::addUserMessage(const QString& content)
{
    conversation_.append({ "user", content });
}

void GeminiClient::addAssistantMessage(const QString& content)
{
    conversation_.append({ "model", content }); // Gemini uses "model" instead of "assistant"
}

void GeminiClient::clearConversation()
{
    conversation_.clear();
    useFullMessages_ = false;
    fullMessages_ = QJsonArray();
    pendingToolCalls_.clear();
}

void GeminiClient::setTools(const QList<LLMService::ToolDef>& tools)
{
    tools_ = tools;
    useFullMessages_ = false;
    fullMessages_ = QJsonArray();
    pendingToolCalls_.clear();
}

void GeminiClient::appendToolsToRequest(QJsonObject& requestBody)
{
    if (tools_.isEmpty()) return;
    QJsonArray decls;
    for (const auto& t : tools_) {
        decls.append(QJsonObject{
            {"name", t.name},
            {"description", t.description},
            {"parameters", t.inputSchema}
        });
    }
    requestBody["tools"] = QJsonArray{QJsonObject{{"functionDeclarations", decls}}};
}

QString GeminiClient::buildEndpointUrl(bool streaming) const
{
    QString endpoint = streaming ? "streamGenerateContent" : "generateContent";
    return QString("%1/models/%2:%3?key=%4")
        .arg(baseUrl_)
        .arg(currentModel_)
        .arg(endpoint)
        .arg(apiKey_);
}

void GeminiClient::buildRequest(const QString& content, QJsonObject& requestBody, bool streaming)
{
    Q_UNUSED(streaming)

    QJsonArray contents;

    if (useFullMessages_) {
        contents = fullMessages_;
    } else {
        if (!systemPrompt_.isEmpty()) {
            contents.append(QJsonObject{
                {"role", "user"},
                {"parts", QJsonArray{QJsonObject{{"text", systemPrompt_}}}}
            });
        }
        for (const auto& msg : conversation_) {
            contents.append(QJsonObject{
                {"role", msg.role},
                {"parts", QJsonArray{QJsonObject{{"text", msg.content}}}}
            });
        }
        if (!content.isEmpty()) {
            contents.append(QJsonObject{
                {"role", "user"},
                {"parts", QJsonArray{QJsonObject{{"text", content}}}}
            });
        }
    }

    requestBody["contents"] = contents;

    QJsonObject generationConfig;
    generationConfig["temperature"] = temperature_;
    generationConfig["maxOutputTokens"] = maxTokens_;
    requestBody["generationConfig"] = generationConfig;

    appendToolsToRequest(requestBody);
}

void GeminiClient::sendMessage(const QString& content)
{
    if (!isAvailable()) {
        Response errorResponse;
        errorResponse.success = false;
        errorResponse.error = "API key not configured. Please set your Google Gemini API key.";
        emit responseReady(errorResponse);
        return;
    }

    QJsonObject requestBody;

    if (!tools_.isEmpty()) {
        // Tool use path: manage conversation in fullMessages_ (Gemini contents format)
        if (!useFullMessages_) {
            useFullMessages_ = true;
            fullMessages_ = QJsonArray();
            if (!systemPrompt_.isEmpty()) {
                fullMessages_.append(QJsonObject{
                    {"role", "user"},
                    {"parts", QJsonArray{QJsonObject{{"text", systemPrompt_}}}}
                });
            }
            for (const auto& msg : conversation_) {
                fullMessages_.append(QJsonObject{
                    {"role", msg.role},
                    {"parts", QJsonArray{QJsonObject{{"text", msg.content}}}}
                });
            }
        }
        if (!content.isEmpty()) {
            fullMessages_.append(QJsonObject{
                {"role", "user"},
                {"parts", QJsonArray{QJsonObject{{"text", content}}}}
            });
        }
        buildRequest("", requestBody, false);
    } else {
        conversation_.append({ "user", content });
        buildRequest(content, requestBody, false);
    }

    QNetworkRequest request(QUrl(buildEndpointUrl(false)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    currentReply_ = networkManager_->post(request,
        QJsonDocument(requestBody).toJson(QJsonDocument::Compact));

    timeoutTimer_->start(TIMEOUT_MS);
}

void GeminiClient::sendMessageStream(const QString& content)
{
    if (!tools_.isEmpty()) {
        sendMessage(content);
        return;
    }

    if (!isAvailable()) {
        Response errorResponse;
        errorResponse.success = false;
        errorResponse.error = "API key not configured. Please set your Google Gemini API key.";
        emit responseReady(errorResponse);
        return;
    }

    conversation_.append({ "user", content });
    streaming_ = true;
    streamingRequest_ = true;
    streamBuffer_.clear();

    QJsonObject requestBody;
    buildRequest(content, requestBody, true);

    QNetworkRequest request(QUrl(buildEndpointUrl(true)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    currentReply_ = networkManager_->post(request,
        QJsonDocument(requestBody).toJson(QJsonDocument::Compact));

    // Connect to readyRead for streaming
    connect(currentReply_, &QNetworkReply::readyRead, this, [this]() {
        if (streaming_ && currentReply_) {
            QByteArray data = currentReply_->readAll();
            processStreamResponse(data);
        }
    });

    timeoutTimer_->start(TIMEOUT_MS);
}

void GeminiClient::stopStreaming()
{
    if (currentReply_ && streaming_) {
        streaming_ = false;
        currentReply_->abort();
    }
}

void GeminiClient::processResponse(const QJsonObject& response)
{
    QJsonArray candidates = response["candidates"].toArray();
    if (candidates.isEmpty()) {
        Response errorResponse;
        errorResponse.success = false;
        errorResponse.error = "Empty response from Gemini API";
        emit responseReady(errorResponse);
        return;
    }

    QJsonObject candidate = candidates[0].toObject();
    QJsonObject content = candidate["content"].toObject();
    QJsonArray parts = content["parts"].toArray();

    QList<PendingToolCall> newCalls;
    QString textContent;

    for (const QJsonValue& part : parts) {
        QJsonObject partObj = part.toObject();
        if (partObj.contains("functionCall")) {
            QJsonObject fc = partObj["functionCall"].toObject();
            newCalls.append({fc["name"].toString(), fc["args"].toObject()});
        } else {
            textContent += partObj["text"].toString();
        }
    }

    if (!newCalls.isEmpty()) {
        if (!useFullMessages_) {
            useFullMessages_ = true;
            fullMessages_ = QJsonArray();
            if (!systemPrompt_.isEmpty()) {
                fullMessages_.append(QJsonObject{
                    {"role", "user"},
                    {"parts", QJsonArray{QJsonObject{{"text", systemPrompt_}}}}
                });
            }
            for (const auto& msg : conversation_) {
                fullMessages_.append(QJsonObject{
                    {"role", msg.role},
                    {"parts", QJsonArray{QJsonObject{{"text", msg.content}}}}
                });
            }
        }
        fullMessages_.append(QJsonObject{{"role", "model"}, {"parts", parts}});

        if (!textContent.isEmpty()) {
            Response textResp;
            textResp.text = textContent;
            textResp.model = currentModel_;
            textResp.success = true;
            emit responseReady(textResp);
        }

        pendingToolCalls_ = newCalls;
        PendingToolCall first = pendingToolCalls_.takeFirst();
        emit toolCallRequested(first.name, first.name, first.args);
        return;
    }

    if (useFullMessages_) {
        fullMessages_.append(QJsonObject{
            {"role", "model"},
            {"parts", QJsonArray{QJsonObject{{"text", textContent}}}}
        });
    } else {
        conversation_.append({"model", textContent});
    }

    Response result;
    result.text = textContent;
    result.model = currentModel_;
    result.success = true;

    QJsonObject usageMetadata = response["usageMetadata"].toObject();
    result.tokensUsed = usageMetadata["promptTokenCount"].toInt() +
                        usageMetadata["candidatesTokenCount"].toInt();

    emit responseReady(result);
}

void GeminiClient::sendToolResult(const QString& callId, const QString& toolName, const QJsonObject& result)
{
    Q_UNUSED(callId)
    fullMessages_.append(QJsonObject{
        {"role", "user"},
        {"parts", QJsonArray{QJsonObject{
            {"functionResponse", QJsonObject{
                {"name", toolName},
                {"response", result}
            }}
        }}}
    });

    if (!pendingToolCalls_.isEmpty()) {
        PendingToolCall next = pendingToolCalls_.takeFirst();
        emit toolCallRequested(next.name, next.name, next.args);
        return;
    }

    sendContinuation();
}

void GeminiClient::sendContinuation()
{
    QJsonObject requestBody;
    buildRequest("", requestBody, false);

    QNetworkRequest request(QUrl(buildEndpointUrl(false)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    currentReply_ = networkManager_->post(request,
        QJsonDocument(requestBody).toJson(QJsonDocument::Compact));
    timeoutTimer_->start(TIMEOUT_MS);
}

void GeminiClient::processStreamResponse(const QByteArray& data)
{
    streamBuffer_ += data;

    // Gemini streaming returns JSON objects separated by newlines
    while (true) {
        int endPos = streamBuffer_.indexOf('\n');
        if (endPos == -1) break;

        QByteArray line = streamBuffer_.left(endPos).trimmed();
        streamBuffer_ = streamBuffer_.mid(endPos + 1);

        if (line.isEmpty()) continue;

        QJsonDocument doc = QJsonDocument::fromJson(line);
        if (!doc.isNull() && doc.isObject()) {
            QJsonObject obj = doc.object();
            
            // Check for error
            if (obj.contains("error")) {
                streaming_ = false;
                QString error = obj["error"].toObject()["message"].toString();
                emit streamingError(error);
                return;
            }

            // Extract text from candidates
            QJsonArray candidates = obj["candidates"].toArray();
            if (!candidates.isEmpty()) {
                QJsonObject candidate = candidates[0].toObject();
                QJsonObject content = candidate["content"].toObject();
                QJsonArray parts = content["parts"].toArray();

                for (const QJsonValue& part : parts) {
                    QString text = part.toObject()["text"].toString();
                    if (!text.isEmpty()) {
                        emit streamingChunk(text);
                    }
                }

                // Check if this is the final chunk
                QString finishReason = candidate["finishReason"].toString();
                if (!finishReason.isEmpty() && finishReason != "STOP") {
                    streaming_ = false;
                    if (finishReason == "MAX_TOKENS") {
                        emit streamingError("Response truncated: maximum tokens reached");
                    } else if (finishReason == "SAFETY") {
                        emit streamingError("Response blocked by safety filters");
                    }
                    return;
                } else if (finishReason == "STOP") {
                    streaming_ = false;
                    emit streamingFinished();
                    return;
                }
            }
        }
    }
}

void GeminiClient::onRequestFinished(QNetworkReply* reply)
{
    timeoutTimer_->stop();

    if (reply != currentReply_) return;

    if (reply->error() != QNetworkReply::NoError) {
        Response errorResponse;
        errorResponse.success = false;
        errorResponse.error = reply->errorString();

        if (reply->error() == QNetworkReply::AuthenticationRequiredError) {
            errorResponse.error = "Gemini API authentication failed. Please check your API key.";
        }

        emit responseReady(errorResponse);
        reply->deleteLater();
        return;
    }

    if (streamingRequest_) {
        // Streaming data was already processed via readyRead signals;
        // just finalise in case the STOP marker was missed.
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
        errorResponse.error = "Invalid JSON response from Gemini API";
        emit responseReady(errorResponse);
        reply->deleteLater();
        return;
    }

    QJsonObject response = doc.object();

    // Check for API error
    if (response.contains("error")) {
        Response errorResponse;
        errorResponse.success = false;
        errorResponse.error = response["error"].toObject()["message"].toString();
        emit responseReady(errorResponse);
        reply->deleteLater();
        return;
    }

    processResponse(response);

    reply->deleteLater();
}

void GeminiClient::onAuthenticationRequired(QNetworkReply*, QAuthenticator* authenticator)
{
    Q_UNUSED(authenticator)
    // Gemini uses API key in URL, not basic auth
}

void GeminiClient::onSSLErrors(QNetworkReply* reply, const QList<QSslError>& errors)
{
    // In production, you should verify SSL certificates
    // For development, we allow all SSL errors
    Q_UNUSED(errors)
    reply->ignoreSslErrors();
}

void GeminiClient::onTimeout()
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
