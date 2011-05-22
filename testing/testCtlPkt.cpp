#include "CtlPkt.h"

buffer_t b; CtlPkt *pp1; CtlPkt *pp2;

void setup(CpTypeIndex cpt, CpRrType rrt, int32_t seq) {
	pp1->reset(b);
	pp1->cpType = cpt; pp1->rrType = rrt; pp1->seqNum = seq;
}

void doit() {
	pp1->print(cout); 
	int len = pp1->pack();
	if (len == 0) cout << "packing error\n";
	pp2->reset(b);
	if (!pp2->unpack(len)) cout << "unpacking error\n";
	pp2->print(cout);
	cout << endl;
}

int main() {
	CtlPkt p1(b), p2(b); pp1 = & p1; pp2 = &p2;

	setup(CLIENT_ADD_COMTREE,REQUEST,123);
	doit();

	setup(CLIENT_ADD_COMTREE,POS_REPLY,123);
	p1.setAttrVal(COMTREE_NUM,456);
	doit();

	setup(CLIENT_ADD_COMTREE,NEG_REPLY,123);
	p1.errMsg = "oops!";
	doit();
	cout << "===================\n";

	setup(CLIENT_DROP_COMTREE,REQUEST,123);
	p1.setAttrVal(COMTREE_NUM,456);
	doit();

	setup(CLIENT_DROP_COMTREE,POS_REPLY,123);
	doit();

	setup(CLIENT_DROP_COMTREE,NEG_REPLY,123);
	p1.errMsg = "oops!";
	doit();
	cout << "===================\n";

	setup(CLIENT_JOIN_COMTREE,REQUEST,123);
	p1.setAttrVal(COMTREE_NUM,456);
	doit();

	setup(CLIENT_JOIN_COMTREE,POS_REPLY,123);
	doit();

	setup(CLIENT_JOIN_COMTREE,NEG_REPLY,123);
	p1.errMsg = "oops!";
	doit();
	cout << "===================\n";

	setup(CLIENT_LEAVE_COMTREE,REQUEST,123);
	p1.setAttrVal(COMTREE_NUM,456);
	doit();

	setup(CLIENT_LEAVE_COMTREE,POS_REPLY,123);
	doit();

	setup(CLIENT_LEAVE_COMTREE,NEG_REPLY,123);
	p1.errMsg = "oops!";
	doit();
	cout << "===================\n";

	setup(CLIENT_RESIZE_COMTREE,REQUEST,123);
	p1.setAttrVal(COMTREE_NUM,456);
	doit();

	setup(CLIENT_RESIZE_COMTREE,POS_REPLY,123);
	doit();

	setup(CLIENT_RESIZE_COMTREE,NEG_REPLY,123);
	p1.errMsg = "oops!";
	doit();
	cout << "===================\n";

	setup(CLIENT_GET_COMTREE,REQUEST,123);
	p1.setAttrVal(COMTREE_NUM,456);
	doit();

	setup(CLIENT_GET_COMTREE,POS_REPLY,123);
	p1.setAttrVal(COMTREE_NUM,456);
	p1.setAttrVal(COMTREE_OWNER,forest::forestAdr(1,2));
	p1.setAttrVal(LEAF_COUNT,10);
	p1.setAttrVal(INT_BIT_RATE_DOWN,11);
	p1.setAttrVal(INT_BIT_RATE_UP,12);
	p1.setAttrVal(INT_PKT_RATE_DOWN,13);
	p1.setAttrVal(INT_PKT_RATE_UP,14);
	p1.setAttrVal(EXT_BIT_RATE_DOWN,21);
	p1.setAttrVal(EXT_BIT_RATE_UP,22);
	p1.setAttrVal(EXT_PKT_RATE_DOWN,23);
	p1.setAttrVal(EXT_PKT_RATE_UP,24);
	doit();

	setup(CLIENT_GET_COMTREE,NEG_REPLY,123);
	p1.errMsg = "oops!";
	doit();
	cout << "===================\n";

	setup(CLIENT_MOD_COMTREE,REQUEST,123);
	p1.setAttrVal(COMTREE_NUM,456);
	p1.setAttrVal(INT_BIT_RATE_DOWN,11);
	p1.setAttrVal(INT_BIT_RATE_UP,12);
	p1.setAttrVal(INT_PKT_RATE_DOWN,13);
	p1.setAttrVal(INT_PKT_RATE_UP,14);
	p1.setAttrVal(EXT_BIT_RATE_DOWN,21);
	p1.setAttrVal(EXT_BIT_RATE_UP,22);
	p1.setAttrVal(EXT_PKT_RATE_DOWN,23);
	p1.setAttrVal(EXT_PKT_RATE_UP,24);
	doit();

	setup(CLIENT_MOD_COMTREE,POS_REPLY,123);
	doit();

	setup(CLIENT_MOD_COMTREE,NEG_REPLY,123);
	p1.errMsg = "oops!";
	doit();
	cout << "===================\n";

	setup(CLIENT_GET_LEAF_RATE,REQUEST,123);
	p1.setAttrVal(COMTREE_NUM,456);
	p1.setAttrVal(LEAF_ADR,forest::forestAdr(2,3));
	doit();

	setup(CLIENT_GET_LEAF_RATE,POS_REPLY,123);
	p1.setAttrVal(COMTREE_NUM,456);
	p1.setAttrVal(LEAF_ADR,forest::forestAdr(2,3));
	p1.setAttrVal(BIT_RATE_DOWN,100);
	p1.setAttrVal(BIT_RATE_UP,101);
	p1.setAttrVal(PKT_RATE_DOWN,200);
	p1.setAttrVal(PKT_RATE_UP,201);
	doit();

	setup(CLIENT_GET_LEAF_RATE,NEG_REPLY,123);
	p1.errMsg = "oops!";
	doit();
	cout << "===================\n";

	setup(CLIENT_MOD_LEAF_RATE,REQUEST,123);
	p1.setAttrVal(COMTREE_NUM,456);
	p1.setAttrVal(LEAF_ADR,forest::forestAdr(2,3));
	p1.setAttrVal(BIT_RATE_DOWN,100);
	p1.setAttrVal(BIT_RATE_UP,101);
	p1.setAttrVal(PKT_RATE_DOWN,200);
	p1.setAttrVal(PKT_RATE_UP,201);
	doit();

	setup(CLIENT_MOD_LEAF_RATE,POS_REPLY,123);
	doit();

	setup(CLIENT_MOD_LEAF_RATE,NEG_REPLY,123);
	p1.errMsg = "oops!";
	doit();
	cout << "===================\n";

	setup(ADD_IFACE,REQUEST,123);
	p1.setAttrVal(IFACE_NUM,456);
	p1.setAttrVal(LOCAL_IP,misc::ipAddress("2.3.4.5"));
	p1.setAttrVal(MAX_BIT_RATE,11);
	p1.setAttrVal(MAX_PKT_RATE,12);
	doit();

	setup(ADD_IFACE,POS_REPLY,123);
	doit();

	setup(ADD_IFACE,NEG_REPLY,123);
	p1.errMsg = "oops!";
	doit();
	cout << "===================\n";

	setup(DROP_IFACE,REQUEST,123);
	p1.setAttrVal(IFACE_NUM,456);
	doit();

	setup(DROP_IFACE,POS_REPLY,123);
	doit();

	setup(DROP_IFACE,NEG_REPLY,123);
	p1.errMsg = "oops!";
	doit();
	cout << "===================\n";

	setup(GET_IFACE,REQUEST,123);
	p1.setAttrVal(IFACE_NUM,456);
	doit();

	setup(GET_IFACE,POS_REPLY,123);
	p1.setAttrVal(IFACE_NUM,456);
	p1.setAttrVal(LOCAL_IP,misc::ipAddress("2.3.4.5"));
	p1.setAttrVal(MAX_BIT_RATE,11);
	p1.setAttrVal(MAX_PKT_RATE,12);
	doit();

	setup(GET_IFACE,NEG_REPLY,123);
	p1.errMsg = "oops!";
	doit();
	cout << "===================\n";

	setup(MOD_IFACE,REQUEST,123);
	p1.setAttrVal(IFACE_NUM,456);
	p1.setAttrVal(MAX_BIT_RATE,11);
	p1.setAttrVal(MAX_PKT_RATE,12);
	doit();

	setup(MOD_IFACE,POS_REPLY,123);
	doit();

	setup(MOD_IFACE,NEG_REPLY,123);
	p1.errMsg = "oops!";
	doit();
	cout << "===================\n";

	setup(ADD_LINK,REQUEST,123);
	p1.setAttrVal(LINK_NUM,234);
	p1.setAttrVal(IFACE_NUM,456);
	p1.setAttrVal(PEER_TYPE,CLIENT);
	p1.setAttrVal(PEER_IP,misc::ipAddress("2.3.4.5"));
	p1.setAttrVal(PEER_ADR,forest::forestAdr(5,6));
	doit();

	setup(ADD_LINK,POS_REPLY,123);
	doit();

	setup(ADD_LINK,NEG_REPLY,123);
	p1.errMsg = "oops!";
	doit();
	cout << "===================\n";

	setup(GET_LINK,REQUEST,123);
	p1.setAttrVal(LINK_NUM,234);
	doit();

	setup(GET_LINK,POS_REPLY,123);
	p1.setAttrVal(LINK_NUM,234);
	p1.setAttrVal(IFACE_NUM,456);
	p1.setAttrVal(PEER_TYPE,CLIENT);
	p1.setAttrVal(PEER_IP,misc::ipAddress("2.3.4.5"));
	p1.setAttrVal(PEER_ADR,forest::forestAdr(5,6));
	p1.setAttrVal(PEER_PORT,2345);
	p1.setAttrVal(PEER_DEST,forest::forestAdr(7,8));
	p1.setAttrVal(BIT_RATE,400);
	p1.setAttrVal(PKT_RATE,500);
	doit();

	setup(GET_LINK,NEG_REPLY,123);
	p1.errMsg = "oops!";
	doit();
	cout << "===================\n";

	setup(DROP_LINK,REQUEST,123);
	p1.setAttrVal(LINK_NUM,234);
	doit();

	setup(DROP_LINK,POS_REPLY,123);
	doit();

	setup(DROP_LINK,NEG_REPLY,123);
	p1.errMsg = "oops!";
	doit();
	cout << "===================\n";

	setup(MOD_LINK,REQUEST,123);
	p1.setAttrVal(LINK_NUM,234);
	p1.setAttrVal(PEER_TYPE,CLIENT);
	p1.setAttrVal(PEER_PORT,2345);
	p1.setAttrVal(PEER_DEST,forest::forestAdr(7,8));
	p1.setAttrVal(BIT_RATE,400);
	p1.setAttrVal(PKT_RATE,500);
	doit();

	setup(MOD_LINK,POS_REPLY,123);
	doit();

	setup(MOD_LINK,NEG_REPLY,123);
	p1.errMsg = "oops!";
	doit();
	cout << "===================\n";

	setup(ADD_COMTREE,REQUEST,123);
	p1.setAttrVal(COMTREE_NUM,789);
	doit();

	setup(ADD_COMTREE,POS_REPLY,123);
	doit();

	setup(ADD_COMTREE,NEG_REPLY,123);
	p1.errMsg = "oops!";
	doit();
	cout << "===================\n";

	setup(DROP_COMTREE,REQUEST,123);
	p1.setAttrVal(COMTREE_NUM,789);
	doit();

	setup(DROP_COMTREE,POS_REPLY,123);
	doit();

	setup(DROP_COMTREE,NEG_REPLY,123);
	p1.errMsg = "oops!";
	doit();
	cout << "===================\n";

	setup(GET_COMTREE,REQUEST,123);
	p1.setAttrVal(COMTREE_NUM,789);
	doit();

	setup(GET_COMTREE,POS_REPLY,123);
	p1.setAttrVal(COMTREE_NUM,789);
	p1.setAttrVal(CORE_FLAG,0);
	p1.setAttrVal(PARENT_LINK,3);
	p1.setAttrVal(QUEUE_NUM,20);
	doit();

	setup(GET_COMTREE,NEG_REPLY,123);
	p1.errMsg = "oops!";
	doit();
	cout << "===================\n";

	setup(MOD_COMTREE,REQUEST,123);
	p1.setAttrVal(COMTREE_NUM,789);
	p1.setAttrVal(CORE_FLAG,0);
	p1.setAttrVal(PARENT_LINK,3);
	p1.setAttrVal(QUEUE_NUM,20);
	doit();

	setup(MOD_COMTREE,POS_REPLY,123);
	doit();

	setup(MOD_COMTREE,NEG_REPLY,123);
	p1.errMsg = "oops!";
	doit();
	cout << "===================\n";

	setup(ADD_COMTREE_LINK,REQUEST,123);
	p1.setAttrVal(COMTREE_NUM,789);
	p1.setAttrVal(LINK_NUM,7);
	doit();

	setup(ADD_COMTREE_LINK,POS_REPLY,123);
	doit();

	setup(ADD_COMTREE_LINK,NEG_REPLY,123);
	p1.errMsg = "oops!";
	doit();
	cout << "===================\n";

	setup(DROP_COMTREE_LINK,REQUEST,123);
	p1.setAttrVal(COMTREE_NUM,789);
	p1.setAttrVal(LINK_NUM,7);
	doit();

	setup(DROP_COMTREE_LINK,POS_REPLY,123);
	doit();

	setup(DROP_COMTREE_LINK,NEG_REPLY,123);
	p1.errMsg = "oops!";
	doit();
	cout << "===================\n";

	setup(RESIZE_COMTREE_LINK,REQUEST,123);
	p1.setAttrVal(COMTREE_NUM,789);
	p1.setAttrVal(LINK_NUM,7);
	p1.setAttrVal(BIT_RATE_DOWN,11);
	p1.setAttrVal(BIT_RATE_UP,12);
	p1.setAttrVal(PKT_RATE_DOWN,13);
	p1.setAttrVal(PKT_RATE_UP,14);
	doit();

	setup(RESIZE_COMTREE_LINK,POS_REPLY,123);
	doit();

	setup(RESIZE_COMTREE_LINK,NEG_REPLY,123);
	p1.errMsg = "oops!";
	doit();
	cout << "===================\n";

	setup(ADD_ROUTE,REQUEST,123);
	p1.setAttrVal(COMTREE_NUM,789);
	p1.setAttrVal(DEST_ADR,forest::forestAdr(11,12));
	p1.setAttrVal(LINK_NUM,8);
	p1.setAttrVal(QUEUE_NUM,5);
	doit();

	setup(ADD_ROUTE,POS_REPLY,123);
	doit();

	setup(ADD_ROUTE,NEG_REPLY,123);
	p1.errMsg = "oops!";
	doit();
	cout << "===================\n";

	setup(DROP_ROUTE,REQUEST,123);
	p1.setAttrVal(COMTREE_NUM,789);
	p1.setAttrVal(DEST_ADR,forest::forestAdr(11,12));
	doit();

	setup(DROP_ROUTE,POS_REPLY,123);
	doit();

	setup(DROP_ROUTE,NEG_REPLY,123);
	p1.errMsg = "oops!";
	doit();
	cout << "===================\n";

	setup(MOD_ROUTE,REQUEST,123);
	p1.setAttrVal(COMTREE_NUM,789);
	p1.setAttrVal(DEST_ADR,forest::forestAdr(11,12));
	p1.setAttrVal(LINK_NUM,8);
	p1.setAttrVal(QUEUE_NUM,5);
	doit();

	setup(MOD_ROUTE,POS_REPLY,123);
	doit();

	setup(MOD_ROUTE,NEG_REPLY,123);
	p1.errMsg = "oops!";
	doit();
	cout << "===================\n";

	setup(ADD_ROUTE_LINK,REQUEST,123);
	p1.setAttrVal(COMTREE_NUM,789);
	p1.setAttrVal(DEST_ADR,forest::forestAdr(11,12));
	p1.setAttrVal(LINK_NUM,11);
	doit();

	setup(ADD_ROUTE_LINK,POS_REPLY,123);
	doit();

	setup(ADD_ROUTE_LINK,NEG_REPLY,123);
	p1.errMsg = "oops!";
	doit();
	cout << "===================\n";

	setup(DROP_ROUTE_LINK,REQUEST,123);
	p1.setAttrVal(COMTREE_NUM,789);
	p1.setAttrVal(DEST_ADR,forest::forestAdr(11,12));
	p1.setAttrVal(LINK_NUM,8);
	doit();

	setup(DROP_ROUTE_LINK,POS_REPLY,123);
	doit();

	setup(DROP_ROUTE_LINK,NEG_REPLY,123);
	p1.errMsg = "oops!";
	doit();
	cout << "===================\n";

}
