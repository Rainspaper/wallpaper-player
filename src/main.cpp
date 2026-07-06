#include <QApplication>
#include <QIcon>
#include "mainwindow.h"

#ifdef __MINGW32__
#include <crtdbg.h>
extern "C" { extern int __argc; __declspec(dllexport) int *__imp___argc = &__argc; }
#endif

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/ico1.ico"));

    MainWindow w;
    w.setWindowIcon(QIcon(":/ico1.ico"));
    w.showMaximized();

    return app.exec();
}
