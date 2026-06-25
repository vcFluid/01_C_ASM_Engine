#include <stdio.h>
#include <stdlib.h>

int main(void){
    double pressure = 0.0;
    int N = 10;

    printf("input pressure in Pascal: ");
    scanf("%lf", &pressure);
    for(int i = 0; i < N; i++){
        pressure = pressure + 0.1;
    }
    printf("output pressure%.2f pa\n", pressure);
    system ("pause");
    return 0;
}