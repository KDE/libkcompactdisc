/*
 *  KCompactDisc - A CD drive interface for the KDE Project.
 *
 *  Copyright (C) 2007 Alexander Kern <alex.kern@gmx.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef WMLIB_INTERFACE_H
#define WMLIB_INTERFACE_H

#include "kcompactdisc_p.h"

class KWMLibCompactDiscPrivate : public KCompactDiscPrivate
{
    Q_OBJECT

	public:
		KWMLibCompactDiscPrivate(KCompactDisc *, const QString&, const QString &, const QString&);
		~KWMLibCompactDiscPrivate() override;

		bool createInterface() override;

		unsigned trackLength(unsigned) override;
		bool isTrackAudio(unsigned) override;
		void playTrackPosition(unsigned, unsigned) override;
		void pause() override;
		void stop() override;
		void eject() override;
		void closetray() override;
	
		void setVolume(unsigned) override;
		void setBalance(unsigned) override;
		unsigned volume() override;
		unsigned balance() override;
	
		void queryMetadata() override;


	private:
		KCompactDisc::DiscStatus discStatusTranslate(int);
		void *m_handle;
		QString m_audioSystem;
		QString m_audioDevice;

	
	private Q_SLOTS:
		void timerExpired();
		void cdtext();
};

#endif // WMLIB_INTERFACE_H
