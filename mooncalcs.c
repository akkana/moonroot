/*
 * mooncalcs.c: astronomical calculations for moonroot.
 *
 * Copyright 2004 by Akkana Peck.
 * You are free to use or modify this code under the Gnu Public License.
 */

#include "moonroot.h"

#include <math.h>
#include <time.h>
#include <stdio.h>

double UnixTimeToJulian(time_t sec);
int parseMonth(char* mon);

#define DEG2RAD (M_PI / 180)

/* convert degrees to a valid angle, mod 360: */
double angle(double deg)
{
    while (deg >= 360.)
        deg -= 360.;
    while (deg < 0.)
        deg += 360.;
    return deg * DEG2RAD;
}

/* Return the phase angle for the given date, in RADIANS.
 * Equation from Meeus eqn. 46.4.
 * Returns -1. for error.
 */
double GetPhaseAngle(time_t date)
{
    /* Time measured in Julian centuries from epoch J2000.0: */
    /* was 946728057... why? 946684800 should be right. */
    /* Right now T seems to be about one day too large, so subtract 1d */
    double T = (date - 946684800 - 86400) / 60. / 60. / 24. / 365.2425 / 100.;
    double T2 = T*T;
    double T3 = T2*T;
    double T4 = T3*T;

    /* Mean elongation of the moon: */
    double D = angle
        ( 297.8502042
          + 445267.1115168 * T
          - 0.0016300 * T2
          + T3 / 545868
          + T4 / 113065000 );
    /* Sun's mean anomaly: */
    double Msun = angle
        ( 357.5291092
          + 35999.0502909 * T
          - 0.0001536 * T2
          + T3 / 24490000 );
    /* Moon's mean anomaly: */
    double Mmoon = angle
        ( 134.9634114
          + 477198.8676313 * T
          + 0.0089970 * T2
          - T3 / 3536000
          + T4 / 14712000 );

    return ( angle ( 180 - (D/DEG2RAD)
                     - 6.289 * sin(Mmoon)
                     + 2.100 * sin(Msun)
                     - 1.274 * sin(2*D - Mmoon)
                     - 0.658 * sin(2*D)
                     - 0.214 * sin(2*Mmoon)
                     - 0.110 * sin(D) ) );
}

void PaintDarkside(int moonsize, time_t date)
{
    double phaseAngle = GetPhaseAngle(date);
    int moonradius = moonsize / 2;
    static GC darksideGC = 0;
    double positionAngle, cosTerm, rsquared;
    int whichQuarter;
    int j;

    /*printf("Phase angle: %lf\n", phaseAngle/DEG2RAD);*/

    /* The phase angle is the angle sun-moon-earth,
     * so 0 = full phase, 180 = new.
     * What we're actually interested in for drawing purposes
     * is the position angle of the sunrise terminator,
     * which runs the opposite direction from the phase angle,
     * so we have to convert.
     */
    positionAngle = M_PI - phaseAngle;
    if (positionAngle < 0.) positionAngle += 2.*M_PI;
    /*printf("Pos angle: %lf\n", positionAngle/DEG2RAD);*/

    cosTerm = cos(positionAngle);
    //if (cosTerm < 0) cosTerm = -cosTerm;
    rsquared = moonradius*moonradius;
    whichQuarter = ((int)(positionAngle*2./M_PI) + 4) % 4;

    if (darksideGC == 0) {
        /* dim the moon, rather than blackening it. */
        XGCValues gcv;
        gcv.foreground = WhitePixel(dpy, screen) / 3;
        gcv.function = GXand;
        darksideGC = XCreateGC(dpy, win, GCForeground | GCFunction, &gcv);
    }

    for (j=0; j <= moonradius; ++j)
    {
        double rrf = sqrt(rsquared - j*j);
        int rr = (int)(rrf + .5);
        int xx = (int)(rrf * cosTerm);
        int x1 = moonradius - (whichQuarter < 2 ? rr : xx);
        int w = rr + xx + 1;

        XFillRectangle(dpy, win, darksideGC, x1, moonradius-j, w, 1);
        XFillRectangle(dpy, win, darksideGC, x1, moonradius+j, w, 1);
    }
}

