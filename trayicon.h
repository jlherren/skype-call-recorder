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

#ifndef TRAYICON_H
#define TRAYICON_H

#include <QSystemTrayIcon>

class QAction;

class TrayIcon : public QSystemTrayIcon {
	Q_OBJECT
public:
	TrayIcon(QObject *);

signals:
	void requestQuit();
	void requestQuitNoConfirmation();
	void requestAbout();
	void requestOpenPreferences();
	void requestBrowseCalls();

signals:
	void startRecording();
	void stopRecordingAndDelete();
	void stopRecording();

public slots:
	void startedCall(const QString &);
	void stoppedCall();
	void startedRecording();
	void stoppedRecording();

private:
	QAction *currentCallAction;
	QAction *startAction;
	QAction *stopAction;
	QAction *stopAndDeleteAction;

private slots:
	void activate(QSystemTrayIcon::ActivationReason);

private:
	// disabled
	TrayIcon(const TrayIcon &);
	TrayIcon &operator=(const TrayIcon &);
};

#endif

