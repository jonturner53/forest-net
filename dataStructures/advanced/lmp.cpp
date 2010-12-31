#include "stdinc.h"
#include "lmp.h"

lmp::lmp(int N1) : N(N1) {
	vec = new nodeItem[N];	
	n = 0;
}

lmp::~lmp() { delete [] vec; }

int lmp::lookup(ipa_t adr) {
// Lookup adr in vec and return nexthop for longest match.
// Return Null if no match.
	int best = -1;
	for (int i = 0; i < n; i++) {
		int s = 32 - vec[i].len;
		if (adr >> s != vec[i].pref >> s) continue;
		if (best < 0 || vec[i].len > vec[best].len)
			best = i;
	}
	if (best < 0) return Null;
	return vec[best].nexthop;
}

bool lmp::insert(ipa_t prefix, int len, int next) {
// Add (prefix, next hop) pair to list of prefixes.
// If prefix already present, replaces the nexthop value.
	for (int i = 0; i < n; i++) {
		if (vec[i].len != len) continue;
		int s = 32 - len;
		if (prefix >> s != vec[i].pref >> s) continue;
		vec[i].nexthop = next;
		return true;
	}
	if (n == N) return false;
	vec[n].pref = prefix; vec[n].len = len; vec[n].nexthop = next;
	n++;
	return true;
}

void lmp::remove(ipa_t prefix, int len) {
// Remove the specified prefix.
	for (int i = 0; i < n; i++) {
		if (vec[i].len != len) continue;
		int s = 32 - len;
		if (prefix >> s != vec[i].pref >> s) continue;
		vec[i] = vec[--n];
		return;
	}
}

// Print all prefixes in the list.
void lmp::print() {
	for (int i = 0; i < n; i++) {
		printf("%d.%d.%d.%d/%d => %d\n",
			(vec[i].pref >> 24) & 0xff,
			(vec[i].pref >> 16) & 0xff,
			(vec[i].pref >> 8) & 0xff,
			 vec[i].pref & 0xff, vec[i].len, vec[i].nexthop);
	}
}
