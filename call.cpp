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
#include <ctime>

#include "call.h"
#include "common.h"
#include "skype.h"
#include "wavewriter.h"
#include "mp3writer.h"
#include "preferences.h"
#include "callgui.h"


Call::Call(QObject *p, Skype *sk, CallID i) :
	QObject(p),
	skype(sk),
	id(i),
	status("UNKNOWN"),
	writer(NULL),
	isRecording(false),
	shouldRecord(1)
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

	setStatus("UNKNOWN");

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

void Call::setStatus(const QString &s) {
	bool wasInProgress = status == "INPROGRESS";
	status = s;
	bool nowInProgress = status == "INPROGRESS";

	if (!wasInProgress && nowInProgress)
		emit startedCall(skypeName);
	else if (wasInProgress && !nowInProgress)
		emit stoppedCall();
}

bool Call::statusDone() const {
	return status == "BUSY" ||
		status == "CANCELLED" ||
		status == "FAILED" ||
		status == "FINISHED" ||
		status == "MISSED" ||
		status == "REFUSED";
	// TODO: see what the deal is with REDIAL_PENDING (protocol 8)
}

QString Call::constructFileName() const {
	return getFileName(skypeName, displayName, skype->getSkypeName(),
		skype->getObject("PROFILE FULLNAME"), timeStartRecording);
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
	if (confirmation) {
		delete confirmation;
		shouldRecord = should;
	}
}

void Call::confirmRecording() {
	shouldRecord = 2;
}

void Call::denyRecording() {
	// note that the call might already be finished by now
	shouldRecord = 0;
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

	timeStartRecording = QDateTime::currentDateTime();
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
			"The replies from Skype were:\n%1\n%2").arg(rep1, rep2));
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
	emit startedRecording();
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
	int samples; // number of samples to write

	if (flush) {
		// when flushing, we pad the shorter buffer, so that all
		// available data is written.  this shouldn't usually be a
		// significant amount, but it might be if there was an audio
		// I/O error in Skype.
		if (l < r) {
			bufferLocal.append(QByteArray(r - l, 0));
			debug(QString("Call %1: padding %2 samples on local buffer while flushing").arg(id).arg((l - r) / 2));
			samples = r / 2;
		} else if (l > r) {
			bufferRemote.append(QByteArray(l - r, 0));
			debug(QString("Call %1: padding %2 samples on remote buffer while flushing").arg(id).arg((l - r) / 2));
			samples = l / 2;
		} else {
			samples = l / 2;
		}
	} else {
		samples = (l < r ? l : r) / 2;
		// skype usually sends more PCM data every 10ms, i.e. 160
		// samples (skype operates at 16kHz).  let's accumulate at
		// least 100ms of data before bothering to write it to disk
		if (samples < 1600)
			return;
	}

	// got new samples to write to file, or have to flush.  note that we
	// have to flush even if samples == 0

	bool success;

	if (channelMode == 0) {
		// mono
		mixToMono(samples);
		QByteArray dummy;
		success = writer->write(bufferMono, dummy, samples, flush);
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
	emit stoppedRecording();
}

// ---- CallHandler ----

CallHandler::CallHandler(QObject *parent, Skype *s) : QObject(parent), skype(s), currentCall(-1) {
}

CallHandler::~CallHandler() {
	prune();

	QList<Call *> list = calls.values();
	if (!list.isEmpty()) {
		debug(QString("Destroying CallHandler, these calls still exist:"));
		for (int i = 0; i < list.size(); i++) {
			Call *c = list.at(i);
			debug(QString("    call %1, status=%2, okToDelete=%3").arg(c->getID()).arg(c->getStatus()).arg(c->okToDelete()));
		}
	}
}

void CallHandler::callCmd(const QStringList &args) {
	CallID id = args.at(0).toInt();

	if (ignore.contains(id))
		return;

	bool newCall = false;

	Call *call;

	if (calls.contains(id)) {
		call = calls[id];
	} else {
		call = new Call(this, skype, id);
		calls[id] = call;
		newCall = true;

		connect(call, SIGNAL(startedCall(const QString &)), this, SIGNAL(startedCall(const QString &)));
		connect(call, SIGNAL(stoppedCall()),                this, SIGNAL(stoppedCall()));
		connect(call, SIGNAL(startedRecording()),           this, SIGNAL(startedRecording()));
		connect(call, SIGNAL(stoppedRecording()),           this, SIGNAL(stoppedRecording()));
	}

	// this holds the current call.  skype currently only allows for one
	// call at any time, so this should work ok
	currentCall = id;

	QString subCmd = args.at(1);

	if (subCmd == "STATUS") {
		QString a = args.at(2);
		call->setStatus(a);

		if (a == "INPROGRESS")
			call->startRecording();

		// don't stop recording when we get "FINISHED".  just wait for
		// the connections to close so that we really get all the data
	} else if (subCmd == "DURATION") {
		/* this is where we start recording calls that are already running, for
		   example if the user starts this program after the call has been placed */
		call->setStatus("INPROGRESS");
		if (newCall)
			call->startRecording();
	}

	prune();
}

void CallHandler::prune() {
	QList<Call *> list = calls.values();
	for (int i = 0; i < list.size(); i++) {
		Call *c = list.at(i);
		if (c->statusDone() && c->okToDelete()) {
			// we ignore this call from now on, because Skype might still send
			// us information about it, like "SEEN" or "VAA_INPUT_STATUS"
			calls.remove(c->getID());
			ignore.insert(c->getID());
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

