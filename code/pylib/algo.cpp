#include "algo.h"

#include <stdlib.h>

#define RGB(r,g,b) ((((int)b)<<16)|(((int)g)<<8)|((int)r))

// sobel map for the x axis
const double _SOBEL_Gx[3][3] = { {-1.0,+0.0,+1.0},
                                 {-2.0,+0.0,+2.0},
                                 {-1.0,+0.0,+1.0}
                               };
// sobel map for the y axis
const double _SOBEL_Gy[3][3] = { {+1.0,+2.0,+1.0},
                                 {+0.0,+0.0,+0.0},
                                 {-1.0,-2.0,-1.0}
                               };

double get_sobel_gradient ( unsigned char const * pimg, int width, int height, int x, int y )
{
    double sobel_gradient_x = 0,
           sobel_gradient_y = 0;
    int mx = 0,
        my = 0,
        sx = 0,
        sy = 0;

    for( mx = x; mx < x + 3 ; mx++ )
    {
        sy = 0;
        for( my = y; my < y + 3 ; my++ )
        {
            if( mx < width && my < height )
            {
                int r,g,b,idx;
                idx = (mx + width * my) * 3;
                r = pimg[ idx + 0 ];
                g = pimg[ idx + 1 ];
                b = pimg[ idx + 2 ];
                sobel_gradient_x += RGB(r,g,b) * _SOBEL_Gx[sx][sy];
                sobel_gradient_y += RGB(r,g,b) * _SOBEL_Gy[sx][sy];
            }
            sy++ ; 
        }
        sx++ ; 
    }

    return abs(sobel_gradient_x) + abs(sobel_gradient_y);
}
