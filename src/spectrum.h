#pragma once

/* EQ CONFIG */
#define EQ_BANDS    (14)
#define NOISECOMP   (70)
#define GAIN        (1.0)
#define EQ_DELTA    (5.0)


void SPECTRUM_init();
void SPECTRUM_sampleSpectrum();
int SPECTRUM_getBandValue(int band);
