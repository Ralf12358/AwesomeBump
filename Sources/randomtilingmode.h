#ifndef RANDOMTILINGMODE_H
#define RANDOMTILINGMODE_H

class RandomTilingMode
{
public:
    RandomTilingMode();
    // Generate random angles.
    void randomize();

    float angles[9];
    float common_phase;
    float inner_radius;
    float outer_radius;
};

#endif // RANDOMTILINGMODE_H
