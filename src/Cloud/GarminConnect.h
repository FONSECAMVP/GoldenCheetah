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

#ifndef GC_GarminConnect_h
#define GC_GarminConnect_h

#include "CloudService.h"

#include <QImage>

// Q_OBJECT and signals are added in REQ-005+ when the credentials dialog and
// download path require asynchronous notifications. REQ-001 only needs the
// tile contract (id/uiName/description/logo/type/capabilities/clone).
class GarminConnect : public CloudService
{
  public:
    GarminConnect(Context* context);
    ~GarminConnect();

    CloudService* clone(Context* context) { return new GarminConnect(context); }

    QString id() const { return QStringLiteral("Garmin Connect"); }
    QString uiName() const { return tr("Garmin Connect"); }
    QString description() const { return tr("Download activities from Garmin Connect."); }
    QImage logo() const;

    // Read-only: Query advertises the listing capability used by sync;
    // Download covers per-activity fetch. No Upload, no OAuth — credentials
    // are username/password/MFA per DEC-005.
    int capabilities() const { return Query | Download; }
    int type() const { return Activities; }
};

#endif // GC_GarminConnect_h
