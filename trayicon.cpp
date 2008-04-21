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
#include "recorder.h"
#include "common.h"

TrayIcon::TrayIcon(Recorder *r) : QSystemTrayIcon(r), recorder(r) {
	if (!QSystemTrayIcon::isSystemTrayAvailable()) {
		QMessageBox::critical(NULL, PROGRAM_NAME " - Error",
			PROGRAM_NAME " cannot start, because it requires a system tray.  None was detected.  "
			"(TODO: Make this work even without a system tray.)");
		recorder->quit();
		return;
	}

	setIcon(QIcon(":/icon.png"));

	menu = new QMenu;
	QAction *action = menu->addAction("About " PROGRAM_NAME);
	action->setEnabled(false);
	connect(action, SIGNAL(triggered()), recorder, SLOT(about()));
	action = menu->addAction("Open preferences...");
	connect(action, SIGNAL(triggered()), recorder, SLOT(openSettings()));
	action = menu->addAction("Browse previous calls");
	action->setEnabled(false);
	connect(action, SIGNAL(triggered()), recorder, SLOT(browseCalls()));
	action = menu->addAction("Exit");
	connect(action, SIGNAL(triggered()), recorder, SLOT(quitConfirmation()));
	setContextMenu(menu);

	setToolTip(PROGRAM_NAME);

	connect(this, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(activate(QSystemTrayIcon::ActivationReason)));

	show();
}

void TrayIcon::activate(QSystemTrayIcon::ActivationReason) {
	contextMenu()->popup(QCursor::pos());
}

