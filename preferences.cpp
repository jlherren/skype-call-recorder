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

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QListView>
#include <QFile>
//#include <QSet>
#include <QTextStream>
#include <QtAlgorithms>

#include "preferences.h"
#include "smartwidgets.h"
#include "common.h"

Preferences preferences;

// preferences dialog

static QVBoxLayout *makeVFrame(QVBoxLayout *parentLayout, const char *title) {
	QGroupBox *box = new QGroupBox(title);
	QVBoxLayout *vbox = new QVBoxLayout(box);
	parentLayout->addWidget(box);
	return vbox;
}

static QHBoxLayout *makeHFrame(QVBoxLayout *parentLayout, const char *title) {
	QGroupBox *box = new QGroupBox(title);
	QHBoxLayout *hbox = new QHBoxLayout(box);
	parentLayout->addWidget(box);
	return hbox;
}

PreferencesDialog::PreferencesDialog() {
	setWindowTitle(PROGRAM_NAME " - Preferences");

	QVBoxLayout *vbox;
	QHBoxLayout *hbox;
	QLabel *label;
	QPushButton *button;
	SmartComboBox *combo;
	SmartEditableComboBox *ecombo;
	SmartLineEdit *edit;
	SmartRadioButton *radio;
	SmartCheckBox *check;

	QVBoxLayout *bigvbox = new QVBoxLayout(this);
	bigvbox->setSizeConstraint(QLayout::SetFixedSize);

	// ---- general options ----
	hbox = makeHFrame(bigvbox, "Automatic recording");

	vbox = new QVBoxLayout;
	Preference &preference = preferences.get("autorecord.default");
	radio = new SmartRadioButton("&Automatically record calls", preference, "yes");
	vbox->addWidget(radio);
	radio = new SmartRadioButton("Ask every time", preference, "ask");
	vbox->addWidget(radio);
	radio = new SmartRadioButton("Do not automatically record calls", preference, "no");
	vbox->addWidget(radio);

	hbox->addLayout(vbox);

	button = new QPushButton("Per-caller preferences");
	connect(button, SIGNAL(clicked(bool)), this, SLOT(editPerCallerSettings()));
	button->setDisabled(true);
	hbox->addWidget(button, 0, Qt::AlignBottom);

	// ---- output file name ----
	vbox = makeVFrame(bigvbox, "Output file");

	label = new QLabel("&Save recorded calls here:");
	edit = new SmartLineEdit(preferences.get("output.path"));
	label->setBuddy(edit);
	vbox->addWidget(label);
	vbox->addWidget(edit);

	label = new QLabel("File &name:");
	ecombo = new SmartEditableComboBox(preferences.get("output.pattern"));
	label->setBuddy(ecombo);
	ecombo->addItem("%Y, %B/Skype call with &s, %A %B %d, %Y, %H:%M:%S");
	ecombo->addItem("Calls with &s/Skype call with &s, %A %B %d, %Y, %H:%M:%S");
	ecombo->addItem("Calls with &s/Skype call with &s, %A %B %d, %Y, %I:%M:%S%p");
	ecombo->addItem("Skype call with &s, %A, %B %d, %Y, %H:%M:%S");
	ecombo->addItem("%Y-%m-%d %H:%M:%S call with &s");
	ecombo->setupDone();
	vbox->addWidget(label);
	vbox->addWidget(ecombo);

	label = new QLabel("Example: blah blah");
	vbox->addWidget(label);

	// ---- output file format ----
	vbox = makeVFrame(bigvbox, "Output file &format");

	hbox = new QHBoxLayout;

	formatWidget = combo = new SmartComboBox(preferences.get("output.format"));
	combo->addItem("WAV PCM", "wav");
	combo->addItem("MP3", "mp3");
	combo->setupDone();
	connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(enableMp3Settings()));
	hbox->addWidget(combo);

	combo = new SmartComboBox(preferences.get("output.format.mp3.bitrate"));
	combo->addItem("8 kbps", 8);
	combo->addItem("16 kbps", 16);
	combo->addItem("24 kbps", 24);
	combo->addItem("32 kbps", 32);
	combo->addItem("40 kbps", 40);
	combo->addItem("48 kbps", 48);
	combo->addItem("56 kbps", 56);
	combo->addItem("64 kbps", 64);
	combo->addItem("80 kbps", 80);
	combo->addItem("96 kbps", 96);
	combo->addItem("112 kbps", 112);
	combo->addItem("128 kbps", 128);
	combo->addItem("144 kbps", 144);
	combo->addItem("160 kbps", 160);
	combo->setupDone();
	mp3Settings.append(combo);
	hbox->addWidget(combo);

	combo = new SmartComboBox(preferences.get("output.channelmode"));
	combo->addItem("Mix to mono", "mono");
	combo->addItem("Stereo, local left, remote right", "stereo");
	combo->addItem("Stereo, local right, remote left", "oerets");
	combo->setupDone();
	hbox->addWidget(combo);

	vbox->addLayout(hbox);

	check = new SmartCheckBox("Save call &information in MP3 files", preferences.get("output.savetags"));
	//mp3Settings.append(check);
	check->setEnabled(false);
	vbox->addWidget(check);

	// ---- buttons ----

	hbox = new QHBoxLayout;
	button = new QPushButton("&Close");
	button->setDefault(true);
	connect(button, SIGNAL(clicked(bool)), this, SLOT(accept()));
	hbox->addStretch();
	hbox->addWidget(button);
	bigvbox->addLayout(hbox);

	enableMp3Settings();
}

