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
#include <QLabel>
#include <QPushButton>
#include <QListView>
#include <QPair>
#include <QFile>
#include <QSet>
#include <QTextStream>
#include <QtAlgorithms>
#include <QDir>
#include <QDateTime>
#include <QList>
#include <QFileIconProvider>
#include <QFileDialog>
#include <QTabWidget>
#include <ctime>

#include "preferences.h"
#include "smartwidgets.h"
#include "common.h"
#include "recorder.h"

Preferences preferences;

QString getOutputPath() {
	QString path = preferences.get(Pref::OutputPath).toString();
	if (path.startsWith("~/") || path == "~")
		path.replace(0, 1, QDir::homePath());
	else if (!path.startsWith('/'))
		path.prepend(QDir::currentPath() + '/');
	return path;
}

namespace {
QString escape(const QString &s) {
	QString out = s;
	out.replace('%', "%%");
	out.replace('&', "&&");
	return out;
}
}

QString getFileName(const QString &skypeName, const QString &displayName,
	const QString &mySkypeName, const QString &myDisplayName, const QDateTime &timestamp, const QString &pattern)
{
	QString fileName;
	if (pattern.isEmpty())
		fileName = preferences.get(Pref::OutputPattern).toString();
	else
		fileName = pattern;

	fileName.replace("&s", escape(skypeName));
	fileName.replace("&d", escape(displayName));
	fileName.replace("&t", escape(mySkypeName));
	fileName.replace("&e", escape(myDisplayName));
	fileName.replace("&&", "&");

	// TODO: uhm, does QT provide any time formatting the strftime() way?
	char *buf = new char[fileName.size() + 1024];
	time_t t = timestamp.toTime_t();
	struct tm *tm = std::localtime(&t);
	std::strftime(buf, fileName.size() + 1024, fileName.toUtf8().constData(), tm);
	fileName = buf;
	delete[] buf;

	return getOutputPath() + '/' + fileName;
}

// preferences dialog

static QVBoxLayout *makeVFrame(QVBoxLayout *parentLayout, const char *title) {
	QGroupBox *box = new QGroupBox(title);
	QVBoxLayout *vbox = new QVBoxLayout(box);
	parentLayout->addWidget(box);
	return vbox;
}

QWidget *PreferencesDialog::createRecordingTab() {
	QWidget *widget = new QWidget;
	QVBoxLayout *vbox = new QVBoxLayout(widget);

	Preference &preference = preferences.get(Pref::AutoRecordDefault);
	SmartRadioButton *radio = new SmartRadioButton("Automatically &record all calls", preference, "yes");
	vbox->addWidget(radio);
	radio = new SmartRadioButton("&Ask for every call", preference, "ask");
	vbox->addWidget(radio);
	radio = new SmartRadioButton("Do &not automatically record calls", preference, "no");
	vbox->addWidget(radio);

	QPushButton *button = new QPushButton("Edit &per caller preferences");
	connect(button, SIGNAL(clicked(bool)), this, SLOT(editPerCallerPreferences()));
	vbox->addWidget(button);

	SmartCheckBox *check = new SmartCheckBox("Show &balloon notification when recording starts", preferences.get(Pref::NotifyRecordingStart));
	vbox->addWidget(check);

	vbox->addStretch();
	return widget;
}

QWidget *PreferencesDialog::createPathTab() {
	QWidget *widget = new QWidget;
	QVBoxLayout *vbox = new QVBoxLayout(widget);

	QLabel *label = new QLabel("&Save recorded calls here:");
	outputPathEdit = new SmartLineEdit(preferences.get(Pref::OutputPath));
	label->setBuddy(outputPathEdit);
	connect(outputPathEdit, SIGNAL(textChanged(const QString &)), this, SLOT(updateAbsolutePathWarning(const QString &)));
	QPushButton *button = new QPushButton(QFileIconProvider().icon(QFileIconProvider::Folder), "Browse");
	connect(button, SIGNAL(clicked(bool)), this, SLOT(browseOutputPath()));
	QHBoxLayout *hbox = new QHBoxLayout;
	hbox->addWidget(outputPathEdit);
	hbox->addWidget(button);
	vbox->addWidget(label);
	vbox->addLayout(hbox);

	label = new QLabel("File &name:");
	patternWidget = new SmartEditableComboBox(preferences.get(Pref::OutputPattern));
	label->setBuddy(patternWidget);
	patternWidget->addItem("%Y-%m-%d %H:%M:%S Call with &s");
	patternWidget->addItem("Call with &s, %a %b %d %Y, %H:%M:%S");
	patternWidget->addItem("%Y, %B/Call with &s, %a %b %d %Y, %H:%M:%S");
	patternWidget->addItem("Calls with &s/Call with &s, %a %b %d %Y, %H:%M:%S");
	patternWidget->setupDone();
	connect(patternWidget, SIGNAL(editTextChanged(const QString &)), this, SLOT(updatePatternToolTip(const QString &)));
	vbox->addWidget(label);
	vbox->addWidget(patternWidget);

	vbox->addStretch();

	absolutePathWarningLabel = new QLabel("<b>Warning:</b> The path you have entered is not an absolute path!");
	vbox->addWidget(absolutePathWarningLabel);

	updatePatternToolTip("");
	updateAbsolutePathWarning(preferences.get(Pref::OutputPath).toString());

	return widget;
}

