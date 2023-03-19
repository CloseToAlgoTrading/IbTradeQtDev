#ifndef CAPPLICATIONCONTROLLER_H
#define CAPPLICATIONCONTROLLER_H

#include "cpresenter.h"
#include "ibtradesystem.h"

class CApplicationController
{
public:
    CApplicationController();

    int run(int argc, char *argv[]);

private:
    IBTradeSystem *w;
    CPresenter *prst;
};

#endif // CAPPLICATIONCONTROLLER_H
