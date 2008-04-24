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

#include <QStringList>
#include <QList>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QDir>
#include <QApplication>
#include <QStyle>
#include <QIcon>
#include <QTimer>
#include <ctime>

#include "call.h"
#include "common.h"
#include "skype.h"
#include "wavewriter.h"
#include "mp3writer.h"
#include "preferences.h"


Call::Call(QObject *p, Skype *sk, CallID i) :
	QObject(p),
	skype(sk),
	id(i),
	status("UNKNOWN"),
	writer(NULL),
	isRecording(false),
	shouldRecord(1),
	confirmation(NULL)
{
	debug(QString("Call %1: Call object contructed").arg(id));

	// Call objects track calls even before they are in progress and also
	// when they are not being recorded.

	// TODO check if we actually should record this call here
	// and ask if we're unsure

	skypeName = skype->getObject(QString("CALL %1 PARTNER_HANDLE").arg(id));
	if (skypeName.isEmpty()) {
		debug(QString("Call %1: cannot get partner handle").arg(id));
		skypeName = "UnknownCaller";
	}

	displayName = skype->getObject(QString("CALL %1 PARTNER_DISPNAME").arg(id));
	if (displayName.isEmpty()) {
		debug(QString("Call %1: cannot get partner display name").arg(id));
		displayName = "Unnamed Caller";
	}
}

Call::~Call() {
	debug(QString("Call %1: Call object destructed").arg(id));

	if (isRecording)
		stopRecording();

	delete confirmation;

	// QT takes care of deleting servers and sockets
}

bool Call::okToDelete() const {
	// this is used for checking whether past calls may now be deleted.
	// when a past call hasn't been decided yet whether it should have been
	// recorded, then it may not be deleted until the decision has been
	// made by the user.

	if (isRecording)
		return false;

	if (confirmation)
		/* confirmation dialog still open */
		return false;

	return true;
}

namespace {
QString escape(const QString &s) {
	QString out = s;
	out.replace('%', "%%");
	out.replace('&', "&&");
	return out;
}
}

QString Call::constructFileName() const {
	QString path = preferences.get("output.path").toString();
	QString fileName = preferences.get("output.pattern").toString();

	path.replace('~', QDir::homePath());

	fileName.replace("&s", escape(skypeName));
	// TODO
	//fileName.replace("&f", escape(fullName));
	//fileName.replace("&t", escape(mySkypeName));
	//fileName.replace("&g", escape(myFullName));
	fileName.replace("&&", "&");

	// TODO: uhm, does QT provide any time formatting the strftime() way?
	char *buf = new char[fileName.size() + 1024];
	time_t t = std::time(NULL);
	struct tm *tm = std::localtime(&t);
	std::strftime(buf, fileName.size() + 1024, fileName.toUtf8().constData(), tm);
	fileName = buf;
	delete[] buf;

	return path + '/' + fileName;
}

void Call::setShouldRecord() {
	// this sets shouldRecord based on preferences.  shouldRecord is 0 if
	// the call should not be recorded, 1 if we should ask and 2 if we
	// should record

	QStringList list = preferences.get("autorecord.yes").toList();
	if (list.contains(skypeName)) {
		shouldRecord = 2;
		return;
	}

	list = preferences.get("autorecord.ask").toList();
	if (list.contains(skypeName)) {
		shouldRecord = 1;
		return;
	}

	list = preferences.get("autorecord.no").toList();
	if (list.contains(skypeName)) {
		shouldRecord = 0;
		return;
	}

	QString def = preferences.get("autorecord.default").toString();
	if (def == "yes")
		shouldRecord = 2;
	else if (def == "ask")
		shouldRecord = 1;
	else if (def == "no")
		shouldRecord = 0;
	else
		shouldRecord = 1;
}

void Call::ask() {
	confirmation = new RecordConfirmationDialog(skypeName, displayName);
	connect(confirmation, SIGNAL(yes()), this, SLOT(confirmRecording()));
	connect(confirmation, SIGNAL(no()), this, SLOT(denyRecording()));
}

void Call::hideConfirmation(int should) {
	if (!confirmation)
		return;
	delete confirmation;
	confirmation = NULL;
	shouldRecord = should;
}

