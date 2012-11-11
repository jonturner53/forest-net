/** @file CommonDefs.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include "CommonDefs.h"

bool Forest::readForestAdr(istream& in, fAdr_t& fa) {
// If next thing on the current line in a forest address,
// return it in fa and return true. Otherwise, return false.
// A negative value on the input stream in interpreted as
// a multicast address. Otherwise, we expect a unicast
// address in dotted decimal format. We require that
// either the zip code part is >0 or both parts are
// equal to zero. We allow 0.0 for null addresses and
// x.0 for unicast routes to foreign zip codes.
// The address is returned in host byte order.
	int b1, b2;
	
	if (!Util::readNum(in,b1)) return false;
	if (b1 < 0) { fa = b1; return true; }
	if (!Util::verify(in,'.') || !Util::readNum(in,b2))
		return false;
	if (b1 == 0 && b2 != 0) return false;
	if (b1 > 0xffff || b2 > 0xffff) return false;
	fa = Forest::forestAdr(b1,b2);
	return true;
}

string& Forest::nodeType2string(ntyp_t nt, string& s) {
	if (nt == CLIENT) s = "client";
	else if (nt == SERVER) s = "server";
	else if (nt == ROUTER) s = "router";
	else if (nt == CONTROLLER) s = "controller";
	else s = "unknown node type";
	return s;
}

ntyp_t Forest::getNodeType(string& s) {
	if (s.compare("client") == 0) return CLIENT;
	if (s.compare("server") == 0) return SERVER;
	if (s.compare("router") == 0) return ROUTER;
	if (s.compare("controller") == 0) return CONTROLLER;
	return UNDEF_NODE;
}

// MAH: for profiling

/*int64_t cycCnt1(void)
{
    uint32_t hi, lo; uint64_t v64;
    //  rdtsc: Loads the time-stamp counter value into %edx:%eax 
    asm volatile ("cpuid; rdtsc; movl %%edx, %0; movl %%eax, %1" // Read the counter
        : "=r" (hi), "=r" (lo)                  // output 
        :                                       // input (none) 
        : "%edx", "%eax");                      // Let gcc know whch registers are used
    v64 = ((uint64_t)hi << 32) + (uint64_t)lo;
    return (int64_t)v64;
}

unsigned long long int cycCnt2(void)
{
   unsigned long long int x;
   unsigned a, d;

   __asm__ volatile("rdtsc" : "=a" (a), "=d" (d));

   return ((unsigned long long)a) | (((unsigned long long)d) << 32);;
}*/

// MAH: for profiling
std::ostream& operator<<(std::ostream& os, const Timer& timer) {
    os << "Timer " << timer.name << ": " 
       << " count=" << timer.count
       << ", cycles=" << timer.cycles
       << ", average=" << timer.avg(); 
    return os;
}