QWidget *PreferencesDialog::createFormatTab() {
	QWidget *widget = new QWidget;
	QVBoxLayout *vbox = new QVBoxLayout(widget);
	QGridLayout *grid = new QGridLayout;

	QLabel *label = new QLabel("Fil&e format:");
	formatWidget = new SmartComboBox(preferences.get(Pref::OutputFormat));
	label->setBuddy(formatWidget);
	formatWidget->addItem("WAV PCM", "wav");
	formatWidget->addItem("MP3", "mp3");
	formatWidget->addItem("Ogg Vorbis", "vorbis");
	formatWidget->setupDone();
	connect(formatWidget, SIGNAL(currentIndexChanged(int)), this, SLOT(updateFormatSettings()));
	grid->addWidget(label, 0, 0);
	grid->addWidget(formatWidget, 0, 1);

	label = new QLabel("MP3 &bitrate:");
	SmartComboBox *combo = new SmartComboBox(preferences.get(Pref::OutputFormatMp3Bitrate));
	label->setBuddy(combo);
	combo->addItem("8 kbps", 8);
	combo->addItem("16 kbps", 16);
	combo->addItem("24 kbps", 24);
	combo->addItem("32 kbps (recommended for mono)", 32);
	combo->addItem("40 kbps", 40);
	combo->addItem("48 kbps", 48);
	combo->addItem("56 kbps", 56);
	combo->addItem("64 kbps (recommended for stereo)", 64);
	combo->addItem("80 kbps", 80);
	combo->addItem("96 kbps", 96);
	combo->addItem("112 kbps", 112);
	combo->addItem("128 kbps", 128);
	combo->addItem("144 kbps", 144);
	combo->addItem("160 kbps", 160);
	combo->setupDone();
	mp3Settings.append(label);
	mp3Settings.append(combo);
	grid->addWidget(label, 1, 0);
	grid->addWidget(combo, 1, 1);

	label = new QLabel("Ogg Vorbis &quality:");
	combo = new SmartComboBox(preferences.get(Pref::OutputFormatVorbisQuality));
	label->setBuddy(combo);
	combo->addItem("Quality -1", -1);
	combo->addItem("Quality 0", 0);
	combo->addItem("Quality 1", 1);
	combo->addItem("Quality 2", 2);
	combo->addItem("Quality 3 (recommended)", 3);
	combo->addItem("Quality 4", 4);
	combo->addItem("Quality 5", 5);
	combo->addItem("Quality 6", 6);
	combo->addItem("Quality 7", 7);
	combo->addItem("Quality 8", 8);
	combo->addItem("Quality 9", 9);
	combo->addItem("Quality 10", 10);
	combo->setupDone();
	vorbisSettings.append(label);
	vorbisSettings.append(combo);
	grid->addWidget(label, 2, 0);
	grid->addWidget(combo, 2, 1);

	vbox->addLayout(grid);

	SmartCheckBox *check = new SmartCheckBox("Save to &stereo file", preferences.get(Pref::OutputStereo));
	connect(check, SIGNAL(clicked(bool)), this, SLOT(updateStereoSettings(bool)));
	vbox->addWidget(check);

	stereoMixLabel = new QLabel("");
	SmartSlider *slider = new SmartSlider(preferences.get(Pref::OutputStereoMix));
	stereoMixLabel->setBuddy(slider);
	slider->setOrientation(Qt::Horizontal);
	slider->setRange(0, 100);
	slider->setSingleStep(1);
	slider->setPageStep(10);
	slider->setTickPosition(QSlider::TicksBelow);
	slider->setTickInterval(10);
	slider->setupDone();
	connect(slider, SIGNAL(valueChanged(int)), this, SLOT(updateStereoMixLabel(int)));
	stereoSettings.append(stereoMixLabel);
	stereoSettings.append(slider);
	vbox->addWidget(stereoMixLabel);
	vbox->addWidget(slider);

	check = new SmartCheckBox("Save call &information in files", preferences.get(Pref::OutputSaveTags));
	mp3Settings.append(check);
	vorbisSettings.append(check);
	vbox->addWidget(check);

	vbox->addStretch();
	updateFormatSettings();
	updateStereoSettings(preferences.get(Pref::OutputStereo).toBool());
	updateStereoMixLabel(preferences.get(Pref::OutputStereoMix).toInt());
	return widget;
}

