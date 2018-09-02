#include <stdio.h>
#include "rand.h"

#define N 624
#define M 397
#define MATRIX_A 0x9908b0dfUL
#define UPPER_MASK 0x80000000UL
#define LOWER_MASK 0x7fffffffUL

static DWORD mt[N];
static INT mti = N + 1;

VOID init_genrand(DWORD s)
{
    mt[0] = s & 0xffffffffUL;
    for (mti = 1; mti < N; mti++) {
        mt[mti] = (1812433253UL * (mt[mti-1] ^ (mt[mti-1] >> 30)) + mti); 
        mt[mti] &= 0xffffffffUL;
    }
}

DWORD genrand_int32(VOID)
{
    DWORD y;
    static DWORD mag01[2]={0x0UL, MATRIX_A};

    if (mti >= N) {
        int kk;

		if (mti == N+1){
			POINT P;
			GetCursorPos(&P);
            init_genrand(GetTickCount() * 12345 + P.x + P.y);
		}

        for (kk = 0; kk < N - M ; kk++) {
            y = (mt[kk] & UPPER_MASK) | (mt[kk + 1] & LOWER_MASK);
            mt[kk] = mt[kk + M] ^ (y >> 1) ^ mag01[y & 0x1UL];
        }
        for (; kk < N - 1; kk++) {
            y = (mt[kk] & UPPER_MASK) | (mt[kk + 1] & LOWER_MASK);
            mt[kk] = mt[kk + (M - N)] ^ (y >> 1) ^ mag01[y & 0x1UL];
        }
        y = (mt[N - 1] & UPPER_MASK) | (mt[0] & LOWER_MASK);
        mt[N - 1] = mt[M - 1] ^ (y >> 1) ^ mag01[y & 0x1UL];

        mti = 0;
    }
  
    y = mt[mti++];

    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680UL;
    y ^= (y << 15) & 0xefc60000UL;
    y ^= (y >> 18);

    return y;
}

DWORD rand_r(DWORD Min, DWORD Max){
	POINT P;
	GetCursorPos(&P);
	for(UINT i = 0; i < ((GetTickCount() >> 2) * P.x + P.y) % 3;  i++)
		genrand_int32();
	return (DWORD) (genrand_int32() % ((Max - Min) + ((Max >= 0xffffffff && !Min)?0:1))) + Min;
}