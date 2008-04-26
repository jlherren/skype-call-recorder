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

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QApplication>
#include <QStyle>
#include <QIcon>
#include <QTimer>

#include "callgui.h"
#include "common.h"

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