void Call::confirmRecording() {
	shouldRecord = 2;
	confirmation = NULL;
}

void Call::denyRecording() {
	// note that the call might already be finished by now
	shouldRecord = 0;
	confirmation = NULL;
	stopRecording(true);
	removeFile();
}

void Call::removeFile() {
	debug(QString("Removing '%1'").arg(fileName));
	QFile::remove(fileName);
}

void Call::startRecording(bool force) {
	if (force)
		hideConfirmation(2);

	if (isRecording)
		return;

	if (!force) {
		setShouldRecord();
		if (shouldRecord == 0)
			return;
		if (shouldRecord == 1)
			ask();
	}

	debug(QString("Call %1: start recording").arg(id));

	// set up encoder for appropriate format

	QString fn = constructFileName();

	QString sm = preferences.get("output.channelmode").toString();

	if (sm == "mono")
		channelMode = 0;
	else if (sm == "oerets")
		channelMode = 2;
	else /* if (sm == "stereo") */
		channelMode = 1;

	QString format = preferences.get("output.format").toString();

	if (format == "wav")
		writer = new WaveWriter;
	else /* if (format == "mp3") */
		writer = new Mp3Writer;

	bool b = writer->open(fn, 16000, channelMode != 0);
	fileName = writer->fileName();

	if (!b) {
		QMessageBox *box = new QMessageBox(QMessageBox::Critical, PROGRAM_NAME " - Error",
			QString(PROGRAM_NAME " could not open the file %1.  Please verify the output file pattern.").arg(fileName));
		box->setWindowModality(Qt::NonModal);
		box->setAttribute(Qt::WA_DeleteOnClose);
		box->show();
		removeFile();
		delete writer;
		return;
	}

	serverLocal = new QTcpServer(this);
	serverLocal->listen();
	connect(serverLocal, SIGNAL(newConnection()), this, SLOT(acceptLocal()));
	serverRemote = new QTcpServer(this);
	serverRemote->listen();
	connect(serverRemote, SIGNAL(newConnection()), this, SLOT(acceptRemote()));

	QString rep1 = skype->sendWithReply(QString("ALTER CALL %1 SET_CAPTURE_MIC PORT=\"%2\"").arg(id).arg(serverLocal->serverPort()));
	QString rep2 = skype->sendWithReply(QString("ALTER CALL %1 SET_OUTPUT SOUNDCARD=\"default\" PORT=\"%2\"").arg(id).arg(serverRemote->serverPort()));

	if (!rep1.startsWith("ALTER CALL ") || !rep2.startsWith("ALTER CALL")) {
		QMessageBox *box = new QMessageBox(QMessageBox::Critical, PROGRAM_NAME " - Error",
			QString(PROGRAM_NAME " could not obtain the audio streams from Skype and can thus not record this call.\n\n"
			"The replies from Skype were:\n%1\n%2").arg(rep1).arg(rep2));
		box->setWindowModality(Qt::NonModal);
		box->setAttribute(Qt::WA_DeleteOnClose);
		box->show();
		removeFile();
		delete writer;
		delete serverRemote;
		delete serverLocal;
		return;
	}

	isRecording = true;
}

void Call::acceptLocal() {
	socketLocal = serverLocal->nextPendingConnection();
	serverLocal->close();
	// we don't delete the server, since it contains the socket.
	// we could reparent, but that automatic stuff of QT is great
	connect(socketLocal, SIGNAL(readyRead()), this, SLOT(readLocal()));
	connect(socketLocal, SIGNAL(disconnected()), this, SLOT(checkConnections()));
}

void Call::acceptRemote() {
	socketRemote = serverRemote->nextPendingConnection();
	serverRemote->close();
	connect(socketRemote, SIGNAL(readyRead()), this, SLOT(readRemote()));
	connect(socketRemote, SIGNAL(disconnected()), this, SLOT(checkConnections()));
}

void Call::readLocal() {
	bufferLocal += socketLocal->readAll();
	if (isRecording)
		tryToWrite();
}

void Call::readRemote() {
	bufferRemote += socketRemote->readAll();
	if (isRecording)
		tryToWrite();
}

