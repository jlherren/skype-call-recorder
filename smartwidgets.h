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

#ifndef SMARTWIDGETS_H
#define SMARTWIDGETS_H

#include <QRadioButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QComboBox>
#include <QSlider>

#include "common.h"
#include "preferences.h"

// Smart widgets that automatically load preferences and set them

// Note: Smart widgets that need to be set up with addItem()/insertItem() can
// not automatically load the current preference.  you need to call setupDone()
// on them after you set them up, so they load the current preference and make the
// widget reflect that.

class SmartRadioButton : public QRadioButton {
	Q_OBJECT
public:
	SmartRadioButton(const QString &label, Preference &p, const char *v) : QRadioButton(label), preference(p), value(v) {
		setChecked(preference.toString() == value);
		connect(this, SIGNAL(toggled(bool)), this, SLOT(set(bool)));
	}

private slots:
	void set(bool active) { if (active) preference.set(value); }

private:
	Preference &preference;
	const char * const value;
};

// --------

class SmartCheckBox : public QCheckBox {
	Q_OBJECT
public:
	SmartCheckBox(const QString &label, Preference &p) : QCheckBox(label), preference(p) {
		setChecked(preference.toBool());
		connect(this, SIGNAL(toggled(bool)), this, SLOT(set(bool)));
	}

private slots:
	void set(bool active) { preference.set(active); }

private:
	Preference &preference;
};

// --------

class SmartLineEdit : public QLineEdit {
	Q_OBJECT
public:
	SmartLineEdit(Preference &p) : preference(p) {
		setText(preference.toString());
		connect(this, SIGNAL(editingFinished()), this, SLOT(set()));
	}

public slots:
	void setText(const QString &text) { QLineEdit::setText(text); set(); }

private slots:
	void set() { preference.set(text()); }

private:
	Preference &preference;
};

// --------

class SmartComboBox : public QComboBox {
	Q_OBJECT
public:
	SmartComboBox(Preference &p) : preference(p) { }
	void setupDone() {
		int i = findData(preference.toString());
		if (i < 0)
			// TODO: what to do here?
			return;
		setCurrentIndex(i);
		connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(set(int)));
	}

private slots:
	void set(int i) {
		QVariant v = itemData(i);
		if (v.type() == QVariant::Int)
			preference.set(v.toInt());
		else
			preference.set(v.toString());
	}

private:
	Preference &preference;
};

// --------

class SmartEditableComboBox : public QComboBox {
	Q_OBJECT
public:
	SmartEditableComboBox(Preference &p) : preference(p) {
		setEditable(true);
		lineEdit()->setText(preference.toString());
	}
	// qt 4.3.2 would overwrite any text set in the constructor, during the
	// first call to addItem
	void setupDone() {
		lineEdit()->setText(preference.toString());
		connect(lineEdit(), SIGNAL(editingFinished()), this, SLOT(set()));
	}

private slots:
	void set() { preference.set(lineEdit()->text()); }

private:
	Preference &preference;
};

// --------

class SmartSlider : public QSlider {
	Q_OBJECT
public:
	SmartSlider(Preference &p) : preference(p) {
	}
	void setupDone() {
		setValue(preference.toInt());
		connect(this, SIGNAL(valueChanged(int)), this, SLOT(set(int)));
	}

private slots:
	void set(int value) { preference.set(value); }

private:
	Preference &preference;
};

#endif

