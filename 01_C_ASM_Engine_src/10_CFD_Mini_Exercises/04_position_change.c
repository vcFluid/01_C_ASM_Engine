#include <stdio.h>
#include <stdlib.h>

int main(void) {
    double x = 0.0;
    double u = 1.5;
    int N = 10;
    double dt = 1.0/N;
    for (int i = 0; i < N; i++) {
        x = x + u * dt;
            printf("Step = %d Final position = %.2f m\n", i+1, x);
    }
    return 0;
}