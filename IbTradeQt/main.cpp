#include "capplicationcontroller.h"

#define QT_LOGGING_DEBUG 1
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    CApplicationController app;
    app.setUpApplication(a);
//    return app.run(argc, argv);
    return a.exec();
}
