/*
	Skype Call Recorder
	Copyright 2008-2010, 2013, 2015 by jlh (jlh at gmx dot ch)

	This program is free software; you can redistribute it and/or modify it
	under the terms of the GNU General Public License as published by the
	Free Software Foundation; either version 2 of the License, version 3 of
	the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful, but
	WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	General Public License for more details.

	You should have received a copy of the GNU General Public License along
	with this program; if not, write to the Free Software Foundation, Inc.,
	51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

	The GNU General Public License version 2 is included with the source of
	this program under the file name COPYING.  You can also get a copy on
	http://www.fsf.org/
*/

#ifndef SKYPE_H
#define SKYPE_H

#include <QObject>
#include <QString>

#include "common.h"

class Skype : public QObject {
	Q_OBJECT
public:
	Skype(QObject *);
	virtual QString sendWithReply(const QString &, int = 10000) = 0;
	virtual void send(const QString &) = 0;
	QString getObject(const QString &);
	const QString &getSkypeName() const { return skypeName; }

signals:
	void notify(const QString &) const;
	void connected(bool) const;
	void connectionFailed(const QString &) const;

protected:
	virtual void sendWithAsyncReply(const QString &) = 0;
	void doNotify(const QString &);

protected:
	int connectionState;
	QString skypeName;

	DISABLE_COPY_AND_ASSIGNMENT(Skype);
};

#endif

