#ifndef MUNT_UI_CONTROLLER_H
#define MUNT_UI_CONTROLLER_H

#include <stdint.h>

class MuntUIController
{
public:
    virtual ~MuntUIController() {}

    virtual void resetSynth() = 0;
    virtual void loadSyx(const char *filename) = 0;
};

#endif
