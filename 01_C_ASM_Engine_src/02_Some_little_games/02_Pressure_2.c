#include <stdio.h>
#include <stdlib.h>

int main(void) {
    double pressure = 101325.0;
    double T = 300.0;
    double R = 287.0;
    double density = pressure / R / T;
    printf("density = %.2f kg/m^3\n", density);
    return 0;
}