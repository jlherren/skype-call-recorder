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
#include <QIcon>
#include <QPixmap>
#include <QTimer>

#include "gui.h"
#include "common.h"
#include "preferences.h"
#include "smartwidgets.h"

// ---- IconDialogBase

IconDialogBase::IconDialogBase(const QString &title, QStyle::StandardPixmap pixmap) {
	setWindowTitle(QString(PROGRAM_NAME) + title);
	setAttribute(Qt::WA_DeleteOnClose);

	QVBoxLayout *mainvbox = new QVBoxLayout(this);
	mainvbox->setSizeConstraint(QLayout::SetFixedSize);

	QHBoxLayout *bighbox = new QHBoxLayout;
	hbox = new QHBoxLayout;

	mainvbox->addLayout(bighbox, 1);
	mainvbox->addLayout(hbox);

	// get standard icon
	int iconSize = QApplication::style()->pixelMetric(QStyle::PM_MessageBoxIconSize);
	QIcon icon = QApplication::style()->standardIcon(pixmap);
	QLabel *iconLabel = new QLabel;
	iconLabel->setPixmap(icon.pixmap(iconSize, iconSize));
	bighbox->addWidget(iconLabel, 0, Qt::AlignTop);

	bighbox->addSpacing(iconSize / 3);

	vbox = new QVBoxLayout;
	bighbox->addLayout(vbox, 1);
}

// ---- RecordConfirmationDialog ----

RecordConfirmationDialog::RecordConfirmationDialog(const QString &sn, const QString &displayName) :
	IconDialogBase("Recording confirmation", QStyle::SP_MessageBoxQuestion),
	skypeName(sn)
{
	QLabel *label = new QLabel(QString(PROGRAM_NAME " has started recording the call with <b>%1</b> (%2).<br>"
		"Do you wish to continue recording or shall it stop and delete the file?").arg(skypeName, displayName));
	vbox->addWidget(label);

	vbox->addSpacing(10);

	remember = new QCheckBox(QString("&Automatically perform this action on the next call with %1").arg(skypeName));
	remember->setEnabled(false);
	widgets.append(remember);
	vbox->addWidget(remember);

	hbox->addStretch(1);

	QPushButton *button = new QPushButton("&Continue recording");
	button->setEnabled(false);
	button->setDefault(true);
	button->setMinimumWidth(180);
	widgets.append(button);
	connect(button, SIGNAL(clicked()), this, SLOT(yesClicked()));
	hbox->addWidget(button);

	button = new QPushButton("&Stop recording and delete");
	button->setEnabled(false);
	button->setMinimumWidth(180);
	widgets.append(button);
	connect(button, SIGNAL(clicked()), this, SLOT(noClicked()));
	hbox->addWidget(button);

	hbox->addStretch(1);

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

LegalInformationDialog::LegalInformationDialog() :
	IconDialogBase("Legal information", QStyle::SP_MessageBoxInformation)
{
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

	QWidget *checkBox = new SmartCheckBox("Do not show this information again", preferences.get(Pref::SuppressLegalInformation));
	additionalInfoLayout->addWidget(checkBox);

	vbox->addWidget(additionalInfo);

	QPushButton *button = new QPushButton("More information >>");
	connect(button, SIGNAL(clicked()), button, SLOT(hide()));
	connect(button, SIGNAL(clicked()), additionalInfo, SLOT(show()));
	vbox->addWidget(button, 0, Qt::AlignLeft);

	button = new QPushButton("&OK");
	button->setDefault(true);
	connect(button, SIGNAL(clicked()), this, SLOT(close()));
	hbox->addWidget(button, 0, Qt::AlignHCenter);

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

	QString str =
		"<p><font face='Arial' size='20'><b>Skype Call Recorder</b></font></p>"

		"<p>Copyright &copy; 2008 jlh (<a href='mailto:jlh@gmx.ch'>jlh@gmx.ch</a>)<br>"
		"Version: %1<br>"
		"Website: <a href='%2'>%3</a></p>";
	str = str.arg(recorderVersion).arg(websiteURL).arg(websiteURL);
	QLabel *label = new QLabel(str);
	label->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
	label->setTextFormat(Qt::RichText);
	label->setOpenExternalLinks(true);
	hbox->addWidget(label, 1, Qt::AlignTop);

	label = new QLabel;
	label->setPixmap(QPixmap(":/icon.png").scaled(QSize(80, 80), Qt::KeepAspectRatio, Qt::SmoothTransformation));
	hbox->addWidget(label, 0, Qt::AlignTop);

	str =
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

// ---- FirstRunDialog

FirstRunDialog::FirstRunDialog() :
	IconDialogBase("Information", QStyle::SP_MessageBoxInformation)
{
	QLabel *label = new QLabel(
		"<p>Welcome to Skype Call Recorder!</p>"

		"<p>Please note that Skype Call Recorder does not have a main window.<br>"
		"Instead, it hides itself in the system tray, from where you can open<br>"
		"the preferences dialog, start or stop recording your Skype calls, or<br>"
		"access previously recorded calls.</p>"

		"<p>Thank you for using Skype Call Recorder!</p>"
	);
	label->setTextFormat(Qt::RichText);
	vbox->addWidget(label);

	QWidget *checkBox = new SmartCheckBox("Do not show this information again", preferences.get(Pref::SuppressFirstRunInformation));
	vbox->addWidget(checkBox);

	QPushButton *button = new QPushButton("&OK");
	button->setDefault(true);
	connect(button, SIGNAL(clicked()), this, SLOT(close()));
	hbox->addWidget(button, 0, Qt::AlignHCenter);

	show();
}

// ---- NoSystemTrayDialog ----

NoSystemTrayDialog::NoSystemTrayDialog() :
	IconDialogBase("Missing system tray", QStyle::SP_MessageBoxQuestion)
{
	QLabel *label = new QLabel(
		PROGRAM_NAME " could not detect a system tray.  The system tray is the main point\n"
		"of interaction with Skype Call Recorder.  Do you wish to use a small main window instead?"
	);
	vbox->addWidget(label);

	QPushButton *button = new QPushButton("Yes, &always from now on");
	connect(button, SIGNAL(clicked()), this, SLOT(buttonAlways()));
	hbox->addWidget(button);

	button = new QPushButton("&Yes, this time");
	button->setDefault(true);
	connect(button, SIGNAL(clicked()), this, SLOT(buttonYes()));
	hbox->addWidget(button);

	button = new QPushButton("&Quit");
	connect(button, SIGNAL(clicked()), this, SLOT(buttonDoQuit()));
	hbox->addWidget(button);

	show();
}

void NoSystemTrayDialog::buttonAlways() {
	emit useWindowedModeAlways();
	accept();
}

void NoSystemTrayDialog::buttonYes() {
	emit useWindowedModeNow();
	accept();
}

void NoSystemTrayDialog::buttonDoQuit() {
	emit doQuit();
	accept();
}

// ---- MainWindow

MainWindow::MainWindow(QWidget *parent) : QWidget(parent) {
	setWindowTitle(PROGRAM_NAME);
	setAttribute(Qt::WA_DeleteOnClose);

	QVBoxLayout *vbox = new QVBoxLayout(this);
	vbox->setSizeConstraint(QLayout::SetFixedSize);
	button = new QPushButton(QIcon(":/icon.png"), "Menu");
	vbox->addWidget(button);

	connect(button, SIGNAL(clicked()), this, SIGNAL(activate()));

	show();
}

void MainWindow::setColor(bool color) {
	button->setIcon(QIcon(color ? ":/icon.png" : ":/icongray.png"));
}

