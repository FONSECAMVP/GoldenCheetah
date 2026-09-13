/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#ifndef GC_TEST_MessageCapture_h
#define GC_TEST_MessageCapture_h

// Shared qDebug capture helper for tests exercising a developer-trace log
// line (the Stage 9 garmin_auth_unknown diagnostic in GarminCredentialsPage /
// GarminMfaPage). Qt's message-handler contract permits concurrent invocation
// from any thread, so the ctor's handler-install + `current_` assignment and
// the hook's read/write must share ONE lock. This is the corrected shape
// (B-OBS001-02) already applied to ObsCapture in testGarminConnectSync.cpp —
// mirrored here verbatim rather than re-deriving it, since both test files
// need it and a shared header avoids duplicating the fix twice.

#include <QMutex>
#include <QMutexLocker>
#include <QString>
#include <QStringList>
#include <QtGlobal>

class MessageCapture
{
  public:
    MessageCapture()
    {
        QMutexLocker locker(&mutex_);
        prev_ = qInstallMessageHandler(&MessageCapture::hook);
        current_ = this;
    }
    ~MessageCapture()
    {
        QMutexLocker locker(&mutex_);
        current_ = nullptr;
        locker.unlock();
        qInstallMessageHandler(prev_);
    }
    MessageCapture(const MessageCapture&) = delete;
    MessageCapture& operator=(const MessageCapture&) = delete;

    QStringList snapshot() const
    {
        QMutexLocker locker(&mutex_);
        return lines_;
    }

  private:
    static void hook(QtMsgType, const QMessageLogContext&, const QString& msg)
    {
        QMutexLocker locker(&mutex_);
        if (current_ != nullptr)
            current_->lines_ << msg;
    }
    // inline (C++17): safe storage for a header included by more than one TU,
    // without a separate out-of-class definition.
    static inline MessageCapture* current_ = nullptr;
    static inline QMutex mutex_;
    QStringList lines_;
    QtMessageHandler prev_;
};

#endif // GC_TEST_MessageCapture_h
