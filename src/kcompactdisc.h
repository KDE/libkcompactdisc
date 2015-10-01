/*
 *  Copyright (C) 2015 Boudhayan Gupta <bgupta@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#ifndef KCOMPACTDISC_H
#define KCOMPACTDISC_H

#include <QList>

#include <Solid/Block>
#include <Solid/Device>
#include <Solid/OpticalDrive>

#include "kcompactdisc_export.h"

namespace KCompactDisc {

KCOMPACTDISC_EXPORT QList<Solid::OpticalDrive *> allOpticalDrives();
KCOMPACTDISC_EXPORT QList<QUrl> allOpticalDriveNodes();

KCOMPACTDISC_EXPORT Solid::OpticalDrive *defaultOpticalDrive();
KCOMPACTDISC_EXPORT QUrl defaultOpticalDriveNode();

} // namespace KCompactDisc

#endif // KCOMPACTDISC_H