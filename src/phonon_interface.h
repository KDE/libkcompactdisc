/*
 *
 * Copyright (C) 2004-2007 Matthias Kretz <kretz@kde.org>
 * Copyright (C) by Alexander Kern <alex.kern@gmx.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 * CDDA version taken from guitest in phonon test directory
 */

#ifndef PHONON_INTERFACE_H
#define PHONON_INTERFACE_H

#include "kcompactdisc_p.h"
#include <phonon/phononnamespace.h>

class ProducerWidget;

class KPhononCompactDiscPrivate : public KCompactDiscPrivate
{
	Q_OBJECT

	public:
		KPhononCompactDiscPrivate(KCompactDisc *, const QString &);
		~KPhononCompactDiscPrivate() override;

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
		ProducerWidget *m_producerWidget;
		ProducerWidget *producer();
		QString m_udi;

		KCompactDisc::DiscStatus discStatusTranslate(Phonon::State);

	public Q_SLOTS:
		void tick(qint64);
		void stateChanged(Phonon::State, Phonon::State);
};

#endif // PHONON_INTERFACE_H