void Call::checkConnections() {
	if (socketLocal->state() == QAbstractSocket::UnconnectedState && socketRemote->state() == QAbstractSocket::UnconnectedState) {
		debug(QString("Call %1: both connections closed, stop recording").arg(id));
		stopRecording();
	}
}

void Call::mixToMono(int samples) {
	long offset = bufferMono.size();
	bufferMono.resize(offset + samples * 2);

	qint16 *monoData = reinterpret_cast<qint16 *>(bufferMono.data()) + offset;
	qint16 *localData = reinterpret_cast<qint16 *>(bufferLocal.data());
	qint16 *remoteData = reinterpret_cast<qint16 *>(bufferRemote.data());

	for (int i = 0; i < samples; i++) {
		long sum = localData[i] + remoteData[i];
		if (sum < -32768)
			sum = -32768;
		else if (sum > 32767)
			sum = 32767;
		monoData[i] = sum;
	}

	bufferLocal.remove(0, samples * 2);
	bufferRemote.remove(0, samples * 2);
}

void Call::tryToWrite(bool flush) {
	//debug(QString("Situation: %3, %4").arg(bufferLocal.size()).arg(bufferRemote.size()));

	int l = bufferLocal.size();
	int r = bufferRemote.size();
	int samples = (l < r ? l : r) / 2;

	if (!samples && !flush)
		return;

	// got new samples to write to file, or have to flush

	bool success;

	if (channelMode == 0) {
		// mono
		mixToMono(samples);
		success = writer->write(bufferMono, bufferMono, samples, flush);
	} else if (channelMode == 1) {
		// stereo
		success = writer->write(bufferLocal, bufferRemote, samples, flush);
	} else if (channelMode == 2) {
		// oerets
		success = writer->write(bufferRemote, bufferLocal, samples, flush);
	} else {
		success = false;
	}

	if (!success) {
		QMessageBox *box = new QMessageBox(QMessageBox::Critical, PROGRAM_NAME " - Error",
			QString(PROGRAM_NAME " encountered an error while writing this call to disk.  Recording terminated."));
		box->setWindowModality(Qt::NonModal);
		box->setAttribute(Qt::WA_DeleteOnClose);
		box->show();
		stopRecording(false);
		return;
	}

	// the writer will remove the samples from the buffers
	//debug(QString("Call %1: wrote %2 samples").arg(id).arg(samples));

	// TODO: handle the case where the two streams get out of sync (buffers
	// not equally fulled by a significant amount).  does skype document
	// whether we always get two nice, equal, in-sync streams, even if
	// there have been transmission errors?  perl-script behavior: if out
	// of sync by more than 6.4ms, then remove 1ms from the stream that's
	// ahead.
}

void Call::stopRecording(bool flush) {
	if (!isRecording)
		return;

	debug(QString("Call %1: stop recording").arg(id));

	// NOTE: we don't delete the sockets here, because we may be here as a
	// reaction to their disconnected() signals; and they don't like being
	// deleted during their signals.  we don't delete the servers either,
	// since they own the sockets and we're too lazy to reparent.  it's
	// easiest to let QT handle all this on its own.  there will be some
	// memory wasted if you start/stop recording within the same call a few
	// times, but unless you do it thousands of times, the waste is more
	// than acceptable.

	// flush data to writer
	if (flush)
		tryToWrite(true);
	writer->close();
	delete writer;

	// we must disconnect all signals from the sockets first, so that upon
	// closing them it won't call checkConnections() and we don't land here
	// recursively again
	disconnect(socketLocal, 0, this, 0);
	disconnect(socketRemote, 0, this, 0);
	socketLocal->close();
	socketRemote->close();

	isRecording = false;
}

// ---- CallHandler ----

CallHandler::CallHandler(Skype *s) : skype(s), currentCall(-1) {
}

