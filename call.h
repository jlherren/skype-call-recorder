/*
	Skype Call Recorder
	Copyright 2008 - 2009 by jlh (jlh at gmx dot ch)

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

#ifndef CALL_H
#define CALL_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QMap>
#include <QSet>
#include <QPointer>
#include <QDateTime>
#include <QTime>
#include <QFile>

#include "common.h"

class QStringList;
class Skype;
class AudioFileWriter;
class QTcpServer;
class QTcpSocket;
class LegalInformationDialog;

class CallHandler;

typedef int CallID;

class AutoSync {
public:
	AutoSync(int, long);
	~AutoSync();
	void add(long);
	long getSync();
	void reset();

private:
	long *delays;
	int size;
	int index;
	long sum;
	qint64 sum2;
	long precision;
	int suppress;

	DISABLE_COPY_AND_ASSIGNMENT(AutoSync);
};

class Call : public QObject {
	Q_OBJECT
public:
	Call(CallHandler *, Skype *, CallID);
	~Call();
	void startRecording(bool = false);
	void stopRecording(bool = true);
	void updateConfID();
	bool okToDelete() const;
	void setStatus(const QString &);
	QString getStatus() const { return status; }
	bool statusDone() const;
	bool statusActive() const;
	CallID getID() const { return id; }
	CallID getConfID() const { return confID; }
	void removeFile();
	void hideConfirmation(int);
	bool getIsRecording() const { return isRecording; }

signals:
	void startedCall(int, const QString &);
	void stoppedCall(int);
	void startedRecording(int);
	void stoppedRecording(int);
	void showLegalInformation();

private:
	QString constructFileName() const;
	QString constructCommentTag() const;
	void mixToMono(long);
	void mixToStereo(long, int);
	void setShouldRecord();
	void ask();
	void doSync(long);

private:
	Skype *skype;
	CallHandler *handler;
	CallID id;
	QString status;
	QString skypeName;
	QString displayName;
	CallID confID;
	AudioFileWriter *writer;
	bool isRecording;
	int stereo;
	int stereoMix;
	int shouldRecord;
	QString fileName;
	QPointer<QObject> confirmation;
	QDateTime timeStartRecording;

	QTime syncTime;
	QFile syncFile;
	AutoSync sync;

	QTcpServer *serverLocal, *serverRemote;
	QTcpSocket *socketLocal, *socketRemote;
	QByteArray bufferLocal, bufferRemote;

private slots:
	void acceptLocal();
	void acceptRemote();
	void readLocal();
	void readRemote();
	void checkConnections();
	long padBuffers();
	void tryToWrite(bool = false);
	void confirmRecording();
	void denyRecording();

private: // moc gets confused without this private:
	DISABLE_COPY_AND_ASSIGNMENT(Call);
};

class CallHandler : public QObject {
	Q_OBJECT
public:
	CallHandler(QObject *, Skype *);
	~CallHandler();
	void updateConfIDs();
	bool isConferenceRecording(CallID) const;
	void callCmd(const QStringList &);

signals:
	// note that {start,stop}Recording signals are not guaranteed to always
	// happen within a {start,stop}Call block.  Especially,
	// stoppedRecording will usually happen *after* stoppedCall
	void startedCall(int, const QString &);
	void stoppedCall(int);
	void startedRecording(int);
	void stoppedRecording(int);

public slots:
	void startRecording(int);
	void stopRecording(int);
	void stopRecordingAndDelete(int);

private slots:
	void showLegalInformation();

private:
	void prune();

private:
	typedef QMap<CallID, Call *> CallMap;
	typedef QSet<CallID> CallSet;

	CallMap calls;
	CallSet ignore;
	Skype *skype;
	QPointer<LegalInformationDialog> legalInformationDialog;

	DISABLE_COPY_AND_ASSIGNMENT(CallHandler);
};

#endif

