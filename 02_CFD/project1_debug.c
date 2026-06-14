#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#define pi 3.1415926
int main()
{
    float dt,dx;
    int model,initial_model;
    printf("Please enter your time and space step length(divided by space):\n");
    scanf("%f %f",&dt,&dx);
    
    if(dt/dx>=1)
    {
        printf("unstable iteration");
        return 1;
    }
    
    printf("Choose your model to caculate:\n1.FTBS\n2.Lax\n3.Lax-Wendroff\n4.Warming-Beam\n5.Roe\n");
    scanf("%d",&model);
    printf("Choose your initial model to caculate:\n1.window\n2.complex window\n3.more complex window\n");
    scanf("%d",&initial_model);
    int xn=(int)(20/dx)+1;
    int tn=(int)(2/dt)+1;
    
    // 动态分配内存
    float **u = (float**)malloc((size_t)xn * (size_t)sizeof(float*));
    for(int i=0; i<xn; i++){
        u[i] = (float*)malloc((size_t)tn * (size_t)sizeof(float));
    }
    
    if(initial_model==1)
    {
         for(int i=0;i<xn;i++)
        {
        float x = -5 + (float)i * dx; //求解域的坐标(-5, 15)   int xn=(int)(20/dx)+1;
        if(x > -1.0 && x < 1.0)
        {
                u[i][0]=1;
        }
        else
        {
                u[i][0]=0;
        }

        }
    }  
    if(initial_model==2)
    {
         for(int i=0;i<xn;i++)
    {
        float x = -5 + (float)i * dx;
        if(x > 0 && x < 1.0)
        {
            u[i][0]= (float)exp((double)(-16.0*(x - 0.5)*(x - 0.5)))* sin(40.0*pi*x);
        }
        else
        {
            u[i][0]=0;
        }
    }
    }
    if(initial_model==3)
    {
         for(int i=0;i<xn;i++)
    {
        float x = -5 + (float)i * dx;
        if(x > 0 && x < 1.0)
        {
            u[i][0]=(float)(-64.0*x*x*x*(x-1.0)*(x-1.0)*(x-1.0)*exp((double)(-16.0*(x-0.5)*(x-0.5))));
        }
        else
        {
            u[i][0]=0;
        }
    }
    }
    //开始计算
    float c = dt/dx;
    
    if(model==1) // FTBS
    {
        for(int j=1;j<tn;j++){
        u[0][j] = 0.0f; // 边界条件，左端固定为0
            for(int i=1;i<xn;i++)
                u[i][j]=(1-c)*u[i][j-1] + c*u[i-1][j-1];
        }
    }
    else if(model==2) // Lax
    {
        for(int j=1;j<tn;j++)
            for(int i=1;i<xn-1;i++)
                u[i][j]=0.5*(u[i-1][j-1]+u[i+1][j-1]) - 0.5*c*(u[i+1][j-1]-u[i-1][j-1]);
    }
    else if(model==3) // Lax-Wendroff
    {
        for(int j=1;j<tn;j++)
            for(int i=1;i<xn-1;i++)
                u[i][j] = u[i][j-1] 
                        - 0.5*c*(u[i+1][j-1]-u[i-1][j-1]) 
                        + 0.5*c*c*(u[i+1][j-1]-2*u[i][j-1]+u[i-1][j-1]);
    }
    else if(model==4) // Warming-Beam
    {
        for(int j=1;j<tn;j++)
            for(int i=2;i<xn-1;i++)
                u[i][j] = u[i][j-1] 
                        - 0.5*c*(3*u[i][j-1]-4*u[i-1][j-1]+u[i-2][j-1]) 
                        + 0.5*c*c*(u[i-2][j-1]-2*u[i-1][j-1]+u[i][j-1]);
    }
    else if(model==5) // Roe
    {
        printf("updating...\n");
    }
    
    FILE *fp=fopen("project_1.dat","w");
    for(int i=0;i<xn;i++)
    {
        float x = -5.0 + i * dx;
        fprintf(fp, "%f %f\n", x, u[i][tn-1]);
    }
    fclose(fp);
    
    printf("Calculation completed. Result saved to project_1.dat\n");
    
    // 释放内存
    for(int i=0; i<xn; i++) free(u[i]);
    free(u);
    
    return 0;
}