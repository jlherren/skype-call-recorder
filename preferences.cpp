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
#include <QRadioButton>
#include <QButtonGroup>
#include <QLabel>
#include <QPushButton>
#include <QListView>
#include <QPair>
#include <QFile>
//#include <QSet>
#include <QTextStream>
#include <QtAlgorithms>
#include <QDir>

#include "preferences.h"
#include "smartwidgets.h"
#include "common.h"

Preferences preferences;

QString getOutputPath() {
	QString path = preferences.get("output.path").toString();
	path.replace('~', QDir::homePath());
	return path;
}

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

PreferencesDialog::PreferencesDialog() : perCallerDialog(NULL) {
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
	radio = new SmartRadioButton("Automatically &record calls", preference, "yes");
	vbox->addWidget(radio);
	radio = new SmartRadioButton("&Ask every time", preference, "ask");
	vbox->addWidget(radio);
	radio = new SmartRadioButton("Do &not automatically record calls", preference, "no");
	vbox->addWidget(radio);

	hbox->addLayout(vbox);

	button = new QPushButton("&Per caller preferences");
	connect(button, SIGNAL(clicked(bool)), this, SLOT(editPerCallerPreferences()));
	hbox->addWidget(button, 0, Qt::AlignBottom);

	// ---- output file name ----
	vbox = makeVFrame(bigvbox, "Output file");

	label = new QLabel("&Save recorded calls here:");
	edit = new SmartLineEdit(preferences.get("output.path"));
	label->setBuddy(edit);
	vbox->addWidget(label);
	vbox->addWidget(edit);

	label = new QLabel("&File name:");
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

void PreferencesDialog::editPerCallerPreferences() {
	perCallerDialog = new PerCallerPreferencesDialog(this);
	connect(perCallerDialog, SIGNAL(finished(int)), this, SLOT(perCallerFinished()));
}

void PreferencesDialog::perCallerFinished() {
	perCallerDialog = NULL;
}

void PreferencesDialog::hideEvent(QHideEvent *event) {
	if (perCallerDialog)
		perCallerDialog->accept();

	QDialog::hideEvent(event);
}

// per caller preferences editor

PerCallerPreferencesDialog::PerCallerPreferencesDialog(QWidget *parent) : QDialog(parent) {
	setWindowTitle("Per Caller Preferences");
	setWindowModality(Qt::WindowModal);
	setAttribute(Qt::WA_DeleteOnClose);

	model = new PerCallerModel(this);

	QHBoxLayout *bighbox = new QHBoxLayout(this);
	QVBoxLayout *vbox = new QVBoxLayout;

	listWidget = new QListView;
	listWidget->setModel(model);
	listWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
	listWidget->setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::DoubleClicked);
	connect(listWidget->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)), this, SLOT(selectionChanged()));
	vbox->addWidget(listWidget);

	QVBoxLayout *frame = makeVFrame(vbox, "Preference for selected Skype names:");
	radioYes = new QRadioButton("Automatically &record calls");
	radioAsk = new QRadioButton("&Ask every time");
	radioNo  = new QRadioButton("Do &not automatically record calls");
	connect(radioYes, SIGNAL(clicked(bool)), this, SLOT(radioChanged()));
	connect(radioAsk, SIGNAL(clicked(bool)), this, SLOT(radioChanged()));
	connect(radioNo,  SIGNAL(clicked(bool)), this, SLOT(radioChanged()));
	frame->addWidget(radioYes);
	frame->addWidget(radioAsk);
	frame->addWidget(radioNo);

	bighbox->addLayout(vbox);

	vbox = new QVBoxLayout;

	QPushButton *button = new QPushButton("A&dd");
	connect(button, SIGNAL(clicked(bool)), this, SLOT(add()));
	vbox->addWidget(button);

	button = new QPushButton("Re&move");
	connect(button, SIGNAL(clicked(bool)), this, SLOT(remove()));
	vbox->addWidget(button);

	vbox->addStretch();

	button = new QPushButton("&Close");
	button->setDefault(true);
	connect(button, SIGNAL(clicked(bool)), this, SLOT(accept()));
	vbox->addWidget(button);

	bighbox->addLayout(vbox);

	// fill in data

	QSet<QString> seen;

	QStringList list = preferences.get("autorecord.yes").toList();
	for (int i = 0; i < list.count(); i++) {
		QString sn = list.at(i);
		if (seen.contains(sn))
			continue;
		seen.insert(sn);
		add(sn, 2, false);
	}

	list = preferences.get("autorecord.ask").toList();
	for (int i = 0; i < list.count(); i++) {
		QString sn = list.at(i);
		if (seen.contains(sn))
			continue;
		seen.insert(sn);
		add(sn, 1, false);
	}

	list = preferences.get("autorecord.no").toList();
	for (int i = 0; i < list.count(); i++) {
		QString sn = list.at(i);
		if (seen.contains(sn))
			continue;
		seen.insert(sn);
		add(sn, 0, false);
	}

	model->sort();
	connect(this, SIGNAL(finished(int)), this, SLOT(save()));
	selectionChanged();
	show();
}

void PerCallerPreferencesDialog::add(const QString &name, int mode, bool edit) {
	int i = model->rowCount();
	model->insertRow(i);

	QModelIndex idx = model->index(i, 0);
	model->setData(idx, name, Qt::EditRole);
	model->setData(idx, mode, Qt::UserRole);

	if (edit) {
		listWidget->clearSelection();
		listWidget->setCurrentIndex(idx);
		listWidget->edit(idx);
	}
}

