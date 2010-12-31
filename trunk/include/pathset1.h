// Header file for path set data structure used to implement
// dynamic trees. Maintains a set of paths on nodes numbered
// {1,...,n}. This version uses balanced binary search trees.

typedef int path;		// path
typedef int item;		// item in set
typedef int cost;
struct cpair { item s; cost c;};// pair returned by findpathcost
struct ppair { path s1, s2;};	// pair returned by split

class pathset {
	int	n;			// pathset defined on {1,...,n}
	int	*pprop;			// path property
	struct pnode {
		int lchild, rchild, parent, rank;
		cost deltacost, deltamin;
	} *vec;
	item	splay(item);
	void	splaystep(item);
	void	lrotate(item);
	void	rrotate(item);
public: 	pathset(int,int*);
		~pathset();
	path	findpath(item);		// return path containing item
	item	findtail(path);		// return tail of path
	cpair	findpathcost(path);	// return last min cost vertex and cost
	item	findtreeroot(item);	// return the root of the search tree
	void	addpathcost(path,cost);	// add to cost of every item in path
	path	join(path,item,path);	// join item with two paths
	ppair	split(item);		// split path on item and return set pair
	cost	c(item);		// cost of item - mainly for debugging
	void	print();		// print path set
	void	pprint(path,int);	// print single path
	void	tprint(path,int);	// print single path as tree
};
