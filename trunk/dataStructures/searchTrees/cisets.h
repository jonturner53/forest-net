// Header file for data structure representing a collection of items
// and which allows whole intervals to be inserted and removed in one step.
// Each set is maintained in sorted order using self-adjusting
// binary search trees.

typedef int iset;		// set in collection
typedef int item;		// item in set
typedef int keytyp;
struct ispair { iset s1,s2; };	// pair of sets, returned by split
struct interval { int l,h; };

class cisets {
	int	n;			// number of disjoint intervals.
	int	free;			// pointer to first free node
	struct spnode {
		iset lchild, rchild, parent;
		keytyp loval, hival;
	} *vec;
	item	splay(item);
	void	splaystep(item);
	void	lrotate(item);
	void	rrotate(item);
	iset	max(iset);		// splay at max interval in set
	iset	min(iset);		// splay at min interval in set
	iset	find(int,iset);		// splay at node containing int
	void	recover(iset);		// recover the nodes in an iset
public: 	cisets(int=100);
		~cisets();
	iset	insert(int,int,iset);	// insert interval into set
	iset	remove(int,int,iset);	// remove interval from set
	interval search(int,iset);	// return largest interval containing
					//  given integer
	iset	join(iset,int,int,iset);// join two sets at interval
	ispair	split(int,iset);	// split set on int and return set pair
	void	print();		// print collection of sets
	void	sprint(iset);		// print single set
	void	tprint(iset,int);	// print single set as tree
};
