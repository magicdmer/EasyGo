#ifndef COMMONTYPES_H
#define COMMONTYPES_H

#include <QString>
#include "Plugin.h"

enum InputMode {
    NormalInput,
    MenuInput,
    EnterInput,
    DropInput
};

struct InputState {
    InputMode primaryMode;
    bool isSecondary;
    QString additionalData;
    Query additionalQuery;
};

#endif // COMMONTYPES_H
