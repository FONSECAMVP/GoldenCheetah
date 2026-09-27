/*
 * Copyright (c) 2026 GoldenCheetah Contributor
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 */

#ifndef GC_GarminTime_h
#define GC_GarminTime_h

// DEC-084 — the ONE way any Garmin `startTimeGMT` comparison site turns a
// string into an instant. Header-only (option A): no new translation unit,
// so it cannot go missing from one of the two build systems the way a new
// .cpp could (garmin-build-system-duality).

#include <QDateTime>
#include <QString>

// Parses `yyyy-MM-dd HH:mm:ss` first, then falls back to Qt::ISODate. A
// zone-less spelling is startTimeGMT's own UTC wall clock and is DECLARED
// UTC (setTimeSpec) rather than converted; a spelling carrying `Z` or an
// explicit offset already names an instant and is CONVERTED (toUTC()) — a
// blanket choice of either operation mis-reads the other spelling (DEC-084's
// measured Qt 6.8.2 table). Returns an invalid QDateTime on an unparseable
// input; every caller must treat invalid as "do not act", never as a zero
// instant.
inline QDateTime garminInstantFromString(const QString& s)
{
    QDateTime dt = QDateTime::fromString(s, QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    if (!dt.isValid())
        dt = QDateTime::fromString(s, Qt::ISODate);
    if (!dt.isValid())
        return dt;
    if (dt.timeSpec() == Qt::LocalTime)
        dt.setTimeSpec(Qt::UTC);
    else
        dt = dt.toUTC();
    return dt;
}

#endif // GC_GarminTime_h
