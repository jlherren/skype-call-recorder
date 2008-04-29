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

#include <QList>
#include <QVariant>
#include <QTimer>

#include "skype.h"
#include "common.h"

namespace {
const QString skypeServiceName("com.Skype.API");
const QString skypeInterfaceName("com.Skype.API");
}

Skype::Skype() : dbus("SkypeRecorder"), connectionState(0) {
	dbus = QDBusConnection::connectToBus(QDBusConnection::SessionBus, "SkypeRecorder");

	connect(dbus.interface(), SIGNAL(serviceOwnerChanged(const QString &, const QString &, const QString &)),
		this, SLOT(serviceOwnerChanged(const QString &, const QString &, const QString &)));

	// export our object
	exported = new SkypeExport(this);
	if (!dbus.registerObject("/com/Skype/Client", this))
		debug("Cannot register object /com/Skype/Client");

	QTimer::singleShot(0, this, SLOT(connectToSkype()));
}

void Skype::connectToSkype() {
	if (connectionState)
		return;

	QDBusReply<bool> exists = dbus.interface()->isServiceRegistered(skypeServiceName);

	if (!exists.isValid() || !exists.value()) {
		debug(QString("Service %1 not found on DBus").arg(skypeServiceName));
		emit skypeNotFound();
		return;
	}

	sendWithAsyncReply("NAME SkypeRecorder");
	connectionState = 1;
}

void Skype::serviceOwnerChanged(const QString &name, const QString &oldOwner, const QString &newOwner) {
	if (name != skypeServiceName)
		return;

	if (oldOwner.isEmpty()) {
		debug(QString("DBUS: Skype API service appeared as %1").arg(newOwner));
		if (connectionState != 3)
			connectToSkype();
	} else if (newOwner.isEmpty()) {
		debug("DBUS: Skype API service disappeared");
		if (connectionState == 3)
			emit connectionLost();
		connectionState = 0;
	}
}

void Skype::sendWithAsyncReply(const QString &s) {
	debug(QString("SKYPE --> %1 (async reply)").arg(s));

	QDBusMessage msg = QDBusMessage::createMethodCall(skypeServiceName, "/com/Skype", skypeInterfaceName, "Invoke");
	QList<QVariant> args;
	args.append(s);
	msg.setArguments(args);

	dbus.callWithCallback(msg, this, SLOT(methodCallback(const QDBusMessage &)), SLOT(methodError(const QDBusError &, const QDBusMessage &)), 3600000);
}

QString Skype::sendWithReply(const QString &s) {
	debug(QString("SKYPE --> %1 (sync reply)").arg(s));

	QDBusMessage msg = QDBusMessage::createMethodCall(skypeServiceName, "/com/Skype", skypeInterfaceName, "Invoke");
	QList<QVariant> args;
	args.append(s);
	msg.setArguments(args);

	msg = dbus.call(msg, QDBus::Block, 10000);

	if (msg.type() != QDBusMessage::ReplyMessage) {
		debug(QString("SKYPE <R- (failed)"));
		return QString();
	}

	QString ret = msg.arguments().value(0).toString();
	debug(QString("SKYPE <R- %1").arg(ret));
	return ret;
}

void Skype::send(const QString &s) {
	debug(QString("SKYPE --> %1 (no reply)").arg(s));

	QDBusMessage msg = QDBusMessage::createMethodCall(skypeServiceName, "/com/Skype", skypeInterfaceName, "Invoke");
	QList<QVariant> args;
	args.append(s);
	msg.setArguments(args);

	dbus.call(msg, QDBus::NoBlock);
}

QString Skype::getObject(const QString &object) {
	QString ret = sendWithReply("GET " + object);
	if (!ret.startsWith(object))
		return QString();
	return ret.mid(object.size() + 1);
}

void Skype::methodCallback(const QDBusMessage &msg) {
	if (msg.type() != QDBusMessage::ReplyMessage) {
		connectionState = 0;
		emit connectionFailed("Cannot communicate with Skype");
		return;
	}

	QString s = msg.arguments().value(0).toString();
	debug(QString("SKYPE <R- %1").arg(s));

	if (connectionState == 1) {
		if (s == "OK") {
			connectionState = 2;
			sendWithAsyncReply("PROTOCOL 5");
		} else {
			connectionState = 0;
			emit connectionFailed("Skype denied access");
		}
	} else if (connectionState == 2) {
		if (s == "PROTOCOL 5") {
			connectionState = 3;
			emit connected();
		} else {
			connectionState = 0;
			emit connectionFailed("Skype handshake error");
		}
	}
}

void Skype::methodError(const QDBusError &error, const QDBusMessage &) {
	connectionState = 0;
	emit connectionFailed(error.message());
}

void Skype::doNotify(const QString &s) const {
	debug(QString("SKYPE <-- %1").arg(s));
	emit notify(s);
}

// ---- SkypeExport ----

SkypeExport::SkypeExport(Skype *p) : QDBusAbstractAdaptor(p), parent(p) {
}

void SkypeExport::Notify(const QString &s) {
	parent->doNotify(s);
}

