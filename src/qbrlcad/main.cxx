/*                        M A I N . C X X
 * BRL-CAD
 *
 * Copyright (c) 2014 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @file main.cxx
 *
 * Command line parsing and main application launching for qbrlcad
 *
 */

#include "bu/log.h"
#include "brlcad_version.h"

#include <iostream>

#if defined(__GNUC__) && (__GNUC__ == 4 && __GNUC_MINOR__ < 6) && !defined(__clang__)
#  pragma message "Disabling GCC float equality comparison warnings via pragma due to Qt headers..."
#endif
#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)) && !defined(__clang__)
#  pragma GCC diagnostic push
#endif
#if defined(__clang__)
#  pragma clang diagnostic push
#endif
#if defined(__GNUC__) && (__GNUC__ == 4 && __GNUC_MINOR__ >= 3) && !defined(__clang__)
#  pragma GCC diagnostic ignored "-Wfloat-equal"
#endif
#if defined(__clang__)
#  pragma clang diagnostic ignored "-Wfloat-equal"
#endif
#undef Success
#include <QCommandLineOption>
#undef Success
#include <QCommandLineParser>
#undef Success
#include <QApplication>
#undef Success
#include <QTreeView>
#undef Success
#include <QHeaderView>
#undef Success
#include <QObject>
#if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)) && !defined(__clang__)
#  pragma GCC diagnostic pop
#endif
#if defined(__clang__)
#  pragma clang diagnostic pop
#endif

#include "main_window.h"
#include "cadapp.h"
#include "cadconsole.h"
#include "cadtreemodel.h"

int main(int argc, char *argv[])
{
    CADApp app(argc, argv);
    BRLCAD_MainWindow mainWin;

    app.initialize();

    QCoreApplication::setApplicationName("BRL-CAD");
    QCoreApplication::setApplicationVersion(brlcad_version());

    QCommandLineParser parser;
    parser.setApplicationDescription("BRL-CAD - an Open Source Solid Modeling Computer Aided Design System");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption consoleOption(QStringList() << "c" << "console", "Run in command-line mode");
    parser.addOption(consoleOption);
    parser.process(app);

    const QStringList args = parser.positionalArguments();

    if (args.size() > 1) {
	bu_exit(1, "Error: Only one .g file at a time may be opened.");
    }

    if (args.size() == 1) {
	if (app.opendb(args.at(0))) {
	    fprintf(stderr, "%s%s%s\n", "Error: opening ", (const char *)args.at(0).toLocal8Bit(), " failed.");
	    exit(1);
	}
    }

    // TODO - this needs to be a setting that is saved and restored
    mainWin.resize(1100, 800);

    app.register_command(QString("ls"), ged_ls);
    app.register_command(QString("tops"), ged_tops);
    app.register_command(QString("search"), ged_search);

    if (parser.isSet(consoleOption)) {
	QString cmd("ls");
	QString result;
	app.exec_command(&cmd, &result);
	fprintf(stdout, "%s\n", (const char *)result.toLocal8Bit());
	bu_exit(1, "Console mode unimplemented\n");
    } else {
	/* This setup should probably go in main_window.cxx... */
	CADConsole *console = new CADConsole(mainWin.console_dock);
	mainWin.console_dock->setWidget(console);
	CADTreeModel *treemodel = new CADTreeModel();
	QTreeView *treeview = new QTreeView(mainWin.tree_dock);
	mainWin.tree_dock->setWidget(treeview);
	treeview->setModel(treemodel);
	treeview->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	treeview->header()->setStretchLastSection(false);
	QObject::connect(&app, SIGNAL(db_change()), treemodel, SLOT(refresh()));
	treemodel->populate(app.dbip());

	mainWin.show();
	return app.exec();
    }
}

/*
 * Local Variables:
 * mode: C++
 * tab-width: 8
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
