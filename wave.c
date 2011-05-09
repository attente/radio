#include "wave.h"

#include <stdio.h>
#include <math.h>



double (*wave) (double x) = sine;



static double mod (double x);



double
sine (double x)
{
  return sin (2 * M_PI * x);
}



double
square (double x)
{
  return mod (x) < 0.5 ? 1 : -1;
}



double
triangle (double x)
{
  double y = mod (x);

  return y < 0.5 ? 1 - 4 * fabs (y - 0.25) : -1 + 4 * fabs (y - 0.75);
}



double
sawtooth (double x)
{
  return 2 * mod (x + 0.5) - 1;
}



static double
mod (double x)
{
  double z;
  double y = modf (x, &z);

  return y + (x < 0 && y);
}
