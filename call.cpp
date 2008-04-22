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
#include <QDir>
#include <ctime>

#include "call.h"
#include "common.h"
#include "skype.h"
#include "wavewriter.h"
#include "mp3writer.h"
#include "preferences.h"


Call::Call(Skype *sk, const QString &sn, CallID i) :
	skype(sk),
	skypeName(sn),
	id(i),
	writer(NULL),
	isRecording(false)
{
	debug(QString("Call %1: Call object contructed").arg(id));

	// Call objects track calls even before they are in progress and also
	// when they are not being recorded.

	// TODO check if we actually should record this call here
	// and ask if we're unsure
}

Call::~Call() {
	debug(QString("Call %1: Call object destructed").arg(id));

	if (isRecording)
		stopRecording();

	// QT takes care of deleting servers and sockets
}

namespace {
QString escape(const QString &s) {
	QString out = s;
	out.replace('%', "%%");
	out.replace('&', "&&");
	return out;
}
}

QString Call::getFileName() const {
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

void Call::startRecording() {
	if (isRecording)
		return;

	debug(QString("Call %1: start recording").arg(id));

	// set up encoder for appropriate format

	QString fileName = getFileName();

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

	bool b = writer->open(fileName, 16000, channelMode != 0);

	if (!b) {
		QMessageBox *box = new QMessageBox(QMessageBox::Critical, PROGRAM_NAME " - Error",
			QString(PROGRAM_NAME " could not open the file %1.  Please verify the output file pattern.").arg(fileName));
		box->setWindowModality(Qt::NonModal);
		box->setAttribute(Qt::WA_DeleteOnClose);
		box->show();
		writer->remove();
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
		writer->remove();
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
	tryToWrite();
}

void Call::readRemote() {
	bufferRemote += socketRemote->readAll();
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

	if (!samples)
		return;

	// got new samples to write to file

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

	isRecording = false;
}


CallHandler::CallHandler(Skype *s) : skype(s) {
}

void CallHandler::closeAll() {
	debug("closing all calls");
	QList<Call *> list = calls.values();
	for (int i = 0; i < list.size(); i++)
		list.at(i)->stopRecording();
}

// ---- CallHandler ----

QString CallHandler::getObject(const QString &object) {
	QString ret = skype->sendWithReply("GET " + object);
	if (!ret.startsWith(object))
		return QString();
	return ret.mid(object.size() + 1);
}

void CallHandler::callCmd(const QStringList &args) {
	CallID id = args.at(0).toInt();

	if (ignore.contains(id))
		return;

	bool newCall = false;

	if (!calls.contains(id)) {
		QString skypeName = getObject(QString("CALL %1 PARTNER_HANDLE").arg(id));
		if (skypeName.isEmpty()) {
			debug(QString("Call %1: cannot get partner handle").arg(id));
			ignore.insert(id);
			return;
		}

		calls[id] = new Call(skype, skypeName, id);
		newCall = true;
	}

	Call *call = calls[id];

	QString subCmd = args.at(1);

	if (subCmd == "STATUS") {
		QString a = args.at(2);
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
}

