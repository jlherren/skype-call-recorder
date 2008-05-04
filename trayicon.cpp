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

#include "trayicon.h"
#include "skype.h"
#include "common.h"

TrayIcon::TrayIcon(QObject *p) : QSystemTrayIcon(p) {
	if (!QSystemTrayIcon::isSystemTrayAvailable()) {
		QMessageBox::critical(NULL, PROGRAM_NAME " - Error",
			PROGRAM_NAME " cannot start, because it requires a system tray.  None was detected.  "
			"(TODO: Make this work even without a system tray.)");
		emit requestQuitNoConfirmation();
		return;
	}

	setIcon(QIcon(":/icon.png"));

	// current call submenu
	QMenu *subMenu = new QMenu("Call with unknown");
	startAction = subMenu->addAction("&Start recording", this, SIGNAL(startRecording()));
	stopAction = subMenu->addAction("S&top recording", this, SIGNAL(stopRecording()));
	stopAndDeleteAction = subMenu->addAction("Stop recording and &delete file", this, SIGNAL(stopRecordingAndDelete()));

	QMenu *menu = new QMenu;

	menu->addAction("&About " PROGRAM_NAME, this, SIGNAL(requestAbout()));
	currentCallAction = menu->addMenu(subMenu);
	currentCallAction->setVisible(false);
	menu->addAction("Open &preferences...", this, SIGNAL(requestOpenPreferences()));
	menu->addAction("&Browse previous calls", this, SIGNAL(requestBrowseCalls()));
	menu->addSeparator();
	menu->addAction("&Exit", this, SIGNAL(requestQuit()));

	setContextMenu(menu);
	setToolTip(PROGRAM_NAME);

	connect(this, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(activate(QSystemTrayIcon::ActivationReason)));

	show();
}

void TrayIcon::activate(QSystemTrayIcon::ActivationReason) {
	contextMenu()->popup(QCursor::pos());
}

void TrayIcon::startedCall(const QString &skypeName) {
	currentCallAction->setText("Call with " + skypeName);
	currentCallAction->setVisible(true);
	startAction->setEnabled(true);
	stopAction->setEnabled(false);
	stopAndDeleteAction->setEnabled(false);
}

void TrayIcon::stoppedCall() {
	currentCallAction->setVisible(false);
}

void TrayIcon::startedRecording() {
	startAction->setEnabled(false);
	stopAction->setEnabled(true);
	stopAndDeleteAction->setEnabled(true);
}

void TrayIcon::stoppedRecording() {
	startAction->setEnabled(true);
	stopAction->setEnabled(false);
	stopAndDeleteAction->setEnabled(false);
}

