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

#include "kcompactdisc.h"
#include <QUrl>

QList<Solid::OpticalDrive *> KCompactDisc::allOpticalDrives()
{
    QList<Solid::OpticalDrive *> driveList;

    for(auto drive: Solid::Device::listFromType(Solid::DeviceInterface::OpticalDrive)) {
        Solid::Block *blockDev = drive.as<Solid::Block>();
        if (!blockDev) {
            continue;
        }
        driveList.append(drive.as<Solid::OpticalDrive>());
    }
    return driveList;
}

QList<QUrl> KCompactDisc::allOpticalDriveNodes()
{
    QList<QUrl> nodeList;

    for(auto drive: Solid::Device::listFromType(Solid::DeviceInterface::OpticalDrive)) {
        Solid::Block *blockDev = drive.as<Solid::Block>();
        if (!blockDev) {
            continue;
        }
        nodeList.append(QUrl::fromUserInput(blockDev->device()));
    }
    return nodeList;
}

Solid::OpticalDrive *KCompactDisc::defaultOpticalDrive()
{
    return allOpticalDrives().first();
}

QUrl KCompactDisc::defaultOpticalDriveNode()
{
    return allOpticalDriveNodes().first();
}
