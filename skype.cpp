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

#include <QList>
#include <QVariant>
#include <QTimer>
#include <QMessageBox>

#include "skype.h"
#include "common.h"

Skype::Skype(QObject *parent) : QObject(parent), connectionState(0) {
}

QString Skype::getObject(const QString &object) {
	QString ret = sendWithReply("GET " + object);
	if (!ret.startsWith(object))
		return QString();
	return ret.mid(object.size() + 1);
}

void Skype::doNotify(const QString &s) {
	if (connectionState != 3)
		return;

	debug(QString("SKYPE <-- %1").arg(s));

	if (s.startsWith("CURRENTUSERHANDLE "))
		skypeName = s.mid(18);

	emit notify(s);
}

