#include "MainWindow.h"
#include <QtPlugin>
#include <QApplication>
#include <QStyleFactory>
#include <QPluginLoader>
#include <QLibraryInfo>
#include <QFileInfo>
#include <QtMultimedia/QMediaServiceProviderPlugin>

#include "utils.h"

void makeFolders()
{
    gCfgDir = gExeDir + "/cfg";
    if (!QDir(gCfgDir).exists()) {
        qInfo() << "Creating configuration directory:" << gCfgDir;
        QDir().mkdir(gCfgDir);
    }
    qInfo() << "Configuration directory:" << gCfgDir;
}

int main(int argc, char *argv[])
{
    gExeDir = QDir::currentPath();

    // Makes sure the binary deployment can find the uvc engine
#define LIBRARY_PATH_WORKAROUND
#ifdef LIBRARY_PATH_WORKAROUND
    QStringList libraryPaths = QCoreApplication::libraryPaths();
    libraryPaths.append(gExeDir + "/plugins");
    QCoreApplication::setLibraryPaths(libraryPaths);
#else
    // WARNING: do not use addLibraryPath; it will preprend the path and here be dragons
    //QCoreApplication::addLibraryPath(gExeDir + "/plugins");
#endif

    qInstallMessageHandler(logMessages);
    gTimer.start();
    logInitBanner();

    if (!gTimer.isMonotonic())  // TODO: give the user the option to continue with an unreliable clock
        qFatal("Non monotonic reference clock.");

    makeFolders();

    QApplication a(argc, argv);
    a.setStyle(QStyleFactory::create("Fusion"));

//#define DARK_THEME
#ifdef DARK_THEME
     QPalette darkPalette;
     darkPalette.setColor(QPalette::Window, QColor(53,53,53));
     darkPalette.setColor(QPalette::WindowText, Qt::white);
     darkPalette.setColor(QPalette::Base, QColor(25,25,25));
     darkPalette.setColor(QPalette::AlternateBase, QColor(53,53,53));
     darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
     darkPalette.setColor(QPalette::ToolTipText, Qt::white);
     darkPalette.setColor(QPalette::Text, Qt::white);
     darkPalette.setColor(QPalette::Button, QColor(53,53,53));
     darkPalette.setColor(QPalette::ButtonText, Qt::white);
     darkPalette.setColor(QPalette::BrightText, Qt::red);
     darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
     darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
     darkPalette.setColor(QPalette::HighlightedText, Qt::black);
     a.setPalette(darkPalette);
     a.setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");
#endif

    MainWindow w;

    w.show();

    int ret = a.exec();

    logExitBanner();

    return ret;
}
