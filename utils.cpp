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

#include <QFile>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "utils.h"
#include "common.h"

LockFile::LockFile() :
	fd(-1)
{
}

LockFile::~LockFile() {
	unlock();
}

bool LockFile::lock(const QString &fn) {
	fileName = fn;
	fd = open(QFile::encodeName(fileName).constData(), O_CREAT | O_WRONLY, 0644);

	if (fd < 0) {
		debug("ERROR: opening lock file failed");
		return false;
	}

	if (flock(fd, LOCK_EX | LOCK_NB) != 0) {
		close(fd);
		fd = -1;
		debug("ERROR: cannot get lock on lock file");
		return false;
	}

	// set the FD_CLOEXEC flag, so that when another process forks off of
	// us, it won't inherit the file descriptor and the lock.  see commit
	// ce6f838b4587528630784c1ea1ad272b64e78544 to see why we do this

	int flags = fcntl(fd, F_GETFD, 0);

	if (flags == -1) {
		debug("ERROR: failed to fcntl() lock file");
		unlock();
		return false;
	}

	flags |= FD_CLOEXEC;
	flags = fcntl(fd, F_SETFD, flags);

	if (flags == -1) {
		debug("ERROR: failed to set FD_CLOEXEC on lock file");
		unlock();
		return false;
	}

	debug("Got lock on lock file");
	return true;
}

void LockFile::unlock() {
	if (!isLocked())
		return;
	QFile::remove(fileName);
	flock(fd, LOCK_UN);
	close(fd);
	fd = -1;
}

bool LockFile::isLocked() const {
	return fd >= 0;
}

