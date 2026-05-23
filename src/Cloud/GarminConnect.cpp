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

#include "GarminConnect.h"

#include <QColor>

GarminConnect::GarminConnect(Context* c) : CloudService(c) {}

GarminConnect::~GarminConnect() {}

QImage GarminConnect::logo() const
{
    // Production builds link the application qrc and resolve the branded PNG.
    // Headless / unit-test builds do not load qrc resources, so fall back to
    // an in-code placeholder. Either way the contract — "logo() is non-null
    // and has positive dimensions" — holds.
    QImage img(QStringLiteral(":images/services/garminconnect.png"));
    if (!img.isNull())
        return img;

    QImage fallback(64, 64, QImage::Format_ARGB32);
    fallback.fill(QColor(0, 122, 195)); // Garmin brand blue
    return fallback;
}

// Static-init registration, mirroring the precedent in Selfloops.cpp /
// Strava.cpp. Runs before main(); the factory rejects duplicates so this is
// safe even if the translation unit were somehow linked twice.
static bool addGarminConnect()
{
    CloudServiceFactory::instance().addService(new GarminConnect(NULL));
    return true;
}

static bool addedGarminConnect = addGarminConnect();
