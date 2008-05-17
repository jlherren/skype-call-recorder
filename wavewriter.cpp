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

#include <QByteArray>
#include <QString>
#include "wavewriter.h"
#include "common.h"

// little-endian helper class

class LittleEndianArray : public QByteArray {
public:
	void appendUInt16(int i) {
		append((char)i);
		append((char)(i >> 8));
	}

	void appendUInt32(long i) {
		append((char)i);
		append((char)(i >> 8));
		append((char)(i >> 16));
		append((char)(i >> 24));
	}
};

// WaveWriter

WaveWriter::WaveWriter() :
	hasFlushed(false)
{
}

WaveWriter::~WaveWriter() {
	if (file.isOpen()) {
		debug("WARNING: WaveWriter::~WaveWriter(): File has not been closed, closing it now");
		close();
	}
}

bool WaveWriter::open(const QString &fn, long sr, bool s) {
	bool b = AudioFileWriter::open(fn + ".wav", sr, s);

	if (!b)
		return false;

	updateHeaderInterval = sampleRate; // update header every second
	nextUpdateHeader = sampleRate;

	int channels = stereo ? 2 : 1;
	LittleEndianArray array;
	array.reserve(44);

	// main header
	array.append("RIFF");             // RIFF signature
	fileSizeOffset = array.size();
	array.appendUInt32(0);            // file size excluding signature and this size
	array.append("WAVE");             // RIFF type
	// format chunk
	array.append("fmt ");             // chunk name
	array.appendUInt32(16);           // chunk size excluding name and this size
	array.appendUInt16(1);            // compression code, 1 == PCM uncompressed
	array.appendUInt16(channels);     // number of channels
	array.appendUInt32(sampleRate);   // sample rate
	array.appendUInt32(channels * 2 * sampleRate); // average bytes per second, block align * sample rate
	array.appendUInt16(channels * 2); // block align for each sample group, (usually) significant bits / 8 * number of channels
	array.appendUInt16(16);           // significant bits per sample
	// data chunk
	array.append("data");             // chunk name
	dataSizeOffset = array.size();
	array.appendUInt32(0);            // chunk size excluding name and this size
	// PCM data follows

	qint64 w = file.write(array);

	fileSize = array.size() - 8;
	dataSize = 0;

	if (w < 0)
		return false;

	// Note: the file size field and the "data" chunk size field can't be
	// filled in yet, which is why we put zero in there for now.  some
	// players can play those files anyway, but we'll seek back and update
	// these fields every now and then, so that even if we crash, we'll
	// have a valid wav file (with potentially trailing data)

	return true;
}

void WaveWriter::close() {
	if (!file.isOpen()) {
		debug("WARNING: WaveWriter::close() called, but file not open");
		return;
	}

	if (!hasFlushed) {
		debug("WARNING: WaveWriter::close() called but no flush happened, flushing now");
		QByteArray dummy1, dummy2;
		write(dummy1, dummy2, 0, true);
	}

	AudioFileWriter::close();
}

bool WaveWriter::write(QByteArray &left, QByteArray &right, int samples, bool flush) {
	QByteArray output;

	if (stereo) {
		// interleave data... TODO: is this something that advanced
		// processors instructions can handle faster?

		output.resize(samples * 4);
		qint16 *outputData = reinterpret_cast<qint16 *>(output.data());
		qint16 *leftData = reinterpret_cast<qint16 *>(left.data());
		qint16 *rightData = reinterpret_cast<qint16 *>(right.data());

		for (int i = 0; i < samples; i++) {
			outputData[i * 2] = leftData[i];
			outputData[i * 2 + 1] = rightData[i];
		}
	} else {
		output = left;
		output.truncate(samples * 2);
	}

	bool ret = file.write(output);

	fileSize += output.size();
	dataSize += output.size();
	samplesWritten += samples;

	left.remove(0, samples * 2);
	if (stereo)
		right.remove(0, samples * 2);

	if (!ret)
		return false;

	nextUpdateHeader -= samples;
	if (flush || nextUpdateHeader <= 0) {
		nextUpdateHeader = updateHeaderInterval;
		updateHeader();

		if (flush)
			hasFlushed = true;
	}

	return true;
}

void WaveWriter::updateHeader() {
	qint64 pos = file.pos();
	LittleEndianArray tmp;

	tmp.appendUInt32(fileSize);
	file.seek(fileSizeOffset);
	file.write(tmp);

	tmp.clear();
	tmp.appendUInt32(dataSize);
	file.seek(dataSizeOffset);
	file.write(tmp);

	file.seek(pos);
}

