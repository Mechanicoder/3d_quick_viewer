// Mechanicoder
// 2022/06/04

#include "3d_quick_viewer.h"

#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    TdQuickViewer viewer;
    viewer.show();

    return app.exec();
}