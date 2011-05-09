#include <math.h>



static double hann (int N,
                    int n);



double (*window) (int N, int n) = hann;



static double
hann (int N,
      int n)
{
  return 0.5 * (1 - cos (2 * M_PI * n / (N - 1)));
}
