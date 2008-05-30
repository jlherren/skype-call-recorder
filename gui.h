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

#ifndef GUI_H
#define GUI_H

#include <QDialog>
#include <QList>
#include <QStyle>
#include <QString>

#include "common.h"

class QWidget;
class QCheckBox;
class QHBoxLayout;
class QVBoxLayout;

// base dialog with a pixmap, a vbox and an hbox

class IconDialogBase : public QDialog {
protected:
	IconDialogBase(const QString &, QStyle::StandardPixmap);

protected:
	QHBoxLayout *hbox;
	QVBoxLayout *vbox;

	DISABLE_COPY_AND_ASSIGNMENT(IconDialogBase);
};

// recording confirmation dialog for calls

class RecordConfirmationDialog : public IconDialogBase {
	Q_OBJECT
public:
	RecordConfirmationDialog(const QString &, const QString &);

signals:
	void yes();
	void no();

private slots:
	void yesClicked();
	void noClicked();
	void enableWidgets();

private:
	QString skypeName;
	QList<QWidget *> widgets;
	QCheckBox *remember;

	DISABLE_COPY_AND_ASSIGNMENT(RecordConfirmationDialog);
};

// information dialog about legality of recording calls

class LegalInformationDialog: public IconDialogBase {
public:
	LegalInformationDialog();

	DISABLE_COPY_AND_ASSIGNMENT(LegalInformationDialog);
};

// about dialog

class AboutDialog : public QDialog {
public:
	AboutDialog();

	DISABLE_COPY_AND_ASSIGNMENT(AboutDialog);
};

// first run dialog

class FirstRunDialog : public IconDialogBase {
public:
	FirstRunDialog();

	DISABLE_COPY_AND_ASSIGNMENT(FirstRunDialog);
};

#endif

