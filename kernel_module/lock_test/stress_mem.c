#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#define N 500000000
int list[N];

int main(int argc, char **argv)
{
  int i;
  srand(time(NULL));
  for(i=0; i<N; i++)
     list[i] = rand()%1000;
  return 0;
}
