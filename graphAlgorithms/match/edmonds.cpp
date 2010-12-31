#include "edmonds.h"

edmonds::edmonds(graph& G1, dlist& M1, int &size) : G(&G1), M(&M1) {
// Find a maximum size matching in the graph G and
// return it as a list of edges.
	vertex u, v; edge e;
	blossoms = new prtn(G->n());	// set per blossom
	augpath = new rlist(G->m()); 	// reversible list
	origin = new vertex[G->n()+1];	// original vertex for each blossom
	bridge = new evpair[G->n()+1];	// edge that formed a blossom
	state = new stype[G->n()+1];	// state used in path search
	pEdge = new edge[G->n()+1];	// edge to parent in tree
	mEdge = new edge[G->n()+1];	// incident matching edge (if any)
	mark = new bool[G->n()+1];	// mark bits used by nca
	
	// Create initial maximal (not maximum) matching
	M->clear(); size = 0;
	for (u = 1; u <= G->n(); u++) {
		mEdge[u] = Null; mark[u] = false;
	}
	for (e = 1; e <= G->m(); e++) {
		u = G->left(e); v = G->right(e);
		if (mEdge[u] == Null && mEdge[v] == Null) {
			mEdge[u] = mEdge[v] = e; (*M) &= e; size++;
		}
	}
		
	while((e = findpath()) != Null) { augment(e); size++; }

	delete blossoms; delete augpath; delete [] origin;
	delete [] bridge; delete [] pEdge; delete [] mEdge; delete[] mark;
}

void edmonds::augment(edge e) {
// Modify the matching by augmenting along the path defined by
// the list in the augpath data structure whose last element is e.

	vertex u, v; edge e1;
	while (true) {
		e1 = augpath->first(e);
		if (M->mbr(e1)) (*M) -= e1;
		else {
			(*M) &= e1;
			mEdge[G->left(e1)] = mEdge[G->right(e1)] = e1;
		}
		if (e == augpath->first(e)) break;
		e = augpath->pop(e);
	}
}

vertex edmonds::nca(vertex u, vertex v) {
// If u and v are in the same tree, return their nearest common
// ancestor in the current "condensed graph". To avoid excessive
// search time, search upwards from both vertices in parallel, using
// mark bits to identify the nca. Before returning, clear the mark
// bits by traversing the paths a second time. The mark bits are
// initialized in the constructor.
	vertex x,px,y,py,result;

	// first pass to find the nca
	x = u; px = (pEdge[x] != Null ? G->mate(x,pEdge[x]) : Null);
	y = v; py = (pEdge[y] != Null ? G->mate(y,pEdge[y]) : Null);
	while (true) {
		if (x == y) { result = x; break; }
		if (px == Null &&  py == Null) { result = Null; break; }
		if (px != Null) {
			if (mark[x]) { result = x; break; }
			mark[x] = true;
			x = origin[blossoms->find(px)];
			px = (pEdge[x] != Null ? G->mate(x,pEdge[x]) : Null);
		}
		if (py != Null) {
			if (mark[y]) { result = y; break; }
			mark[y] = true;
			y = origin[blossoms->find(py)];
			py = (pEdge[y] != Null ? G->mate(y,pEdge[y]) : Null);
		}
	}
	// second pass to clear mark bits
	x = u, y = v; 
	while (mark[x] || mark[y]) {
		mark[x] = mark[y] = false;
		px = (pEdge[x] != Null ? G->mate(x,pEdge[x]) : Null);
		py = (pEdge[y] != Null ? G->mate(y,pEdge[y]) : Null);
		x = (px == Null ? x : origin[blossoms->find(px)]);
		y = (py == Null ? y : origin[blossoms->find(py)]);
	}
	return result;
}

