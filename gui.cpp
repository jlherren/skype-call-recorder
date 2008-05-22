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
#include <QPixmap>
#include <QPushButton>
#include <QTimer>

#include "gui.h"
#include "common.h"
#include "preferences.h"
#include "smartwidgets.h"

// ---- RecordConfirmationDialog ----

RecordConfirmationDialog::RecordConfirmationDialog(const QString &sn, const QString &displayName) :
	skypeName(sn)
{
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
		"Do you wish to continue recording or shall it stop and delete the file?").arg(skypeName, displayName));
	vbox->addWidget(label);

	vbox->addSpacing(10);

	remember = new QCheckBox(QString("&Automatically perform this action on the next call with %1").arg(skypeName));
	remember->setEnabled(false);
	widgets.append(remember);
	vbox->addWidget(remember);

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
	if (remember->isChecked())
		preferences.setPerCallerPreference(skypeName, 2);
	accept();
}

void RecordConfirmationDialog::noClicked() {
	emit no();
	if (remember->isChecked())
		preferences.setPerCallerPreference(skypeName, 0);
	accept();
}

void RecordConfirmationDialog::enableWidgets() {
	for (int i = 0; i < widgets.size(); i++)
		widgets.at(i)->setEnabled(true);
}

// ---- LegalInformationDialog ----

LegalInformationDialog::LegalInformationDialog() {
	setWindowTitle(PROGRAM_NAME);
	setAttribute(Qt::WA_DeleteOnClose);

	QHBoxLayout *bighbox = new QHBoxLayout(this);
	bighbox->setSizeConstraint(QLayout::SetFixedSize);

	// get standard icon
	int iconSize = QApplication::style()->pixelMetric(QStyle::PM_MessageBoxIconSize);
	QIcon icon = QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation);
	QLabel *iconLabel = new QLabel;
	iconLabel->setPixmap(icon.pixmap(iconSize, iconSize));
	bighbox->addWidget(iconLabel, 0, Qt::AlignTop);

	bighbox->addSpacing(10);

	QVBoxLayout *vbox = new QVBoxLayout;
	bighbox->addLayout(vbox);

	QLabel *label = new QLabel("Please make sure that recording this call is legal and that all involved parties\nagree with it.");
	vbox->addWidget(label);

	QWidget *additionalInfo = new QWidget;
	additionalInfo->hide();
	QVBoxLayout *additionalInfoLayout = new QVBoxLayout(additionalInfo);
	additionalInfoLayout->setMargin(0);

	additionalInfoLayout->addSpacing(10);

	label = new QLabel(
		"<p>The legality of recording calls depends on whether the involved parties agree<br>"
		"with it and on the laws that apply to their geographic locations.  Here is a<br>"
		"simple rule of thumb:</p>"

		"<p>If all involved parties have been asked for permission to record a call and<br>"
		"agree with the intended use or storage of the resulting file, then recording a<br>"
		"call is probably legal.</p>"

		"<p>If not all involved parties have been asked for permission, or if they don't<br>"
		"explicitely agree with it, or if the resulting file is used for purposes other than<br>"
		"what has been agreed upon, then it's probably illegal to record calls.</p>"

		"<p>For more serious legal advice, consult a lawyer.  For more information, you can<br>"
		"search the internet for '<a href='http://www.google.com/search?q=call+recording+legal'>call recording legal</a>'.</p>"
	);
	label->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
	label->setTextFormat(Qt::RichText);
	label->setOpenExternalLinks(true);
	additionalInfoLayout->addWidget(label);

	additionalInfoLayout->addSpacing(10);

	QWidget *checkBox = new SmartCheckBox("Do not inform about this again", preferences.get("suppress.legalinformation"));
	additionalInfoLayout->addWidget(checkBox);

	vbox->addWidget(additionalInfo);

	QPushButton *button = new QPushButton("More information >>");
	connect(button, SIGNAL(clicked()), button, SLOT(hide()));
	connect(button, SIGNAL(clicked()), additionalInfo, SLOT(show()));
	vbox->addWidget(button, 0, Qt::AlignLeft);

	button = new QPushButton("&OK");
	button->setDefault(true);
	connect(button, SIGNAL(clicked()), this, SLOT(close()));
	vbox->addWidget(button, 0, Qt::AlignHCenter);

	show();
}

// ---- AboutDialog ----

AboutDialog::AboutDialog() {
	setWindowTitle(PROGRAM_NAME " - About");
	setAttribute(Qt::WA_DeleteOnClose);

	QVBoxLayout *vbox = new QVBoxLayout(this);
	vbox->setSizeConstraint(QLayout::SetFixedSize);

	QHBoxLayout *hbox = new QHBoxLayout;
	vbox->addLayout(hbox);

	QLabel *label = new QLabel(
		"<p><font face='Arial' size='20'><b>Skype Call Recorder</b></font></p>"

		"<p>Copyright &copy; 2008 jlh (<a href='mailto:jlh@gmx.ch'>jlh@gmx.ch</a>)<br>"
		"Website: <a href='http://atdot.ch/scr/'>http://atdot.ch/scr/</a></p>"
	);
	label->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
	label->setTextFormat(Qt::RichText);
	label->setOpenExternalLinks(true);
	hbox->addWidget(label, 1, Qt::AlignTop);

	label = new QLabel;
	label->setPixmap(QPixmap(":/icon.png").scaled(QSize(80, 80), Qt::KeepAspectRatio, Qt::SmoothTransformation));
	hbox->addWidget(label, 0, Qt::AlignTop);

	QString str =
		"<hr>"
		"<p>This program is free software; you can redistribute it and/or modify it under<br>"
		"the terms of the GNU General Public License as published by the <a href='http://www.fsf.org/'>Free<br>"
		"Software Foundation</a>; either <a href='http://www.gnu.org/licenses/old-licenses/gpl-2.0.html'>version 2 of the License</a>, "
		"<a href='http://www.gnu.org/licenses/gpl.html'>version 3 of the<br>"
		"License</a>, or (at your option) any later version.</p>"

		"<p>This program is distributed in the hope that it will be useful, but WITHOUT<br>"
		"ANY WARRANTY; without even the implied warranty of MERCHANTABILITY<br>"
		"or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public<br>"
		"License for more details.</p>"
		"<hr>"
		"<p>This product uses the Skype API but is not endorsed, certified or otherwise<br>"
		"approved in any way by Skype.</p>"
		"<hr>"
		"<p><small>Git commit: %1<br>"
		"Build date: %2</small></p>";
	str = str.arg(recorderCommit, recorderDate);
	label = new QLabel(str);
	label->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
	label->setTextFormat(Qt::RichText);
	label->setOpenExternalLinks(true);
	vbox->addWidget(label);

	QPushButton *button = new QPushButton("&Close");
	connect(button, SIGNAL(clicked()), this, SLOT(close()));
	vbox->addWidget(button);

	show();
}

