#include "randomtilingmode.h"

#include <QtGlobal>

RandomTilingMode::RandomTilingMode()
{
    inner_radius = 0.2;
    outer_radius = 0.4;
    common_phase = 0.0;
    for(int i = 0; i < 9 ; i++)
    {
        angles[i] = 0;
    }
}

void RandomTilingMode::randomize()
{
    static int seed = 312;
    qsrand(seed);
    // Fake seed
    seed = qrand() % 41211;
    for(int i = 0; i < 9 ; i++)
    {
        angles[i] = 2 * 3.1415269 * qrand() / (RAND_MAX + 0.0);
    }
}
