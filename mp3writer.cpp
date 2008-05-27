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
#include <lame/lame.h>
#include <id3/tag.h>
#include "mp3writer.h"
#include "common.h"
#include "preferences.h"

Mp3Writer::Mp3Writer() :
	lame(NULL),
	hasFlushed(false)
{
}

Mp3Writer::~Mp3Writer() {
	if (file.isOpen()) {
		debug("WARNING: Mp3Writer::~Mp3Writer(): File has not been closed, closing it now");
		close();
	}

	if (lame)
		lame_close(lame);
}

bool Mp3Writer::open(const QString &fn, long sr, bool s) {
	bool b = AudioFileWriter::open(fn + ".mp3", sr, s);

	if (!b)
		return false;

	lame = lame_init();
	if (!lame)
		return false;

	bitRate = preferences.get("output.format.mp3.bitrate").toInt();

	lame_set_in_samplerate(lame, sampleRate);
	lame_set_num_channels(lame, stereo ? 2 : 1);
	lame_set_out_samplerate(lame, sampleRate);
	// TODO: do we need this?
	lame_set_bWriteVbrTag(lame, 0);
	lame_set_mode(lame, stereo ? STEREO : MONO);
	lame_set_brate(lame, bitRate);
	if (lame_init_params(lame) == -1)
		return false;

	return true;
}

void Mp3Writer::close() {
	if (!file.isOpen()) {
		debug("WARNING: Mp3Writer::close() called, but file not open");
		return;
	}

	if (!hasFlushed) {
		debug("WARNING: Mp3Writer::close() called but no flush happened, flushing now");
		QByteArray dummy1, dummy2;
		write(dummy1, dummy2, 0, true);
	}

	AudioFileWriter::close();
	writeTags();
}

namespace {
ID3_Frame *getOrCreateTag(ID3_Tag &tag, ID3_FrameID id) {
	ID3_Frame *frame = tag.Find(id);

	if (!frame) {
		frame = new ID3_Frame(id);
		tag.AttachFrame(frame);
	}

	return frame;
}
}

void Mp3Writer::writeTags() {
	if (!mustWriteTags)
		return;

	debug("Writing tags to MP3 file");

	QByteArray fn = QFile::encodeName(file.fileName());
	ID3_Tag tag(fn.constData());

	// NOTE: we don't set ID3FID_TITLE as the file name is already meant to
	// be a good enough description of the content

	QString str = tagTime.toString("yyyyddMMhhmm");
	// TODO: the following would be better but doesn't work.  find out why
	//getOrCreateTag(tag, ID3FID_COMMENT    )->GetField(ID3FN_TEXT)->Set(tagComment.utf16());
	getOrCreateTag(tag, ID3FID_COMMENT    )->GetField(ID3FN_TEXT)->Set(tagComment.toAscii().constData());
	getOrCreateTag(tag, ID3FID_CONTENTTYPE)->GetField(ID3FN_TEXT)->Set("(101)Skype Call");
	getOrCreateTag(tag, ID3FID_YEAR       )->GetField(ID3FN_TEXT)->Set(str.mid(0, 4).toAscii().constData());
	getOrCreateTag(tag, ID3FID_DATE       )->GetField(ID3FN_TEXT)->Set(str.mid(4, 4).toAscii().constData());
	getOrCreateTag(tag, ID3FID_TIME       )->GetField(ID3FN_TEXT)->Set(str.mid(8, 4).toAscii().constData());

	tag.Update();
	mustWriteTags = false;
}

bool Mp3Writer::write(QByteArray &left, QByteArray &right, long samples, bool flush) {
	int ret;
	QByteArray output;
	// rough upper bound formula taken from lame.h
	long size = samples + samples / 4 + 7200;

	do {
		output.resize(size);

		if (stereo) {
			ret = lame_encode_buffer(lame, reinterpret_cast<const short *>(left.constData()),
				reinterpret_cast<const short *>(right.constData()), samples,
				reinterpret_cast<unsigned char *>(output.data()), output.size());
		} else {
			// lame.h claims to write to the buffers, even though they're declared const, be safe
			// TODO: this mixes both channels again!  can lame take only mono samples?
			ret = lame_encode_buffer(lame, reinterpret_cast<const short *>(left.data()),
				reinterpret_cast<const short *>(left.data()), samples,
				reinterpret_cast<unsigned char *>(output.data()), output.size());
		}

		if (ret == -1) {
			// there wasn't enough space in output
			size *= 2;
			continue;
		}
	} while (false);

	if (ret < 0) {
		debug(QString("Error while writing MP3 file, code = %1").arg(ret));
		return false;
	}

	samplesWritten += samples;

	if (ret > 0) {
		output.truncate(ret);
		file.write(output);
	}

	left.remove(0, samples * 2);
	if (stereo)
		right.remove(0, samples * 2);

	if (!flush)
		return true;

	// flush mp3

	output.resize(10240);
	ret = lame_encode_flush(lame, reinterpret_cast<unsigned char *>(output.data()), output.size());

	lame_close(lame);
	lame = NULL;
	hasFlushed = true;

	if (ret < 0) {
		debug(QString("Error while flushing MP3 file, code = %1").arg(ret));
		return false;
	}

	if (ret > 0) {
		output.truncate(ret);
		file.write(output);
	}

	return true;
}

