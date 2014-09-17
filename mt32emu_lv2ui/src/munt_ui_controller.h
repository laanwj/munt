#ifndef MUNT_UI_CONTROLLER_H
#define MUNT_UI_CONTROLLER_H

#include <stdint.h>

class MuntUIController
{
public:
    virtual ~MuntUIController() {}

    virtual void test1() = 0;
    virtual void test2() = 0;
};

#endif
