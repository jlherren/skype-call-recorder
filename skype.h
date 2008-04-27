/*
	Skype Call Recorder
	Copyright (C) 2008 jlh (jlh at gmx dot ch)

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
#include <QtDBus>
#include <QString>

class SkypeExport;

class Skype : public QObject {
	Q_OBJECT
public:
	friend class SkypeExport;

	Skype();
	QString sendWithReply(const QString &);
	void send(const QString &);
	QString getObject(const QString &);

signals:
	void notify(const QString &) const;
	void connected() const;
	void connectionFailed(const QString &) const;
	void skypeNotFound() const;

private:
	void sendWithAsyncReply(const QString &);
	void doNotify(const QString &) const;

private slots:
	void connectToSkype();
	void methodCallback(const QDBusMessage &);
	void methodError(const QDBusError &, const QDBusMessage &);

private:
	QDBusConnection dbus;
	int connectionState;
	SkypeExport *exported;

private:
	// disabled
	Skype(const Skype &);
	Skype &operator=(const Skype &);
};

class SkypeExport : public QDBusAbstractAdaptor {
	Q_OBJECT
	Q_CLASSINFO("D-Bus Interface", "com.Skype.API.Client")
public:
	SkypeExport(Skype *);

public slots:
	Q_NOREPLY void Notify(const QString &);

private:
	Skype *parent;

private:
	// disabled
	SkypeExport(const SkypeExport &);
	SkypeExport &operator=(const SkypeExport &);
};

#endif

