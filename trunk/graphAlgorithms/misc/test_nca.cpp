// usage:
//	test_nca
//
// Test_nca reads a tree from stdin and a list of vertex pairs,
// then invokes the nca routine to, computes the nearest common
// ancestor for each pair. Vertex 1 is treated as the tree root.
// On completion, test_nca prints the list of pairs and their nca.

#include "nca.h"

const int maxP = 100;

main()
{
	int np; vertex x, y;
	graph T; cin >> T;
	vertex_pair pairs[maxP];
	vertex ncav[maxP];

	np = 0;
	while (1) {
		misc::skipBlank(cin);
		if (!misc::verify(cin,'(') || !misc::getNode(cin,x,T.n()) ||
		    !misc::verify(cin,',') || !misc::getNode(cin,y,T.n()) ||
		    !misc::verify(cin,')') || np >= maxP)
			break;
		pairs[np].v1 = x; pairs[np++].v2 = y;
	}

	nca(T,1,pairs,np,ncav);
	for (int i = 0; i < np; i++) {
		cout << "nca("; misc::putNode(cout,pairs[i].v1,T.n());
		cout << ",";    misc::putNode(cout,pairs[i].v2,T.n());
		cout << ")=";   misc::putNode(cout,ncav[i],T.n());
		if (i%5 == 4) cout << endl;
		else cout << " ";
	}
	if (np%5 != 0) cout << endl;
}