QWidget *PreferencesDialog::createMiscTab() {
	QWidget *widget = new QWidget;
	QVBoxLayout *vbox = new QVBoxLayout(widget);

	SmartCheckBox *check = new SmartCheckBox("&Display a small main window.  Enable this if your\n"
		"environment does not provide a system tray (needs restart)", preferences.get(Pref::GuiWindowed));
	vbox->addWidget(check);

	vbox->addStretch();
	return widget;
}

PreferencesDialog::PreferencesDialog() {
	setWindowTitle(PROGRAM_NAME " - Preferences");
	setAttribute(Qt::WA_DeleteOnClose);

	QVBoxLayout *vbox = new QVBoxLayout(this);
	vbox->setSizeConstraint(QLayout::SetFixedSize);

	QTabWidget *tabWidget = new QTabWidget;
	vbox->addWidget(tabWidget);

	tabWidget->addTab(createRecordingTab(), "Au&tomatic Recording");
	tabWidget->addTab(createPathTab(), "&File names");
	tabWidget->addTab(createFormatTab(), "File F&ormat");
	tabWidget->addTab(createMiscTab(), "&Misc");
	tabWidget->setUsesScrollButtons(false);

	QHBoxLayout *hbox = new QHBoxLayout;
	QPushButton *button = new QPushButton("&Close");
	button->setDefault(true);
	connect(button, SIGNAL(clicked(bool)), this, SLOT(accept()));
	hbox->addStretch();
	hbox->addWidget(button);
	vbox->addLayout(hbox);

	show();
}

void PreferencesDialog::updateFormatSettings() {
	QVariant v = formatWidget->itemData(formatWidget->currentIndex());
	// disable
	if (v != "mp3")
		for (int i = 0; i < mp3Settings.size(); i++)
			mp3Settings.at(i)->setEnabled(false);
	if (v != "vorbis")
		for (int i = 0; i < vorbisSettings.size(); i++)
			vorbisSettings.at(i)->setEnabled(false);
	// enable
	if (v == "mp3")
		for (int i = 0; i < mp3Settings.size(); i++)
			mp3Settings.at(i)->setEnabled(true);
	if (v == "vorbis")
		for (int i = 0; i < vorbisSettings.size(); i++)
			vorbisSettings.at(i)->setEnabled(true);
}

void PreferencesDialog::updateStereoSettings(bool stereo) {
	for (int i = 0; i < stereoSettings.size(); i++)
		if (stereo)
			stereoSettings.at(i)->setEnabled(true);
		else
			stereoSettings.at(i)->setEnabled(false);
}

void PreferencesDialog::updateStereoMixLabel(int value) {
	stereoMixLabel->setText(QString("Stereo mi&x: (left channel: local %1%, remote %2%)").arg(100 - value).arg(value));
}

void PreferencesDialog::editPerCallerPreferences() {
	perCallerDialog = new PerCallerPreferencesDialog(this);
}

void PreferencesDialog::browseOutputPath() {
	preferences.get(Pref::OutputPath).set(outputPathEdit->text());
	QFileDialog dialog(this, "Select output path", getOutputPath());
	dialog.setFileMode(QFileDialog::DirectoryOnly);
	if (!dialog.exec())
		return;
	QStringList list = dialog.selectedFiles();
	if (!list.size())
		return;
	QString path = list.at(0);
	QString home = QDir::homePath();
	if (path.startsWith(home + '/') || path == home)
		path.replace(0, home.size(), '~');
	if (path.endsWith('/') || path.endsWith('\\'))
		path.chop(1);
	outputPathEdit->setText(path);
}

void PreferencesDialog::updateAbsolutePathWarning(const QString &string) {
	if (string.startsWith('/') || string.startsWith("~/") || string == "~")
		absolutePathWarningLabel->hide();
	else
		absolutePathWarningLabel->show();
}

void PreferencesDialog::hideEvent(QHideEvent *event) {
	if (perCallerDialog)
		perCallerDialog->accept();

	QDialog::hideEvent(event);
}

