#include <stdio.h>

/*
int recalc(unsigned int x) {
	unsigned int i, mask, mantissa;
	x = (x+9)/10; // round up to next larger multiple of 10
	x = (x < (0xf << 0xf) ? x : (0xf << 0xf));
	if (x == 0) x = 1;
	// find high order bit
	i = 18, mask = (1 << 18);
	while ((x & mask) == 0) {
		i--; mask >>= 1;
	}
	mantissa = x >> (i-3);
	if (x == (mantissa << (i-3))) {
		x = (mantissa << 4) | (i-3);
	} else if (mantissa < 0xf) {
		x = ((mantissa + 1) << 4) | (i-3);
	} else {
		x = (1 << 4) | (i+1);
	}
	return 10*(((x >> 4) & 0xf) << (x & 0xf));
}
*/

int recalc(unsigned int x) {
	unsigned int i, mask, mantissa;
	x = (x <= (0x1f << 0xf) ? x : (0x1f << 0xf));
	if (x < 16) x = 16;
	// find high order bit
	i = 19, mask = (1 << 19);
	while ((x & mask) == 0) {
		i--; mask >>= 1;
	}
	mantissa = x >> (i-4);
	if (x == (mantissa << (i-4))) {
		x = (mantissa << 4) | (i-4);
	} else if (mantissa < 0x1f) {
		x = ((mantissa + 1) << 4) | (i-4);
	} else {
		x = (1 << 8) | (i-3);
	}
	x &= 0xff; // trim off high bit of mantissa which is always 1

	x |= 0x100; // put back high bit of mantissa
	return ((x >> 4) & 0x1f) << (x & 0xf);
}

main (int argc, char *argv[]) {
	unsigned int i, n, x, z;
	sscanf(argv[1],"%d",&x);

	printf("%d:%x %d:%x\n",x,x,recalc(x),recalc(x));
}

