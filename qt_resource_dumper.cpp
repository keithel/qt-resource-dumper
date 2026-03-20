// Copyright (C) 2026 Keith Kyzivat <https://github.com/keithel>
// SPDX-License-Identifier: GPL-3.0-only
//
// qt_resource_dumper.cpp
//
// LD_PRELOAD library that dumps the Qt resource filesystem of any Qt app.
//
// Usage:
//   LD_PRELOAD=/path/to/libqt_resource_dumper.so ./your_qt_app
//
// Environment variables:
//   QT_DUMP_RESOURCES=1          - Dump on first event loop tick (default;
//                                  catches ALL resources:
//                                  engine.loadFromModule() and friends run
//                                  synchronously before the loop starts, so all
//                                  dynamic QML module libraries are already
//                                  loaded by then)
//
//   QT_DUMP_RESOURCES=startup    - Dump at QCoreApplication construction
//                                  (early; only sees statically-linked
//                                  resources, misses dynamic QML module
//                                  libraries loaded by QQmlEngine)
//
//   QT_DUMP_RESOURCES=exit       - Dump at QCoreApplication::aboutToQuit
//                                  (requires a clean Qt shutdown; SIGKILL will
//                                  not trigger it)
//   QT_DUMP_RESOURCES_CONTENT=1  - Also print contents of text files
//                                  (.qml, .qmldir, .json, .js, .conf, .ini,
//                                  .qss, .xml, .txt)
//   QT_DUMP_RESOURCE_PATH=...    - Only dump this resource path (e.g.
//                                  ":/qt/qml/Com/Example" or "qrc:/qt/qml").
//                                  If not set, dumps entire fs (":/")

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QByteArray>
#include <QString>
#include <QTimer>
#include <cstdio>

static const QStringList sTextSuffixes = {
    ".qml", ".qmldir", ".js", ".json", ".qss", ".xml", ".txt", ".conf", ".ini"
};

static bool isTextFile(const QString &path)
{
    for (const QString &suffix : sTextSuffixes)
        if (path.endsWith(suffix, Qt::CaseInsensitive))
            return true;
    return false;
}

static void dumpResourceDir(const QString &path, int depth, bool dumpContent)
{
    QDir dir(path);
    const QFileInfoList entries = dir.entryInfoList(
        QDir::AllEntries | QDir::NoDotAndDotDot, QDir::DirsFirst | QDir::Name);

    for (const QFileInfo &fi : entries) {
        const QString fullPath = fi.absoluteFilePath();

        if (fi.isDir()) {
            fprintf(stderr, "%*s[DIR]  %s\n",
                    depth * 2, "", fullPath.toLocal8Bit().constData());
            dumpResourceDir(fullPath, depth + 1, dumpContent);
        } else {
            fprintf(stderr, "%*s[FILE] %s  (%lld bytes)\n",
                    depth * 2, "",
                    fullPath.toLocal8Bit().constData(),
                    (long long)fi.size());

            if (dumpContent && isTextFile(fullPath)) {
                QFile f(fullPath);
                if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    const QByteArray content = f.readAll();
                    // Indent each line of content
                    const QList<QByteArray> lines = content.split('\n');
                    for (const QByteArray &line : lines)
                        fprintf(stderr, "%*s  | %s\n",
                                depth * 2, "", line.constData());
                }
            }
        }
    }
}

static void doResourceDump(const char *when)
{
    const bool dumpContent =
        !qEnvironmentVariableIsEmpty("QT_DUMP_RESOURCES_CONTENT");

    // Get optional resource path filter
    QString dumpPath = ":/";  // default: entire filesystem
    const QByteArray pathEnv = qgetenv("QT_DUMP_RESOURCE_PATH");
    if (!pathEnv.isEmpty()) {
        dumpPath = QString::fromUtf8(pathEnv);
        if (dumpPath.startsWith("qrc:/")) {
            dumpPath = dumpPath.mid(3);  // "qrc:/foo" -> ":/foo"
        } else if (!dumpPath.startsWith(":/")) {
            // If it doesn't start with ":/" and not "qrc:/", assume it's a bare
            // path
            dumpPath = ":/" + dumpPath;
        }
    }

    fprintf(stderr, "\n");
    fprintf(stderr, "╔════════════════════════════════════════════════════════╗\n");
    fprintf(stderr, "║              Qt Resource Filesystem Dump               ║\n");
    fprintf(stderr, "║  Timing: %-46s║\n", when);
    fprintf(stderr, "║  Path: %-48s║\n", dumpPath.toLocal8Bit().constData());
    fprintf(stderr, "╚════════════════════════════════════════════════════════╝\n");

    dumpResourceDir(dumpPath, 0, dumpContent);

    fprintf(stderr, "──────────────────────────────────────────────────\n\n");
    fflush(stderr);
}

// Startup Callback hook
static void onQtResourceDumperStartup()
{
    if (qEnvironmentVariableIsEmpty("QT_DUMP_RESOURCES"))
        return;

    const QByteArray mode = qgetenv("QT_DUMP_RESOURCES");

    if (mode == "startup") {
        // Immediate dump: only sees resources from statically-linked libraries.
        // Dynamic QML module libs (loaded later by QQmlEngine) are not yet
        // registered.
        doResourceDump("QCoreApplication constructed (startup)");
    } else if (mode == "exit") {
        // Dump at aboutToQuit: requires clean Qt shutdown.
        QObject::connect(qApp, &QCoreApplication::aboutToQuit, []() {
            doResourceDump("QCoreApplication::aboutToQuit (exit)  ");
        });
    } else {
        // Default (QT_DUMP_RESOURCES=1 or any other value):
        // Dump on the first event loop tick via QTimer::singleShot(0).
        // engine.loadFromModule() / engine.load() are called synchronously in
        // main() BEFORE exec() starts the event loop, so all QML modules (and
        // their embedded resources) are fully loaded by the time this lambda
        // runs.
        QTimer::singleShot(0, qApp, []() {
            doResourceDump("first event loop tick (all QML modules loaded)");
        });
    }
}

// Q_COREAPP_STARTUP_FUNCTION registers a callback called immediately after
// QCoreApplication is constructed.
Q_COREAPP_STARTUP_FUNCTION(onQtResourceDumperStartup)