void PerCallerPreferencesDialog::remove() {
	QModelIndexList sel = listWidget->selectionModel()->selectedIndexes();
	qSort(sel);
	while (!sel.isEmpty())
		model->removeRow(sel.takeLast().row());
}

void PerCallerPreferencesDialog::selectionChanged() {
	QModelIndexList sel = listWidget->selectionModel()->selectedIndexes();
	bool notEmpty = !sel.isEmpty();
	int mode = -1;
	while (!sel.isEmpty()) {
		int m = model->data(sel.takeLast(), Qt::UserRole).toInt();
		if (mode == -1) {
			mode = m;
		} else if (mode != m) {
			mode = -1;
			break;
		}
	}
	if (mode == -1) {
		// Qt is a bit annoying about this: You can't deselect
		// everything unless you disable auto-exclusive mode
		radioYes->setAutoExclusive(false);
		radioAsk->setAutoExclusive(false);
		radioNo ->setAutoExclusive(false);
		radioYes->setChecked(false);
		radioAsk->setChecked(false);
		radioNo ->setChecked(false);
		radioYes->setAutoExclusive(true);
		radioAsk->setAutoExclusive(true);
		radioNo ->setAutoExclusive(true);
	} else if (mode == 0) {
		radioNo->setChecked(true);
	} else if (mode == 1) {
		radioAsk->setChecked(true);
	} else if (mode == 2) {
		radioYes->setChecked(true);
	}

	radioYes->setEnabled(notEmpty);
	radioAsk->setEnabled(notEmpty);
	radioNo ->setEnabled(notEmpty);
}

void PerCallerPreferencesDialog::radioChanged() {
	int mode = 1;
	if (radioYes->isChecked())
		mode = 2;
	else if (radioNo->isChecked())
		mode = 0;

	QModelIndexList sel = listWidget->selectionModel()->selectedIndexes();
	while (!sel.isEmpty())
		model->setData(sel.takeLast(), mode, Qt::UserRole);
}

void PerCallerPreferencesDialog::save() {
	model->sort();
	int n = model->rowCount();
	QStringList yes, ask, no;
	for (int i = 0; i < n; i++) {
		QModelIndex idx = model->index(i, 0);
		QString sn = model->data(idx, Qt::EditRole).toString();
		if (sn.isEmpty())
			continue;
		int mode = model->data(idx, Qt::UserRole).toInt();
		if (mode == 0)
			no.append(sn);
		else if (mode == 1)
			ask.append(sn);
		else if (mode == 2)
			yes.append(sn);
	}
	preferences.get("autorecord.yes").set(yes);
	preferences.get("autorecord.ask").set(ask);
	preferences.get("autorecord.no").set(no);
}

// per caller model

int PerCallerModel::rowCount(const QModelIndex &) const {
	return skypeNames.count();
}

namespace {
	const char *PerCallerModel_data_table[3] = {
		"Don't record", "Ask", "Automatic"
	};
}

QVariant PerCallerModel::data(const QModelIndex &index, int role) const {
	if (!index.isValid() || index.row() >= skypeNames.size())
		return QVariant();
	if (role == Qt::DisplayRole) {
		int i = index.row();
		return skypeNames.at(i) + " - " + PerCallerModel_data_table[modes.at(i)];
	}
	if (role == Qt::EditRole)
		return skypeNames.at(index.row());
	if (role == Qt::UserRole)
		return modes.at(index.row());
	return QVariant();
}

bool PerCallerModel::setData(const QModelIndex &index, const QVariant &value, int role) {
	if (!index.isValid() || index.row() >= skypeNames.size())
		return false;
	if (role == Qt::EditRole) {
		skypeNames[index.row()] = value.toString();
		emit dataChanged(index, index);
		return true;
	}
	if (role == Qt::UserRole) {
		modes[index.row()] = value.toInt();
		emit dataChanged(index, index);
		return true;
	}
	return false;
}

bool PerCallerModel::insertRows(int position, int rows, const QModelIndex &) {
	beginInsertRows(QModelIndex(), position, position + rows - 1);
	for (int i = 0; i < rows; i++) {
		skypeNames.insert(position, "");
		modes.insert(position, 1);
	}
	endInsertRows();
	return true;
}

bool PerCallerModel::removeRows(int position, int rows, const QModelIndex &) {
	beginRemoveRows(QModelIndex(), position, position + rows - 1);
	for (int i = 0; i < rows; i++) {
		skypeNames.removeAt(position);
		modes.removeAt(position);
	}
	endRemoveRows();
	return true;
}

void PerCallerModel::sort(int, Qt::SortOrder) {
	typedef QPair<QString, int> Pair;
	typedef QList<Pair> List;
	List list;
	for (int i = 0; i < skypeNames.size(); i++)
		list.append(Pair(skypeNames.at(i), modes.at(i)));
	qSort(list);
	for (int i = 0; i < skypeNames.size(); i++) {
		skypeNames[i] = list.at(i).first;
		modes[i] = list.at(i).second;
	}
	reset();
}

Qt::ItemFlags PerCallerModel::flags(const QModelIndex &index) const {
	Qt::ItemFlags flags = QAbstractListModel::flags(index);
	if (!index.isValid() || index.row() >= skypeNames.size())
		return flags;
	return flags | Qt::ItemIsEditable;
}

// preference

void Preference::listAdd(const QString &value) {
	QStringList list = toList();
	if (!list.contains(value)) {
		list.append(value);
		set(list);
	}
}

void Preference::listRemove(const QString &value) {
	QStringList list = toList();
	if (list.removeAll(value))
		set(list);
}

bool Preference::listContains(const QString &value) {
	QStringList list = toList();
	return list.contains(value);
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

