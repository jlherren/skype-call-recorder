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

#include <iostream>

#include <QMessageBox>
#include <QTextEdit>
#include <QTimer>
#include <QDir>
#include <QProcess>
#include <cstdlib>

#include "recorder.h"
#include "common.h"
#include "trayicon.h"
#include "preferences.h"
#include "skype.h"
#include "call.h"

Recorder::Recorder(int argc, char **argv) :
	QApplication(argc, argv),
	debugWidget(NULL)
{
	setDebugHandler(this);

	debug("Initializing application");

	loadSettings();

	setupGUI();
	setupSkype();
	setupCallHandler();

	QTimer::singleShot(0, skype, SLOT(connectToSkype()));
}

Recorder::~Recorder() {
	delete preferencesDialog;
	delete debugWidget;
}

void Recorder::setupGUI() {
	setQuitOnLastWindowClosed(false);

	trayIcon = new TrayIcon(this);
	connect(trayIcon, SIGNAL(requestQuit()),               this, SLOT(quitConfirmation()));
	connect(trayIcon, SIGNAL(requestQuitNoConfirmation()), this, SLOT(quit()));
	connect(trayIcon, SIGNAL(requestAbout()),              this, SLOT(about()));
	connect(trayIcon, SIGNAL(requestOpenSettings()),       this, SLOT(openSettings()));
	connect(trayIcon, SIGNAL(requestBrowseCalls()),        this, SLOT(browseCalls()));

	// TODO: temporary
	debugWidget = new QTextEdit;
	debugWidget->setWindowTitle(PROGRAM_NAME " - Debug");
	debugWidget->setFont(QFont("Arial", 8));
	//debugWidget->show();
	for (int i = 0; i < savedDebugMessages.size(); i++)
		debugWidget->append(savedDebugMessages.at(i));
	savedDebugMessages.clear();

	preferencesDialog = new PreferencesDialog();
	connect(preferencesDialog, SIGNAL(finished(int)), this, SLOT(saveSettings()));

	debug("GUI initialized");
}

void Recorder::setupSkype() {
	skype = new Skype;
	connect(skype, SIGNAL(notify(const QString &)),           this, SLOT(skypeNotify(const QString &)));
	connect(skype, SIGNAL(connected()),                       this, SLOT(skypeConnected()));
	connect(skype, SIGNAL(connectionFailed(const QString &)), this, SLOT(skypeConnectionFailed(const QString &)));
}

void Recorder::setupCallHandler() {
	callHandler = new CallHandler(skype);

	connect(trayIcon, SIGNAL(startRecording()),         callHandler, SLOT(startRecording()));
	connect(trayIcon, SIGNAL(stopRecording()),          callHandler, SLOT(stopRecording()));
	connect(trayIcon, SIGNAL(stopRecordingAndDelete()), callHandler, SLOT(stopRecordingAndDelete()));

	connect(callHandler, SIGNAL(startedCall(const QString &)), trayIcon, SLOT(startedCall(const QString &)));
	connect(callHandler, SIGNAL(stoppedCall()),                trayIcon, SLOT(stoppedCall()));
	connect(callHandler, SIGNAL(startedRecording()),           trayIcon, SLOT(startedRecording()));
	connect(callHandler, SIGNAL(stoppedRecording()),           trayIcon, SLOT(stoppedRecording()));
}

QString Recorder::getConfigFile() const {
	return QDir::homePath() + "/.skypecallrecorder.rc";
}

void Recorder::loadSettings() {
	preferences.load(getConfigFile());
	int c = preferences.count();

	#define X(n, v) preferences.get(#n).setIfNotSet(v);
	// default preferences
	X(autorecord.default,        "ask");         // "yes", "ask", "no"
	X(autorecord.ask,            "");            // comma separated skypenames to always ask for
	X(autorecord.yes,            "");            // comma separated skypenames to always record
	X(autorecord.no,             "");            // comma separated skypenames to never record
	X(output.path,               "~/Skype Calls");
	X(output.pattern,            "%Y, %B/Skype call with &s, %A %B %d, %Y, %H:%M:%S");
	X(output.format,             "mp3");         // "mp3" or "wav"
	X(output.format.mp3.bitrate, 96);
	X(output.channelmode,        "stereo");      // mono, stereo, oerets
	X(output.savetags,           true);
	#undef X

	c = preferences.count() - c;

	if (c)
		debug(QString("Loading %1 built-in default preference(s)").arg(c));
}

void Recorder::saveSettings() {
	preferences.save(getConfigFile());
	// TODO: when failure?
}

void Recorder::about() {
	QMessageBox::information(NULL, PROGRAM_NAME " - About",
		"This is a place holder for a future about dialog.");
}

void Recorder::openSettings() {
	debug("Show preferences dialog");
	preferencesDialog->show();
	preferencesDialog->raise();
	preferencesDialog->activateWindow();
}

void Recorder::browseCalls() {
	QString program;
	QStringList arguments;

	const char *v = std::getenv("GNOME_DESKTOP_SESSION_ID");
	if (v && *v) {
		// GNOME is running
		program = "gnome-open";
	} else {
		// otherwise, just launch kfmclient.  KDE could be detected via
		// KDE_FULL_SESSION=true
		program = "kfmclient";
		arguments << "exec";
	}

	QString path = getOutputPath();
	QDir().mkpath(path);
	arguments << path;
	int ret = QProcess::execute(program, arguments);

	if (ret != 0) {
		QMessageBox::information(NULL, PROGRAM_NAME, QString("Failed to launch '%1 %2', exit code %3").
			arg(program, arguments.join(" ")).arg(ret));
	}
}

void Recorder::quitConfirmation() {
	debug("Request to quit");
	saveSettings();
	quit();
}

void Recorder::skypeNotify(const QString &s) {
	QStringList args = s.split(' ');
	QString cmd = args.takeFirst();
	if (cmd == "CALL")
		callHandler->callCmd(args);
}

void Recorder::skypeConnected() {
	debug("skype connection established");
}

void Recorder::skypeConnectionFailed(const QString &reason) {
	debug("skype connection failed, reason: " + reason);

	QMessageBox::critical(NULL, PROGRAM_NAME " - Error",
		QString("The connection to Skype failed!  %1 cannot operate without this "
		"connection, please make sure you haven't blocked access from within Skype.\n\n"
		"Internal reason for failure: %2").arg(PROGRAM_NAME).arg(reason));
	// TODO: if skype is not running: "skype will now continually poll for connections, blah blah"
}

void Recorder::debugMessage(const QString &s) {
	if (debugWidget)
		debugWidget->append(s);
	else
		savedDebugMessages.append(s);
	std::cout << s.toLocal8Bit().constData() << "\n";
}

int main(int argc, char **argv) {
	Recorder recorder(argc, argv);

	return recorder.exec();
}

