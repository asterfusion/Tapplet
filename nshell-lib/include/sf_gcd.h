#ifndef __sf_gcd_h__
#define __sf_gcd_h__

/*load_balances gcd  */
static inline int calc_gcd(int a, int b)                                                                                                                                                                                                                                      
{
    int c;

    c = a % b; 
    while (c)
    {    
        a = b; 
        b = c; 
        c = a % b; 
    }    
    return b;
}
/*
 *  * return gcd, and out max_weight
 *   */
static int weight_calc_max(int weight[], int num, int *max_weight)
{
    int i, gcd; 

    gcd         = weight[0];
    *max_weight = gcd; 
    for (i = 1; i < num; ++i) 
    {    
        gcd = calc_gcd(weight[i], gcd);

        if (weight[i] > *max_weight)
        {    
            *max_weight = weight[i];
        }    
    }    
    return gcd; 
}
/*********************/


#endif 
