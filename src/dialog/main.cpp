#include <QApplication>
#include <QMessageBox>
#include <QString>
#include <QStringList>
#include <iostream>

#include "dialogwindow.h"

static QString parseArg(int argc, char* argv[], const QString& key) {
    for (int i = 1; i < argc - 1; ++i) {
        if (QString(argv[i]) == key) {
            return QString(argv[i + 1]);
        }
    }
    return {};
}

int main(int argc, char* argv[]) {
    QString filePath = parseArg(argc, argv, "--file");

    if (filePath.isEmpty()) {
        std::cerr << "用法: smartscreen-dialog --file <路径>" << std::endl;
        return 1;
    }

    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    DialogWindow window(filePath);
    window.show();

    // 等待窗口关闭
    QEventLoop loop;
    QObject::connect(&window, &QMainWindow::destroyed, &loop, &QEventLoop::quit);
    loop.exec();

    return window.userAccepted() ? 0 : 1;
}