void CallHandler::callCmd(const QStringList &args) {
	CallID id = args.at(0).toInt();

	if (ignore.contains(id))
		return;

	bool newCall = false;

	if (!calls.contains(id)) {
		calls[id] = new Call(this, skype, id);
		newCall = true;
	}

	Call *call = calls[id];

	// this holds the current call.  skype currently only allows for one
	// call at any time, so this should work ok
	currentCall = id;

	QString subCmd = args.at(1);

	if (subCmd == "STATUS") {
		QString a = args.at(2);
		call->setStatus(a);

		if (a == "INPROGRESS")
			call->startRecording();
		else if (a == "DURATION") {
			/* this is where we start recording calls that are already running, for
			   example if the user starts this program after the call has been placed */
			if (newCall)
				call->startRecording();
		}

		// don't stop recording when we get "FINISHED".  just wait for
		// the connections to close so that we really get all the data
	}

	QList<Call *> list = calls.values();
	for (int i = 0; i < list.size(); i++) {
		Call *c = list.at(i);
		QString status = c->getStatus();
		if ((status == "FINISHED" || status == "CANCELLED") && c->okToDelete()) {
			calls.remove(c->getID());
			delete c;
		}
	}
}

void CallHandler::startRecording() {
	if (!calls.contains(currentCall))
		return;

	calls[currentCall]->startRecording(true);
}

void CallHandler::stopRecording() {
	if (!calls.contains(currentCall))
		return;

	Call *call = calls[currentCall];
	call->stopRecording();
	call->hideConfirmation(2);
}

void CallHandler::stopRecordingAndDelete() {
	if (!calls.contains(currentCall))
		return;

	Call *call = calls[currentCall];
	call->stopRecording();
	call->removeFile();
	call->hideConfirmation(0);
}

// ---- RecordConfirmationDialog ----

RecordConfirmationDialog::RecordConfirmationDialog(const QString &skypeName, const QString &displayName) {
	setWindowTitle(PROGRAM_NAME);
	setAttribute(Qt::WA_DeleteOnClose);

	QHBoxLayout *bighbox = new QHBoxLayout(this);
	bighbox->setSizeConstraint(QLayout::SetFixedSize);

	// get standard icon
	int iconSize = QApplication::style()->pixelMetric(QStyle::PM_MessageBoxIconSize);
	QIcon icon = QApplication::style()->standardIcon(QStyle::SP_MessageBoxQuestion);
	QLabel *iconLabel = new QLabel;
	iconLabel->setPixmap(icon.pixmap(iconSize, iconSize));
	bighbox->addWidget(iconLabel, 0, Qt::AlignTop);

	bighbox->addSpacing(10);

	QVBoxLayout *vbox = new QVBoxLayout;
	bighbox->addLayout(vbox);

	QLabel *label = new QLabel(QString(PROGRAM_NAME " has started recording the call with <b>%1</b> (%2).<br>"
		"Do you wish to continue recording or shall it stop and delete the file?").arg(skypeName).arg(displayName));
	vbox->addWidget(label);

	vbox->addSpacing(10);

	QCheckBox *check = new QCheckBox(QString("&Automatically perform this action on the next call with %1").arg(skypeName));
	check->setEnabled(false);
	//widgets.append(check);
	vbox->addWidget(check);

	QHBoxLayout *hbox = new QHBoxLayout;

	QPushButton *button = new QPushButton("&Continue recording");
	button->setEnabled(false);
	button->setDefault(true);
	widgets.append(button);
	connect(button, SIGNAL(clicked()), this, SLOT(yesClicked()));
	hbox->addWidget(button);

	button = new QPushButton("&Stop recording and delete");
	button->setEnabled(false);
	widgets.append(button);
	connect(button, SIGNAL(clicked()), this, SLOT(noClicked()));
	hbox->addWidget(button);

	vbox->addLayout(hbox);

	connect(this, SIGNAL(rejected()), this, SIGNAL(no()));
	QTimer::singleShot(1000, this, SLOT(enableWidgets()));

	show();
	raise();
	activateWindow();
}

void RecordConfirmationDialog::yesClicked() {
	emit yes();
	// TODO update preferences depending on checkbox
	accept();
}

void RecordConfirmationDialog::noClicked() {
	emit no();
	// TODO update preferences depending on checkbox
	accept();
}

void RecordConfirmationDialog::enableWidgets() {
	for (int i = 0; i < widgets.size(); i++)
		widgets.at(i)->setEnabled(true);
}

