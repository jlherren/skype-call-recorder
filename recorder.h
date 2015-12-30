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

#ifndef RECORDER_H
#define RECORDER_H

#include <QApplication>
#include <QPointer>
#include <QString>

#include "common.h"
#include "utils.h"

class TrayIcon;
class PreferencesDialog;
class Skype;
class CallHandler;
class AboutDialog;

class Recorder : public QApplication {
	Q_OBJECT
public:
	Recorder(int &, char **);
	virtual ~Recorder();

	void debugMessage(const QString &);

public slots:
	void about();
	void openWebsite();
	void openPreferences();
	void closePerCallerDialog();
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
	void sanatizePreferences();
	bool convertSettingsToV2();
	bool sanatizePreferencesGeneric();

	QString getConfigFile() const;

private:
	QPointer<Skype> skype;
	QPointer<CallHandler> callHandler;
	QPointer<PreferencesDialog> preferencesDialog;
	QPointer<TrayIcon> trayIcon;
	QPointer<AboutDialog> aboutDialog;
	LockFile lockFile;

	DISABLE_COPY_AND_ASSIGNMENT(Recorder);
};

#endif

