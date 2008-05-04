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

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <QDialog>
#include <QList>
#include <QString>
#include <QStringList>
#include <QAbstractListModel>

class SmartComboBox;
class QListView;
class PerCallerModel;
class QRadioButton;

// A single preference, with a name and a value

class Preference {
public:
	Preference(const Preference &p) : m_name(p.m_name), isSet(p.isSet), value(p.value) { }
	Preference &operator=(const Preference &p) { m_name = p.m_name; isSet = p.isSet; value = p.value; return *this; }
	Preference(const QString &n) : m_name(n), isSet(false) { }
	template <typename T> Preference(const QString &n, const T &t) : m_name(n), isSet(false) { set(t); }

	const QString &toString() const { return value; }
	int toInt() const { return value.toInt(); }
	bool toBool() const { return value.compare("yes", Qt::CaseInsensitive) == 0 || value.toInt(); }
	QStringList toList() const { return value.split(',', QString::SkipEmptyParts); }

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
};

// A collection of preferences that can be loaded/saved

class Preferences {
public:
	bool load(const QString &);
	bool save(const QString &);

	Preference &get(const QString &);
	void clear() { preferences.clear(); }

	int count() const { return preferences.size(); }

private:
	QList<Preference> preferences;
};

// The preferences dialog

class PreferencesDialog : public QDialog {
	Q_OBJECT
public:
	PreferencesDialog();

private slots:
	void enableMp3Settings();
	void editPerCallerPreferences();

private:
	QList<QWidget *> mp3Settings;
	SmartComboBox *formatWidget;

private:
	// disabled
	PreferencesDialog(const PreferencesDialog &);
	PreferencesDialog operator=(const PreferencesDialog &);
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

#endif

