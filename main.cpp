#include <QApplication>
#include "SimulatorWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    SimulatorWindow window;
    window.show();
    return app.exec();
}
