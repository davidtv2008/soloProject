#include <QApplication>
#include "gamewindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //create my main game window
    GameWindow *gameWindow = new GameWindow();
    return a.exec();
}
