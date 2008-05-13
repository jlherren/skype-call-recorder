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
#include <QLabel>
#include <QPushButton>

#include "about.h"
#include "common.h"

AboutDialog::AboutDialog() {
	setWindowTitle(PROGRAM_NAME " - About");
	setAttribute(Qt::WA_DeleteOnClose);

	QVBoxLayout *vbox = new QVBoxLayout(this);
	vbox->setSizeConstraint(QLayout::SetFixedSize);

	QString str =
		"<p><font face='Arial' size='20'><b>Skype Call Recorder</b></font></p>"

		"<p>Copyright &copy; 2008 jlh (<a href='mailto:jlh@gmx.ch'>jlh@gmx.ch</a>)<br>"
		"Website: <a href='http://atdot.ch/scr/'>http://atdot.ch/scr/</a></p>"
		"<hr>"
		"<p>This program is free software; you can redistribute it and/or modify it<br>"
		"under the terms of the GNU General Public License as published by the<br>"
		"<a href='http://www.fsf.org/'>Free Software Foundation</a>; either "
		"<a href='http://www.gnu.org/licenses/old-licenses/gpl-2.0.html'>version 2 "
		"of the License</a>, <a href='http://www.gnu.org/licenses/gpl.html'>version 3 of<br>"
		"the License</a>, or (at your option) any later version.</p>"

		"<p>This program is distributed in the hope that it will be useful, but WITHOUT<br>"
		"ANY WARRANTY; without even the implied warranty of MERCHANTABILITY<br>"
		"or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public<br>"
		"License for more details.</p>"
		"<hr>"
		"<p><small>Git commit: %1<br>"
		"Build date: %2</small></p>";
	str = str.arg(recorderCommit, recorderDate);
	QLabel *label = new QLabel(str);
	label->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
	label->setFont(QFont("Arial", 9));
	label->setTextFormat(Qt::RichText);
	label->setOpenExternalLinks(true);
	vbox->addWidget(label);

	QPushButton *button = new QPushButton("&Close");
	connect(button, SIGNAL(clicked()), this, SLOT(close()));
	vbox->addWidget(button);

	show();
}

