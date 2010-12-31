// usage: dinicDtrees1
//
// DinicDtrees reads a flograph from stdin, computes a maximum flow between
// vertices 1 and n using Dinic's algorithm and prints it out.
//
// This program is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "basic/flograph.h"
#include "advanced/pathset.h"
#include "advanced/dtrees.h"
#include "basic/list.h"
#include "dinicDtrees.h"

main() {
	flograph G;
	G.get(stdin); dinicDtrees(G); G.put(stdout);
}
