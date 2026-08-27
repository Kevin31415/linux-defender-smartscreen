#include <QApplication>
#include <QString>
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
    app.setQuitOnLastWindowClosed(true);

    DialogWindow dialog(filePath);
    dialog.exec();

    return dialog.userAccepted() ? 0 : 1;
}
