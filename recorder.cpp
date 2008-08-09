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

#include <QMessageBox>
#include <QDir>
#include <QProcess>
#include <QTimer>
#include <QDesktopServices>
#include <QUrl>
#include <iostream>
#include <cstdlib>

#include "recorder.h"
#include "common.h"
#include "gui.h"
#include "trayicon.h"
#include "preferences.h"
#include "skype.h"
#include "call.h"

Recorder::Recorder(int &argc, char **argv) :
	QApplication(argc, argv)
{
	recorderInstance = this;

	debug("Initializing application");

	// check for already running instance
	if (!lockFile.lock(QDir::homePath() + "/.skypecallrecorder.lock")) {
		debug("Other instance is running");
		QTimer::singleShot(0, this, SLOT(quit()));
		return;
	}

	loadPreferences();

	setupGUI();
	setupSkype();
	setupCallHandler();
}

Recorder::~Recorder() {
	// the Recorder object must still exist while the child objects are
	// being destroyed, because debug() must still be available.  Qt would
	// do it automatically but only after Recorder ceased to exist, in the
	// QObject destructor

	delete preferencesDialog;
	delete callHandler;
	delete skype;
	delete trayIcon;
}

void Recorder::setupGUI() {
	setWindowIcon(QIcon(":/icon.png"));
	setQuitOnLastWindowClosed(false);

	trayIcon = new TrayIcon(this);
	connect(trayIcon, SIGNAL(requestQuit()),               this, SLOT(quitConfirmation()));
	connect(trayIcon, SIGNAL(requestAbout()),              this, SLOT(about()));
	connect(trayIcon, SIGNAL(requestWebsite()),            this, SLOT(openWebsite()));
	connect(trayIcon, SIGNAL(requestOpenPreferences()),    this, SLOT(openPreferences()));
	connect(trayIcon, SIGNAL(requestBrowseCalls()),        this, SLOT(browseCalls()));

	debug("GUI initialized");

	if (!preferences.get(Pref::SuppressFirstRunInformation).toBool())
		new FirstRunDialog();
}

void Recorder::setupSkype() {
	skype = new Skype(this);
	connect(skype, SIGNAL(notify(const QString &)),           this, SLOT(skypeNotify(const QString &)));
	connect(skype, SIGNAL(connected(bool)),                   this, SLOT(skypeConnected(bool)));
	connect(skype, SIGNAL(connectionFailed(const QString &)), this, SLOT(skypeConnectionFailed(const QString &)));

	connect(skype, SIGNAL(connected(bool)), trayIcon, SLOT(setColor(bool)));
}

void Recorder::setupCallHandler() {
	callHandler = new CallHandler(this, skype);

	connect(trayIcon, SIGNAL(startRecording(int)),         callHandler, SLOT(startRecording(int)));
	connect(trayIcon, SIGNAL(stopRecording(int)),          callHandler, SLOT(stopRecording(int)));
	connect(trayIcon, SIGNAL(stopRecordingAndDelete(int)), callHandler, SLOT(stopRecordingAndDelete(int)));

	connect(callHandler, SIGNAL(startedCall(int, const QString &)), trayIcon, SLOT(startedCall(int, const QString &)));
	connect(callHandler, SIGNAL(stoppedCall(int)),                  trayIcon, SLOT(stoppedCall(int)));
	connect(callHandler, SIGNAL(startedRecording(int)),             trayIcon, SLOT(startedRecording(int)));
	connect(callHandler, SIGNAL(stoppedRecording(int)),             trayIcon, SLOT(stoppedRecording(int)));
}

QString Recorder::getConfigFile() const {
	return QDir::homePath() + "/.skypecallrecorder.rc";
}

void Recorder::loadPreferences() {
	preferences.load(getConfigFile());
	int c = preferences.count();

	// since Pref::PreferencesVersion did not exist from the first version
	// on, some people might not have it; but we cannot just let it receive
	// the default value, as those old settings must be updated.  If it is
	// missing but Pref::OutputPath exists (which has always been around),
	// then set Pref::PreferencesVersion to 1
	if (!preferences.exists(Pref::PreferencesVersion) && preferences.exists(Pref::OutputPath))
		preferences.get(Pref::PreferencesVersion).set(1);

	#define X(n, v) preferences.get(n).setIfNotSet(v);
	// default preferences
	X(Pref::AutoRecordDefault,           "ask");         // "yes", "ask", "no"
	X(Pref::AutoRecordAsk,               "");            // comma separated skypenames to always ask for
	X(Pref::AutoRecordYes,               "");            // comma separated skypenames to always record
	X(Pref::AutoRecordNo,                "");            // comma separated skypenames to never record
	X(Pref::OutputPath,                  "~/Skype Calls");
	X(Pref::OutputPattern,               "Calls with &s/Call with &s, %a %b %d %Y, %H:%M:%S");
	X(Pref::OutputFormat,                "mp3");         // "mp3", "vorbis" or "wav"
	X(Pref::OutputFormatMp3Bitrate,      64);
	X(Pref::OutputFormatVorbisQuality,   3);
	X(Pref::OutputStereo,                true);
	X(Pref::OutputStereoMix,             0);             // 0 .. 100
	X(Pref::OutputSaveTags,              true);
	X(Pref::SuppressLegalInformation,    false);
	X(Pref::SuppressFirstRunInformation, false);
	X(Pref::PreferencesVersion,          2);
	X(Pref::NotifyRecordingStart,        true)
	X(Pref::GuiWindowed,                 false)
	X(Pref::DebugWriteSyncFile,          false)
	#undef X

	c = preferences.count() - c;

	if (c)
		debug(QString("Loading %1 built-in default preference(s)").arg(c));

	sanatizePreferences();
}