edge edmonds::path(vertex a, vertex b) {
// Find path joining a and b defined by parent pointers and bridges.
// A is assumed to be a descendant of b, and the path to b from a
// is assumed to pass through the matching edge incident to a.
// Return path in the augpath data structure.
	vertex pa, p2a, da; edge e, e1, e2;
	if (a == b) return Null;
	if (state[a] == even) {
		e1 = pEdge[a];  
		pa = G->mate(a,e1);
		if (pa == b) return e1;
		e2 = pEdge[pa]; 
		p2a = G->mate(pa,e2);
		e = augpath->join(e1,e2);
		e = augpath->join(e,path(p2a,b));
		return e;
	} else {
		e = bridge[a].e; da = bridge[a].v;
		e = augpath->join(augpath->reverse(path(da,a)),e);
		e = augpath->join(e,path(G->mate(da,e),b));
		return e;
	}
}

edge edmonds::findpath() {
// Search for an augmenting path in the graph G.
// On success, the path data structure will include a list
// that forms the augmenting path. In this case, the last
// edge in the list is returned as the function value.
// On failure, returns Null.
	vertex u,v,vp,w,wp,x,y; edge e, f;

	blossoms->clear();
	for (u = 1; u <= G->n(); u++) {
		state[u] = even; pEdge[u] = Null; origin[u] = u;
	}

	for (e = (*M)[1]; e != Null; e = M->suc(e)) {
		u = G->left(e); v = G->right(e);
		state[u] = state[v] = unreached;
	}
	list q(G->m()); // list of edges to be processed in main loop
	for (e = 1; e <= G->m(); e++) {
		if (state[G->left(e)] == even || state[G->right(e)] == even)
			q &= e;
	}

	while (!q.empty()) {
		e = q[1]; q <<= 1;
		v = G->left(e); vp = origin[blossoms->find(v)];
		if (state[vp] != even) {
			v = G->right(e); vp = origin[blossoms->find(v)];
		}
		w = G->mate(v,e); wp = origin[blossoms->find(w)];
		if (vp == wp) continue; // skip internal edges in a blossom
		if (state[wp] == unreached) {
			// w is not contained in a blossom and is matched
			// so extend tree and add newly eligible edges to q
			x = G->mate(w,mEdge[w]);
			state[w] = odd;  pEdge[w] = e;
			state[x] = even; pEdge[x] = mEdge[w];
			for (f = G->first(x); f != Null; f = G->next(x,f)) {
				if ((f != mEdge[x]) && !q.mbr(f)) q &= f;
			}
			continue;
		}
		u = nca(vp,wp);
		if (state[wp] == even && u == Null) {
			// vp, wp are different trees - construct path & return
			x = vp;
			while (pEdge[x] != Null) {
				x = origin[blossoms->find(G->mate(x,pEdge[x]))];
			}
			y = wp;
			while (pEdge[y] != Null) {
				y = origin[blossoms->find(G->mate(y,pEdge[y]))];
			}
			e = augpath->join(augpath->reverse(path(v,x)),e);
			e = augpath->join(e,path(w,y));
			return e;
		} else if (state[wp] == even) {
			// vp and wp are in same tree - collapse blossom
			x = vp;
			while (x != u) {
				blossoms->link( blossoms->find(x),
						blossoms->find(u));
				if (state[x] == odd) {
					bridge[x].e = e; bridge[x].v = v;
					for (f = G->first(x); f != G->term(x);
					     f = G->next(x,f))
						if (!q.mbr(f)) q &= f;
				}
				x = origin[blossoms->find(G->mate(x,pEdge[x]))];
			}
			y = wp;
			while (y != u) {
				blossoms->link( blossoms->find(y),
						blossoms->find(u));
				if (state[y] == odd) {
					bridge[y].e = e; bridge[y].v = w;
					for (f = G->first(y); f != G->term(y);
					     f = G->next(y,f))
						if (!q.mbr(f)) q &= f;
				}
				y = origin[blossoms->find(G->mate(y,pEdge[y]))];
			}
			origin[blossoms->find(u)] = u;
		} 
	}
	return Null;
}
