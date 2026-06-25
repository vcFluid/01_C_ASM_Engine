#include   <stdio.h>
#include   <stdlib.h>

int main(void){
    double Re;
    scanf("%lf", &Re);
    if(Re < 2300){
        printf("Laminar Flow\n");
    }else if(Re >= 2300 && Re <= 4000){
        printf("Transitional Flow\n");
    }else{
        printf("Turbulent Flow\n");
    }
    return 0;
}