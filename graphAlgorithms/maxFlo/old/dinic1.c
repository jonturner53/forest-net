// usage: dinic1
//
// Dinic reads a flograph from stdin, computes a maximum flow between
// vertices 1 and n using Dinic's algorithm and prints it out.
//
// This program is not bullet-proof. Caveat emptor.

#include "stdinc.h"
#include "basic/flograph.h"
#include "basic/list.h"
#include "dinic.h"

main() {
	flograph G;
	G.get(stdin); dinic(G); G.put(stdout);
}
