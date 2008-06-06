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

#include <QSystemTrayIcon>
#include <QMessageBox>
#include <QMenu>
#include <QCursor>
#include <QSignalMapper>

#include "trayicon.h"
#include "common.h"
#include "skype.h"
#include "preferences.h"

TrayIcon::TrayIcon(QObject *p) : QSystemTrayIcon(p) {
	if (!QSystemTrayIcon::isSystemTrayAvailable()) {
		QMessageBox::critical(NULL, PROGRAM_NAME " - Error",
			PROGRAM_NAME " cannot start, because it requires a system tray.  None was detected.  "
			"(TODO: Make this work even without a system tray.)");
		emit requestQuitNoConfirmation();
		return;
	}

	smStart = new QSignalMapper(this);
	smStop = new QSignalMapper(this);
	smStopAndDelete = new QSignalMapper(this);

	connect(smStart, SIGNAL(mapped(int)), this, SIGNAL(startRecording(int)));
	connect(smStop, SIGNAL(mapped(int)), this, SIGNAL(stopRecording(int)));
	connect(smStopAndDelete, SIGNAL(mapped(int)), this, SIGNAL(stopRecordingAndDelete(int)));

	setColor(false);

	menu = new QMenu;
	separator = menu->addSeparator();
	menu->addAction("&Browse previous calls", this, SIGNAL(requestBrowseCalls()));
	menu->addAction("Open &preferences...", this, SIGNAL(requestOpenPreferences()));
	menu->addAction("&About " PROGRAM_NAME, this, SIGNAL(requestAbout()));
	menu->addSeparator();
	menu->addAction("&Exit", this, SIGNAL(requestQuit()));

	setContextMenu(menu);
	updateToolTip();

	connect(this, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(activate(QSystemTrayIcon::ActivationReason)));

	show();
}

TrayIcon::~TrayIcon() {
	for (CallMap::const_iterator i = callMap.constBegin(); i != callMap.constEnd(); ++i)
		delete i.value().menu;

	delete menu;
}

void TrayIcon::setColor(bool color) {
	setIcon(QIcon(color ? ":/icon.png" : ":/icongray.png"));
}


void TrayIcon::activate(QSystemTrayIcon::ActivationReason) {
	contextMenu()->popup(QCursor::pos());
}

void TrayIcon::startedCall(int id, const QString &skypeName) {
	CallData &data = callMap[id];

	data.skypeName = skypeName;
	data.isRecording = false;
	data.menu = new QMenu(QString("Call with ") + skypeName);
	data.startAction = data.menu->addAction("&Start recording", smStart, SLOT(map()));
	data.stopAction = data.menu->addAction("S&top recording", smStop, SLOT(map()));
	data.stopAndDeleteAction = data.menu->addAction("Stop recording and &delete file", smStopAndDelete, SLOT(map()));

	data.startAction->setEnabled(true);
	data.stopAction->setEnabled(false);
	data.stopAndDeleteAction->setEnabled(false);

	smStart->setMapping(data.startAction, id);
	smStop->setMapping(data.stopAction, id);
	smStopAndDelete->setMapping(data.stopAndDeleteAction, id);

	menu->insertMenu(separator, data.menu);

	updateToolTip();
}

void TrayIcon::stoppedCall(int id) {
	if (!callMap.contains(id))
		return;
	CallData &data = callMap[id];
	delete data.menu;
	// deleting the menu deletes the actions, which automatically removes
	// the signal mappings
	callMap.remove(id);

	updateToolTip();
}

void TrayIcon::startedRecording(int id) {
	if (!callMap.contains(id))
		return;
	CallData &data = callMap[id];
	data.isRecording = true;
	data.startAction->setEnabled(false);
	data.stopAction->setEnabled(true);
	data.stopAndDeleteAction->setEnabled(true);

	if (supportsMessages() && preferences.get(Pref::NotifyRecordingStart).toBool()) {
		showMessage("Recording started",
			QString("The call with %1 is now being recorded.").arg(data.skypeName),
			Information, 5000);
	}

	updateToolTip();
}

void TrayIcon::stoppedRecording(int id) {
	if (!callMap.contains(id))
		return;
	CallData &data = callMap[id];
	data.isRecording = false;
	data.startAction->setEnabled(true);
	data.stopAction->setEnabled(false);
	data.stopAndDeleteAction->setEnabled(false);

	updateToolTip();
}

void TrayIcon::updateToolTip() {
	QString str = PROGRAM_NAME;

	if (!callMap.isEmpty()) {
		for (CallMap::const_iterator i = callMap.constBegin(); i != callMap.constEnd(); ++i) {
			const CallData &data = i.value();
			str += QString(data.isRecording ?
				"\nThe call with '%1' is being recorded" :
				"\nThe call with '%1' is not being recorded").arg(data.skypeName);
		}
	}

	setToolTip(str);
}

