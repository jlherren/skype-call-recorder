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

#ifndef SKYPE_DBUS_H
#define SKYPE_DBUS_H

#include <QObject>
#include <QDBusConnection>
#include <QDBusAbstractAdaptor>
#include <QString>

#include "common.h"
#include "skype.h"

class SkypeExport;
class QTimer;
class QDBusError;
class QDBusMessage;

class SkypeDBus : public Skype {
	Q_OBJECT
public:
	friend class SkypeExport;

	SkypeDBus(QObject *);
	virtual QString sendWithReply(const QString &, int = 10000);
	virtual void send(const QString &);

protected slots:
	void connectToSkype();
	void methodCallback(const QDBusMessage &);
	void methodError(const QDBusError &, const QDBusMessage &);
	void serviceOwnerChanged(const QString &, const QString &, const QString &);
	void poll();

protected:
	virtual void sendWithAsyncReply(const QString &);

protected:
	QDBusConnection dbus;
	SkypeExport *exported;
	QTimer *timer;

	DISABLE_COPY_AND_ASSIGNMENT(SkypeDBus);
};

class SkypeExport : public QDBusAbstractAdaptor {
	Q_OBJECT
	Q_CLASSINFO("D-Bus Interface", "com.Skype.API.Client")
public:
	SkypeExport(SkypeDBus *);

public slots:
	Q_NOREPLY void Notify(const QString &);

private:
	SkypeDBus *parent;

	DISABLE_COPY_AND_ASSIGNMENT(SkypeExport);
};

#endif