void PreferencesDialog::enableMp3Settings() {
	QVariant v = formatWidget->itemData(formatWidget->currentIndex());
	bool b = v == "mp3";
	for (int i = 0; i < mp3Settings.size(); i++)
		mp3Settings.at(i)->setEnabled(b);
}

void PreferencesDialog::editPerCallerSettings() {
	new PerCallerSettingsDialog(this);
}

// per caller preferences editor

PerCallerSettingsDialog::PerCallerSettingsDialog(QWidget *parent) : QDialog(parent) {
	setWindowTitle("Per-caller Preferences");
	setWindowModality(Qt::WindowModal);
	setAttribute(Qt::WA_DeleteOnClose);

	QVBoxLayout *vbox = new QVBoxLayout(this);

	list = new QListView;
	//QStringList l;
	//l.append("Skype name");
	//l.append("Record automatically");
	//table->setHorizontalHeaderLabels(l);
	//table->setSelectionMode(QAbstractItemView::ExtendedSelection);
	//table->setSelectionBehavior(QAbstractItemView::SelectRows);
	//table->setTextElideMode(Qt::ElideRight);
	//table->setSortingEnabled(true);
	//table->setGridStyle(Qt::NoPen);
	//table->setShowGrid(false);
	vbox->addWidget(list);

	QHBoxLayout *hbox = new QHBoxLayout;

	QPushButton *button = new QPushButton("&Add");
	connect(button, SIGNAL(clicked(bool)), this, SLOT(add()));
	hbox->addWidget(button);

	button = new QPushButton("&Remove");
	connect(button, SIGNAL(clicked(bool)), this, SLOT(remove()));
	hbox->addWidget(button);

	button = new QPushButton("&Close");
	button->setDefault(true);
	connect(button, SIGNAL(clicked(bool)), this, SLOT(accept()));
	hbox->addWidget(button);

	vbox->addLayout(hbox);

	// fill in data
/*
	QSet<QString> seen;
	int rows = 0;

	l = preferences.get("autorecord.yes").toList();
	for (int i = 0; i < l.count(); i++) {
		if (seen.contains(l.at(i)))
			continue;
		table->setRowCount(rows + 1);
		table->setItem(rows, 0, new QTableWidgetItem(l.at(i)));
		table->setItem(rows, 1, new QTableWidgetItem("yes"));
		rows++;
	}

	l = preferences.get("autorecord.ask").toList();
	for (int i = 0; i < l.count(); i++) {
		if (seen.contains(l.at(i)))
			continue;
		table->setRowCount(rows + 1);
		table->setItem(rows, 0, new QTableWidgetItem(l.at(i)));
		table->setItem(rows, 1, new QTableWidgetItem("ask"));
		rows++;
	}

	l = preferences.get("autorecord.no").toList();
	for (int i = 0; i < l.count(); i++) {
		if (seen.contains(l.at(i)))
			continue;
		table->setRowCount(rows + 1);
		table->setItem(rows, 0, new QTableWidgetItem(l.at(i)));
		table->setItem(rows, 1, new QTableWidgetItem("no"));
		rows++;
	}
*/
	//list->sortItems();

	//for (int i = 0; i < list->count(); i++) {
	//	QListWidgetItem *item = list->item(i);
	//	item->setFlags(item->flags() | Qt::ItemIsEditable);
	//}

	connect(this, SIGNAL(finished(int)), this, SLOT(saveSetting()));

	show();
}

void PerCallerSettingsDialog::add() {
	//QListWidgetItem *item = new QListWidgetItem("Enter skype name");
        //item->setFlags(item->flags() | Qt::ItemIsEditable);
	//int i = list->currentRow();
	//if (i < 0)
	//	list->addItem(item);
	//else
	//	list->insertItem(i + 1, item);

	//list->clearSelection();
	//list->setCurrentItem(item);
	//list->editItem(item);
}

void PerCallerSettingsDialog::remove() {
	//QList<QListWidgetItem *> sel = list->selectedItems();
	//while (!sel.isEmpty())
	//	delete sel.takeFirst();
}

void PerCallerSettingsDialog::saveSetting() {
	//table->sortItems(0);
	//QSet<QString> set;
	//for (int i = 0; i < list->count(); i++)
	//	set.insert(list->item(i)->text());
	//preference.set(set.toList());
}

// preferences

bool Preferences::load(const QString &filename) {
	clear();
	QFile file(filename);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		debug(QString("Can't open '%1' for loading preferences").arg(filename));
		return false;
	}
	char buf[65536];
	while (!file.atEnd()) {
		qint64 len = file.readLine(buf, sizeof(buf));
		if (len == -1)
			break;
		QString line(buf);
		line = line.trimmed();
		if (line.at(0) == '#')
			continue;
		int index = line.indexOf('=');
		if (index < 0)
			// TODO warn
			continue;
		get(line.left(index).trimmed()).set(line.mid(index + 1).trimmed());
        }
	debug(QString("Loaded %1 preferences from '%2'").arg(preferences.size()).arg(filename));
	return true;
}

bool Preferences::save(const QString &filename) {
	qSort(preferences);
	QFile file(filename);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		debug(QString("Can't open '%1' for saving preferences").arg(filename));
		return false;
	}
	QTextStream out(&file);
	for (int i = 0; i < preferences.size(); i++) {
		const Preference &p = preferences.at(i);
		out << p.name() << " = " << p.toString() << "\n";
	}
	debug(QString("Saved %1 preferences to '%2'").arg(preferences.size()).arg(filename));
	return true;
}

Preference &Preferences::get(const QString &name) {
	for (int i = 0; i < preferences.size(); i++)
		if (preferences.at(i).name() == name)
			return preferences[i];
	preferences.append(Preference(name));
	return preferences.last();
}