void Recorder::savePreferences() {
	preferences.save(getConfigFile());
	// TODO: when failure?
}

void Recorder::sanatizePreferences() {
	// this converts old preferences to new preferences

	int v = preferences.get(Pref::PreferencesVersion).toInt();
	bool didSomething = false;

	switch (v) {
		case 1:
			didSomething |= convertSettingsToV2();
	}

	didSomething |= sanatizePreferencesGeneric();

	if (didSomething)
		savePreferences();
}

bool Recorder::convertSettingsToV2() {
	debug("Converting settings from v1 to v2");

	QString s = preferences.get("output.channelmode").toString();
	preferences.remove("output.channelmode");

	if (s == "stereo") {
		preferences.get(Pref::OutputStereo).set(true);
		preferences.get(Pref::OutputStereoMix).set(0);
	} else if (s == "oerets") {
		preferences.get(Pref::OutputStereo).set(true);
		preferences.get(Pref::OutputStereoMix).set(100);
	} else if (s == "mono") {
		preferences.get(Pref::OutputStereo).set(false);
		preferences.get(Pref::OutputStereoMix).set(0);
	}

	preferences.get(Pref::PreferencesVersion).set(2);

	return true;
}

bool Recorder::sanatizePreferencesGeneric() {
	QString s;
	int i;
	bool didSomething = false;

	s = preferences.get(Pref::AutoRecordDefault).toString();
	if (s != "ask" && s != "yes" && s != "no") {
		preferences.get(Pref::AutoRecordDefault).set("ask");
		didSomething = true;
	}

	s = preferences.get(Pref::OutputFormat).toString();
	if (s != "mp3" && s != "vorbis" && s != "wav") {
		preferences.get(Pref::OutputFormat).set("mp3");
		didSomething = true;
	}

	i = preferences.get(Pref::OutputFormatMp3Bitrate).toInt();
	if (i < 8 || (i < 64 && i % 8 != 0) || (i < 160 && i % 16 != 0) || i > 160) {
		preferences.get(Pref::OutputFormatMp3Bitrate).set(64);
		didSomething = true;
	}

	i = preferences.get(Pref::OutputFormatVorbisQuality).toInt();
	if (i < -1 || i > 10) {
		preferences.get(Pref::OutputFormatVorbisQuality).set(3);
		didSomething = true;
	}

	i = preferences.get(Pref::OutputStereoMix).toInt();
	if (i < 0 || i > 100) {
		preferences.get(Pref::OutputStereoMix).set(0);
		didSomething = true;
	}

	s = preferences.get(Pref::OutputPath).toString();
	if (s.trimmed().isEmpty()) {
		preferences.get(Pref::OutputPath).set("~/Skype Calls");
		didSomething = true;
	}

	s = preferences.get(Pref::OutputPattern).toString();
	if (s.trimmed().isEmpty()) {
		preferences.get(Pref::OutputPattern).set("Calls with &s/Call with &s, %a %b %d %Y, %H:%M:%S");
		didSomething = true;
	}

	if (didSomething)
		debug("At least one preference has been reset to its default value, because it contained bogus data.");

	return didSomething;
}

void Recorder::about() {
	if (!aboutDialog)
		aboutDialog = new AboutDialog;

	aboutDialog->raise();
	aboutDialog->activateWindow();
}

void Recorder::openWebsite() {
	bool ret = QDesktopServices::openUrl(QUrl::fromEncoded(websiteURL));

	if (!ret)
		QMessageBox::information(NULL, PROGRAM_NAME,
			QString("Failed to open URL %1").arg(websiteURL));
}

void Recorder::openPreferences() {
	debug("Show preferences dialog");

	if (!preferencesDialog) {
		preferencesDialog = new PreferencesDialog();
		connect(preferencesDialog, SIGNAL(finished(int)), this, SLOT(savePreferences()));
	}

	preferencesDialog->raise();
	preferencesDialog->activateWindow();
}

void Recorder::closePerCallerDialog() {
	debug("Hide per-caller dialog");
	if (preferencesDialog)
		preferencesDialog->closePerCallerDialog();
}

void Recorder::browseCalls() {
	QString path = getOutputPath();
	QDir().mkpath(path);
	QUrl url = QUrl(QString("file://") + path);
	bool ret = QDesktopServices::openUrl(url);

	if (!ret)
		QMessageBox::information(NULL, PROGRAM_NAME,
			QString("Failed to open URL %1").arg(QString(url.toEncoded())));
}

void Recorder::quitConfirmation() {
	debug("Request to quit");
	savePreferences();
	quit();
}

void Recorder::skypeNotify(const QString &s) {
	QStringList args = s.split(' ');
	QString cmd = args.takeFirst();
	if (cmd == "CALL")
		callHandler->callCmd(args);
}

void Recorder::skypeConnected(bool conn) {
	if (conn)
		debug("skype connection established");
	else
		debug("skype not connected");
}

void Recorder::skypeConnectionFailed(const QString &reason) {
	debug("skype connection failed, reason: " + reason);

	QMessageBox::critical(NULL, PROGRAM_NAME " - Error",
		QString("The connection to Skype failed!  %1 cannot operate without this "
		"connection, please make sure you haven't blocked access from within Skype.\n\n"
		"Internal reason for failure: %2").arg(PROGRAM_NAME, reason));
}

void Recorder::debugMessage(const QString &s) {
	std::cout << s.toLocal8Bit().constData() << "\n";
}

int main(int argc, char **argv) {
	Recorder recorder(argc, argv);

	return recorder.exec();
}

