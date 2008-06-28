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

// Note: this file doesn't include comments related to writing Ogg Vorbis
// files.  it is mostly based on the examples/encoder_example.c from the vorbis
// library, so have a look there if you're curious about how this works.

// TODO: currently, this only writes tags while opening the file, but doesn't
// update them if they've been changed before close().  for now this is ok, as
// it never happens.

#include <QByteArray>
#include <QString>
#include <cstdlib>
#include <ctime>
#include <vorbis/vorbisenc.h>

#include "vorbiswriter.h"
#include "common.h"
#include "preferences.h"

struct VorbisWriterPrivateData {
	ogg_stream_state os;
	ogg_page og;
	ogg_packet op;
	vorbis_info vi;
	vorbis_comment vc;
	vorbis_dsp_state vd;
	vorbis_block vb;
};

VorbisWriter::VorbisWriter() :
	pd(NULL),
	hasFlushed(false)
{
}

VorbisWriter::~VorbisWriter() {
	if (file.isOpen()) {
		debug("WARNING: VorbisWriter::~VorbisWriter(): File has not been closed, closing it now");
		close();
	}

	if (pd) {
		ogg_stream_clear(&pd->os);
		vorbis_block_clear(&pd->vb);
		vorbis_dsp_clear(&pd->vd);
		vorbis_comment_clear(&pd->vc);
		vorbis_info_clear(&pd->vi);
		delete pd;
	}
}

bool VorbisWriter::open(const QString &fn, long sr, bool s) {
	bool b = AudioFileWriter::open(fn + ".ogg", sr, s);

	if (!b)
		return false;

	int quality = preferences.get(Pref::OutputFormatVorbisQuality).toInt();

	pd = new VorbisWriterPrivateData;
	vorbis_info_init(&pd->vi);

	if (vorbis_encode_init_vbr(&pd->vi, stereo ? 2 : 1, sampleRate, (float)quality / 10.0f) != 0) {
		delete pd;
		pd = NULL;
		return false;
	}

	// TODO: the docs vaguely mention that stereo coupling can be disabled
	// with vorbis_encode_ctl(), but I didn't find anything concrete

	vorbis_comment_init(&pd->vc);
	// vorbis_comment_add_tag() in libvorbis up to version 1.2.0
	// incorrectly takes a char * instead of a const char *.  to prevent
	// compiler warnings we use const_cast<>(), since it's known that
	// libvorbis does not change the arguments.
	vorbis_comment_add_tag(&pd->vc, const_cast<const char *>("COMMENT"), const_cast<const char *>(tagComment.toUtf8().constData()));
	vorbis_comment_add_tag(&pd->vc, const_cast<const char *>("DATE"), const_cast<const char *>(tagTime.toString("yyyy-MM-dd hh:mm").toAscii().constData()));
	vorbis_comment_add_tag(&pd->vc, const_cast<const char *>("GENRE"), const_cast<const char *>("Speech (Skype Call)"));

	vorbis_analysis_init(&pd->vd, &pd->vi);
	vorbis_block_init(&pd->vd, &pd->vb);

	std::srand(std::time(NULL));
	ogg_stream_init(&pd->os, std::rand());

	ogg_packet header;
	ogg_packet header_comm;
	ogg_packet header_code;

	vorbis_analysis_headerout(&pd->vd, &pd->vc, &header, &header_comm, &header_code);
	ogg_stream_packetin(&pd->os, &header);
	ogg_stream_packetin(&pd->os, &header_comm);
	ogg_stream_packetin(&pd->os, &header_code);

	while (ogg_stream_flush(&pd->os, &pd->og) != 0) {
		file.write((const char *)pd->og.header, pd->og.header_len);
		file.write((const char *)pd->og.body, pd->og.body_len);
	}

	return true;
}

void VorbisWriter::close() {
	if (!file.isOpen()) {
		debug("WARNING: VorbisWriter::close() called, but file not open");
		return;
	}

	if (!hasFlushed) {
		debug("WARNING: VorbisWriter::close() called but no flush happened, flushing now");
		QByteArray dummy1, dummy2;
		write(dummy1, dummy2, 0, true);
	}

	AudioFileWriter::close();
}

bool VorbisWriter::write(QByteArray &left, QByteArray &right, long samples, bool flush) {
	const long maxChunkSize = 4096;

	const qint16 *leftData = (const qint16 *)left.constData();
	const qint16 *rightData = stereo ? (const qint16 *)right.constData() : NULL;

	long todoSamples = samples;
	int eos = 0;

	while (!eos) {
		long chunkSize = todoSamples > maxChunkSize ? maxChunkSize : todoSamples;
		todoSamples -= chunkSize;

		if (chunkSize == 0) {
			if (!flush)
				break;
			hasFlushed = true;
			vorbis_analysis_wrote(&pd->vd, 0);
		} else {
			float **buffer = vorbis_analysis_buffer(&pd->vd, chunkSize);

			for (long i = 0; i < chunkSize; i++)
				buffer[0][i] = (float)leftData[i] / 32768.0f;
			leftData += chunkSize;

			if (stereo) {
				for (long i = 0; i < chunkSize; i++)
					buffer[1][i] = (float)rightData[i] / 32768.0f;
				rightData += chunkSize;
			}

			vorbis_analysis_wrote(&pd->vd, chunkSize);
		}

		while (vorbis_analysis_blockout(&pd->vd, &pd->vb) == 1) {
			vorbis_analysis(&pd->vb, NULL);
			vorbis_bitrate_addblock(&pd->vb);

			while (vorbis_bitrate_flushpacket(&pd->vd, &pd->op)) {
				ogg_stream_packetin(&pd->os, &pd->op);

				while (!eos && ogg_stream_pageout(&pd->os, &pd->og) != 0) {
					file.write((const char *)pd->og.header, pd->og.header_len);
					file.write((const char *)pd->og.body, pd->og.body_len);

					if (ogg_page_eos(&pd->og))
						eos = 1;
				}
			}
		}
	}

	samplesWritten += samples;

	left.remove(0, samples * 2);
	if (stereo)
		right.remove(0, samples * 2);

	return true;
}

