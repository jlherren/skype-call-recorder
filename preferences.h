/*
	Skype Call Recorder
	Copyright 2008-2010, 2013, 2015 by jlh (jlh at gmx dot ch)

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

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <QDialog>
#include <QList>
#include <QString>
#include <QStringList>
#include <QAbstractListModel>
#include <QPointer>

#include "common.h"

class SmartComboBox;
class QWidget;
class QListView;
class PerCallerModel;
class PerCallerPreferencesDialog;
class QRadioButton;
class SmartEditableComboBox;
class SmartLineEdit;
class QDateTime;
class QLabel;

// A single preference, with a name and a value

class Preference {
public:
	Preference(const Preference &p) : m_name(p.m_name), isSet(p.isSet), value(p.value) { }
	Preference(const QString &n) : m_name(n), isSet(false) { }
	template <typename T> Preference(const QString &n, const T &t) : m_name(n), isSet(false) { set(t); }

	const QString &toString() const { return value; }
	int toInt() const { return value.toInt(); }
	bool toBool() const { return value.compare("yes", Qt::CaseInsensitive) == 0 || value.toInt(); }
	QStringList toList() const { return value.split(',', QString::SkipEmptyParts); }
	void listAdd(const QString &);
	void listRemove(const QString &);
	bool listContains(const QString &);

	void set(const char *v)        { isSet = true; value = v; }
	void set(const QString &v)     { isSet = true; value = v; }
	void set(int v)                { isSet = true; value.setNum(v); }
	void set(bool v)               { isSet = true; value = v ? "yes" : "no"; }
	void set(const QStringList &v) { isSet = true; value = v.join(","); }

	template <typename T> void setIfNotSet(const T &v) { if (!isSet) set(v); }

	const QString &name() const { return m_name; }

	bool operator<(const Preference &rhs) const {
		return m_name < rhs.m_name;
	}

private:
	QString m_name;
	bool isSet;
	QString value;

private:
	// disable assignment.  we want preference names to be immutable.
	Preference &operator=(const Preference &);
};

// A collection of preferences that can be loaded/saved

class BasePreferences {
public:
	BasePreferences() { };
	~BasePreferences();

	bool load(const QString &);
	bool save(const QString &);

	Preference &get(const QString &);
	// Warning: remove() must only be used if nobody has a pointer to the
	// preference, like for example smart widgets
	void remove(const QString &);
	bool exists(const QString &) const;
	void clear();

	int count() const { return prefs.size(); }

private:
	// this is a list of pointers, so we can control the life time of each
	// Preference.  we want references to them to be valid forever.  Only
	// QLinkedList could give that guarantee, but it's unpractical for
	// sorting.  QList<Preference> would technically be implemented as an
	// array of pointers too, but its sorting semantics with regard to
	// references are not the one we want.
	QList<Preference *> prefs;

	DISABLE_COPY_AND_ASSIGNMENT(BasePreferences);
};

// preferences with some utils

class Preferences : public BasePreferences {
public:
	Preferences() { };

	void setPerCallerPreference(const QString &, int);

	DISABLE_COPY_AND_ASSIGNMENT(Preferences);
};

// The preferences dialog

class PreferencesDialog : public QDialog {
	Q_OBJECT
public:
	PreferencesDialog();
	void closePerCallerDialog();

protected:
	void hideEvent(QHideEvent *);

private slots:
	void updateFormatSettings();
	void editPerCallerPreferences();
	void updatePatternToolTip(const QString &);
	void updateStereoSettings(bool);
	void updateStereoMixLabel(int);
	void browseOutputPath();
	void updateAbsolutePathWarning(const QString &);

private:
	QWidget *createRecordingTab();
	QWidget *createPathTab();
	QWidget *createFormatTab();
	QWidget *createMiscTab();

private:
	QList<QWidget *> mp3Settings;
	QList<QWidget *> vorbisSettings;
	QList<QWidget *> stereoSettings;
	SmartLineEdit *outputPathEdit;
	SmartComboBox *formatWidget;
	QPointer<PerCallerPreferencesDialog> perCallerDialog;
	SmartEditableComboBox *patternWidget;
	QLabel *stereoMixLabel;
	QLabel *absolutePathWarningLabel;

	DISABLE_COPY_AND_ASSIGNMENT(PreferencesDialog);
};

// The per caller editor dialog

class PerCallerPreferencesDialog : public QDialog {
	Q_OBJECT
public:
	PerCallerPreferencesDialog(QWidget *);

private slots:
	void add(const QString & = QString(), int = 1, bool = true);
	void remove();
	void selectionChanged();
	void radioChanged();
	void save();

private:
	QListView *listWidget;
	PerCallerModel *model;
	QRadioButton *radioYes;
	QRadioButton *radioAsk;
	QRadioButton *radioNo;

	DISABLE_COPY_AND_ASSIGNMENT(PerCallerPreferencesDialog);
};

// per caller model

class PerCallerModel : public QAbstractListModel {
	Q_OBJECT
public:
	PerCallerModel(QObject *parent) : QAbstractListModel(parent) { }
	int rowCount(const QModelIndex & = QModelIndex()) const;
	QVariant data(const QModelIndex &, int) const;
	bool setData(const QModelIndex &, const QVariant &, int = Qt::EditRole);
	bool insertRows(int, int, const QModelIndex &);
	bool removeRows(int, int, const QModelIndex &);
	void sort(int = 0, Qt::SortOrder = Qt::AscendingOrder);
	Qt::ItemFlags flags(const QModelIndex &) const;

private:
	QStringList skypeNames;
	QList<int> modes;
};

// the only instance of Preferences

extern Preferences preferences;
extern QString getOutputPath();
extern QString getFileName(const QString &, const QString &, const QString &,
	const QString &, const QDateTime &, const QString & = QString());

// preference constants

#define X(name, string) const char * const name = #string;

namespace Pref {

X(AutoRecordDefault,           autorecord.default)
X(AutoRecordAsk,               autorecord.ask)
X(AutoRecordYes,               autorecord.yes)
X(AutoRecordNo,                autorecord.no)
X(OutputPath,                  output.path)
X(OutputPattern,               output.pattern)
X(OutputFormat,                output.format)
X(OutputFormatMp3Bitrate,      output.format.mp3.bitrate)
X(OutputFormatVorbisQuality,   output.format.vorbis.quality)
X(OutputStereo,                output.stereo)
X(OutputStereoMix,             output.stereo.mix)
X(OutputSaveTags,              output.savetags)
X(SuppressLegalInformation,    suppress.legalinformation)
X(SuppressFirstRunInformation, suppress.firstruninformation)
X(PreferencesVersion,          preferences.version)
X(NotifyRecordingStart,        notify.recordingstart)
X(GuiWindowed,                 gui.windowed)
X(DebugWriteSyncFile,          debug.writesyncfile)

}

#undef X

#endif

