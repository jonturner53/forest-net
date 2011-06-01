/** @file stdinc.h
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#ifndef STDINC_H
#define STDINC_H

#include <iostream>
#include <stdint.h>
#include <fstream>
#include <iomanip>
#include <cstdlib>
#include <cstring>
#include <string>
#include <cmath>
#include <cassert>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <memory.h>

using namespace std;

typedef int bit;

const int Null = 0;
const int BIGINT = 0x7fffffff;
const int EOS = '\0';

inline int max(int x, int y) { return x > y ? x : y; }
inline double max(double x, double y) { return x > y ? x : y; }
inline int min(int x, int y) { return x < y ? x : y; }
inline double min(double x, double y) { return x < y ? x : y; }
inline int abs(int x) { return x < 0 ? -x : x; }
inline double abs(double x) { return x < 0 ? -x : x; }

inline void warning(string msg) { cerr << "Warning: " << msg << endl; }
inline void fatal(string msg) {cerr << "Fatal: " << msg << endl; exit(1);}

double pow(double,double);
double log(double);

// long random(); 

double exp(double),log(double);

// Return a random number in [0,1] 
inline double randfrac() { return ((double) random())/BIGINT; }

// Return a random integer in the range [lo,hi].
// Not very good if range is larger than 10**7.
inline int randint(int lo, int hi) { return lo + (random() % (hi + 1 - lo)); }

// Return a random number from an exponential distribution with mean mu 
inline double randexp(double mu) { return -mu*log(randfrac()); }

// Return a random number from a geometric distribution with mean 1/p
inline int randgeo(double p) {
	return p > .999999 ? 1 : max(1,int(.999999 +log(randfrac())/log(1-p)));
}

// Return a random number from a truncated geometric distribution with mean 1/p
// and maximum value k.
inline int randTruncGeo(double p, int k) {
	double x = 1 - exp((k-1)*log(1-p));
	int r = int(.999999+log(randfrac()/x)/log(1-p));
	return p > .999999 ? 1 : max(1,min(r,k));
}

// Return a random number from a Pareto distribution with mean mu and shape s
inline double randpar(double mu, double s) {
	return mu*(1-1/s)/exp((1/s)*log(randfrac()));
}

#endif
