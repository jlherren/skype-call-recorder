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

#ifndef RECORDER_H
#define RECORDER_H

#include <QApplication>
#include <QStringList>
#include <QPointer>

#include "utils.h"

class TrayIcon;
class QTextEdit;
class QString;
class PreferencesDialog;
class Skype;
class CallHandler;
class AboutDialog;

class Recorder : public QApplication {
	Q_OBJECT
public:
	Recorder(int, char **);
	virtual ~Recorder();

	void debugMessage(const QString &);

public slots:
	void about();
	void openPreferences();
	void closePreferences();
	void browseCalls();
	void quitConfirmation();
	void skypeNotify(const QString &);
	void skypeConnected(bool);
	void skypeConnectionFailed(const QString &);
	void savePreferences();

private:
	void loadPreferences();
	void setupGUI();
	void setupSkype();
	void setupCallHandler();

	QString getConfigFile() const;

private:
	Skype *skype;
	CallHandler *callHandler;
	PreferencesDialog *preferencesDialog;
	TrayIcon *trayIcon;
	QPointer<AboutDialog> aboutDialog;
	LockFile lockFile;

private:
	// disabled
	Recorder(const Recorder &);
	Recorder &operator=(const Recorder &);
};

#endif

