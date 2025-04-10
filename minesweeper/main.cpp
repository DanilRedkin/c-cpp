#include "mainwindow.h"
#include "minesweeper.h"

#include <QApplication>

int main(int argc, char *argv[]) {

    QApplication app(argc, argv);
    bool dbgMode = false;
    if (argc > 1 && QString(argv[1]) == "dbg") {
        dbgMode = true;
    }
    Minesweeper game(dbgMode);
    game.show();
    return app.exec();
}
