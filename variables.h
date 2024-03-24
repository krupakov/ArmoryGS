#ifndef VARIABLES_H
#define VARIABLES_H

#include <QString>

enum Shard {
    THREAD_OF_FATE     = 1,
    STAR_OF_FORTUNE    = 2201,
    GODS_LEGACY        = 101,
    ETERNAL_CALL       = 501,
    YOUNG_GUARD        = 601
};

enum Class {
    BARD       = 274401281,
    WARRIOR    = 61117,
    MAGE       = 64104,
    HEALER     = 61119,
    ENGINEER   = 739833443,
    PSIONICIST = 64105,
    SUMMONER   = 61121,
    SCOUT      = 61123,
    PALADIN    = 61120,
    WARDEN     = 64103,
    WARLOCK    = 740021330
};

struct Player {
    QString gearscore;
    QString guild;
    QString exactname;
};

#endif // VARIABLES_H