void PreferencesDialog::updatePatternToolTip(const QString &pattern) {
	QString tip =
	"This pattern specifies how the file name for the recorded call is constructed.\n"
	"You can use the following directives:\n\n"

	#define X(a, b) "\t" a "\t" b "\n"
	X("&s"     , "The remote skype name or phone number")
	X("&d"     , "The remote display name")
	X("&t"     , "Your skype name")
	X("&e"     , "Your display name")
	X("&&"     , "Literal & character")
	X("%Y"     , "Year")
	X("%A / %a", "Full / abbreviated weekday name")
	X("%B / %b", "Full / abbreviated month name")
	X("%m"     , "Month as a number (01 - 12)")
	X("%d"     , "Day of the month (01 - 31)")
	X("%H"     , "Hour as a 24-hour clock (00 - 23)")
	X("%I"     , "Hour as a 12-hour clock (01 - 12)")
	X("%p"     , "AM or PM")
	X("%M"     , "Minutes (00 - 59)")
	X("%S"     , "Seconds (00 - 59)")
	X("%%"     , "Literal % character")
	#undef X
	"\t...and all other directives provided by strftime()\n\n"

	"With the current choice, the file name might look like this:\n";

	QString fn = getFileName("echo123", "Skype Test Service", "myskype", "My Full Name",
		QDateTime::currentDateTime(), pattern);
	tip += fn;
	if (fn.contains(':'))
		tip += "\n\nWARNING: Microsoft Windows does not allow colon characters (:) in file names.";
	patternWidget->setToolTip(tip);
}

void PreferencesDialog::closePerCallerDialog() {
	if (perCallerDialog)
		perCallerDialog->accept();
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

	QStringList list = preferences.get(Pref::AutoRecordYes).toList();
	for (int i = 0; i < list.count(); i++) {
		QString sn = list.at(i);
		if (seen.contains(sn))
			continue;
		seen.insert(sn);
		add(sn, 2, false);
	}

	list = preferences.get(Pref::AutoRecordAsk).toList();
	for (int i = 0; i < list.count(); i++) {
		QString sn = list.at(i);
		if (seen.contains(sn))
			continue;
		seen.insert(sn);
		add(sn, 1, false);
	}

	list = preferences.get(Pref::AutoRecordNo).toList();
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
	preferences.get(Pref::AutoRecordYes).set(yes);
	preferences.get(Pref::AutoRecordAsk).set(ask);
	preferences.get(Pref::AutoRecordNo).set(no);
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

// base preferences

BasePreferences::~BasePreferences() {
	clear();
}

bool BasePreferences::load(const QString &filename) {
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
	debug(QString("Loaded %1 preferences from '%2'").arg(prefs.size()).arg(filename));
	return true;
}

namespace {
bool comparePreferencePointers(const Preference *p1, const Preference *p2)
{
	return *p1 < *p2;
}
}

bool BasePreferences::save(const QString &filename) {
	qSort(prefs.begin(), prefs.end(), comparePreferencePointers);
	QFile file(filename);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		debug(QString("Can't open '%1' for saving preferences").arg(filename));
		return false;
	}
	QTextStream out(&file);
	for (int i = 0; i < prefs.size(); i++) {
		const Preference &p = *prefs.at(i);
		out << p.name() << " = " << p.toString() << "\n";
	}
	debug(QString("Saved %1 preferences to '%2'").arg(prefs.size()).arg(filename));
	return true;
}

Preference &BasePreferences::get(const QString &name) {
	for (int i = 0; i < prefs.size(); i++)
		if (prefs.at(i)->name() == name)
			return *prefs[i];
	prefs.append(new Preference(name));
	return *prefs.last();
}

void BasePreferences::remove(const QString &name) {
	for (int i = 0; i < prefs.size(); i++) {
		if (prefs.at(i)->name() == name) {
			delete prefs.takeAt(i);
			return;
		}
	}
}

bool BasePreferences::exists(const QString &name) const {
	for (int i = 0; i < prefs.size(); i++)
		if (prefs.at(i)->name() == name)
			return true;
	return false;
}

void BasePreferences::clear() {
	for (int i = 0; i < prefs.size(); i++)
		delete prefs.at(i);
	prefs.clear();
}

// preferences

void Preferences::setPerCallerPreference(const QString &sn, int mode) {
	// this would interfer with the per caller dialog
	recorderInstance->closePerCallerDialog();

	Preference &pYes = get(Pref::AutoRecordYes);
	Preference &pAsk = get(Pref::AutoRecordAsk);
	Preference &pNo = get(Pref::AutoRecordNo);

	pYes.listRemove(sn);
	pAsk.listRemove(sn);
	pNo.listRemove(sn);

	if (mode == 2)
		pYes.listAdd(sn);
	else if (mode == 1)
		pAsk.listAdd(sn);
	else if (mode == 0)
		pNo.listAdd(sn);
}

