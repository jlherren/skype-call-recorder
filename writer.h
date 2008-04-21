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

#ifndef WRITER_H
#define WRITER_H

#include <QFile>

class QString;
class QByteArray;

class AudioFileWriter {
public:
	AudioFileWriter() { };
	virtual ~AudioFileWriter() { };

	// Note: you're not supposed to reopen after a close
	virtual bool open(const QString &, long, bool);
	virtual void close();
	virtual bool write(QByteArray &, QByteArray &, int, bool = false) = 0;
	bool remove();

protected:
	QFile file;
	long sampleRate;
	bool stereo;

private:
	// disabled
	AudioFileWriter(const AudioFileWriter &);
	AudioFileWriter &operator=(const AudioFileWriter &);
};

#endif

