#include "CtlPkt.h"

buffer_t b; CtlPkt *pp1; CtlPkt *pp2;

void setup(CpTypeIndex cpt, CpRrType rrt, int64_t seq) {
	pp1->reset(&b[Forest::HDR_LENG]);
	pp1->setCpType(cpt); pp1->setRrType(rrt); pp1->setSeqNum(seq);
}

void doit() {
	pp1->write(cout); 
	int len = pp1->pack();
	if (len == 0) cout << "packing error\n";
	pp2->reset(&b[Forest::HDR_LENG]);
	if (!pp2->unpack(len)) cout << "unpacking error\n";
	pp2->write(cout);
	cout << endl;
}

int main() {
	CtlPkt p1(&b[Forest::HDR_LENG]), p2(&b[Forest::HDR_LENG]);
	pp1 = &p1; pp2 = &p2;

	setup(CLIENT_ADD_COMTREE,REQUEST,123);
	doit();

	setup(CLIENT_ADD_COMTREE,POS_REPLY,123);
	p1.setAttr(COMTREE_NUM,456);
	doit();

	setup(CLIENT_ADD_COMTREE,NEG_REPLY,123);
	p1.setErrMsg("oops!");
	doit();
	cout << "===================\n";

	setup(CLIENT_DROP_COMTREE,REQUEST,123);
	p1.setAttr(COMTREE_NUM,456);
	doit();

	setup(CLIENT_DROP_COMTREE,POS_REPLY,123);
	doit();

	setup(CLIENT_DROP_COMTREE,NEG_REPLY,123);
	p1.setErrMsg("oops!");
	doit();
	cout << "===================\n";

	setup(CLIENT_JOIN_COMTREE,REQUEST,123);
	p1.setAttr(COMTREE_NUM,456);
	doit();

	setup(CLIENT_JOIN_COMTREE,POS_REPLY,123);
	doit();

	setup(CLIENT_JOIN_COMTREE,NEG_REPLY,123);
	p1.setErrMsg("oops!");
	doit();
	cout << "===================\n";

	setup(CLIENT_LEAVE_COMTREE,REQUEST,123);
	p1.setAttr(COMTREE_NUM,456);
	doit();

	setup(CLIENT_LEAVE_COMTREE,POS_REPLY,123);
	doit();

	setup(CLIENT_LEAVE_COMTREE,NEG_REPLY,123);
	p1.setErrMsg("oops!");
	doit();
	cout << "===================\n";

	setup(CLIENT_RESIZE_COMTREE,REQUEST,123);
	p1.setAttr(COMTREE_NUM,456);
	doit();

	setup(CLIENT_RESIZE_COMTREE,POS_REPLY,123);
	doit();

	setup(CLIENT_RESIZE_COMTREE,NEG_REPLY,123);
	p1.setErrMsg("oops!");
	doit();
	cout << "===================\n";

	setup(CLIENT_GET_COMTREE,REQUEST,123);
	p1.setAttr(COMTREE_NUM,456);
	doit();

	setup(CLIENT_GET_COMTREE,POS_REPLY,123);
	p1.setAttr(COMTREE_NUM,456);
	p1.setAttr(COMTREE_OWNER,Forest::forestAdr(1,2));
	p1.setAttr(LEAF_COUNT,10);
	p1.setAttr(INT_BIT_RATE_DOWN,11);
	p1.setAttr(INT_BIT_RATE_UP,12);
	p1.setAttr(INT_PKT_RATE_DOWN,13);
	p1.setAttr(INT_PKT_RATE_UP,14);
	p1.setAttr(EXT_BIT_RATE_DOWN,21);
	p1.setAttr(EXT_BIT_RATE_UP,22);
	p1.setAttr(EXT_PKT_RATE_DOWN,23);
	p1.setAttr(EXT_PKT_RATE_UP,24);
	doit();

	setup(CLIENT_GET_COMTREE,NEG_REPLY,123);
	p1.setErrMsg("oops!");
	doit();
	cout << "===================\n";

	setup(CLIENT_MOD_COMTREE,REQUEST,123);
	p1.setAttr(COMTREE_NUM,456);
	p1.setAttr(INT_BIT_RATE_DOWN,11);
	p1.setAttr(INT_BIT_RATE_UP,12);
	p1.setAttr(INT_PKT_RATE_DOWN,13);
	p1.setAttr(INT_PKT_RATE_UP,14);
	p1.setAttr(EXT_BIT_RATE_DOWN,21);
	p1.setAttr(EXT_BIT_RATE_UP,22);
	p1.setAttr(EXT_PKT_RATE_DOWN,23);
	p1.setAttr(EXT_PKT_RATE_UP,24);
	doit();

	setup(CLIENT_MOD_COMTREE,POS_REPLY,123);
	doit();

	setup(CLIENT_MOD_COMTREE,NEG_REPLY,123);
	p1.setErrMsg("oops!");
	doit();
	cout << "===================\n";

	setup(CLIENT_GET_LEAF_RATE,REQUEST,123);
	p1.setAttr(COMTREE_NUM,456);
	p1.setAttr(LEAF_ADR,Forest::forestAdr(2,3));
	doit();

	setup(CLIENT_GET_LEAF_RATE,POS_REPLY,123);
	p1.setAttr(COMTREE_NUM,456);
	p1.setAttr(LEAF_ADR,Forest::forestAdr(2,3));
	p1.setAttr(BIT_RATE_DOWN,100);
	p1.setAttr(BIT_RATE_UP,101);
	p1.setAttr(PKT_RATE_DOWN,200);
	p1.setAttr(PKT_RATE_UP,201);
	doit();

	setup(CLIENT_GET_LEAF_RATE,NEG_REPLY,123);
	p1.setErrMsg("oops!");
	doit();
	cout << "===================\n";

	setup(CLIENT_MOD_LEAF_RATE,REQUEST,123);
	p1.setAttr(COMTREE_NUM,456);
	p1.setAttr(LEAF_ADR,Forest::forestAdr(2,3));
	p1.setAttr(BIT_RATE_DOWN,100);
	p1.setAttr(BIT_RATE_UP,101);
	p1.setAttr(PKT_RATE_DOWN,200);
	p1.setAttr(PKT_RATE_UP,201);
	doit();

	setup(CLIENT_MOD_LEAF_RATE,POS_REPLY,123);
	doit();

	setup(CLIENT_MOD_LEAF_RATE,NEG_REPLY,123);
	p1.setErrMsg("oops!");
	doit();
	cout << "===================\n";

	setup(ADD_IFACE,REQUEST,123);
	p1.setAttr(IFACE_NUM,456);
	p1.setAttr(LOCAL_IP,Np4d::ipAddress("2.3.4.5"));
	p1.setAttr(MAX_BIT_RATE,11);
	p1.setAttr(MAX_PKT_RATE,12);
	doit();

	setup(ADD_IFACE,POS_REPLY,123);
	doit();

	setup(ADD_IFACE,NEG_REPLY,123);
	p1.setErrMsg("oops!");
	doit();
	cout << "===================\n";

	setup(DROP_IFACE,REQUEST,123);
	p1.setAttr(IFACE_NUM,456);
	doit();

	setup(DROP_IFACE,POS_REPLY,123);
	doit();

	setup(DROP_IFACE,NEG_REPLY,123);
	p1.setErrMsg("oops!");
	doit();
	cout << "===================\n";

	setup(GET_IFACE,REQUEST,123);
	p1.setAttr(IFACE_NUM,456);
	doit();

	setup(GET_IFACE,POS_REPLY,123);
	p1.setAttr(IFACE_NUM,456);
	p1.setAttr(LOCAL_IP,Np4d::ipAddress("2.3.4.5"));
	p1.setAttr(MAX_BIT_RATE,11);
	p1.setAttr(MAX_PKT_RATE,12);
	doit();

	setup(GET_IFACE,NEG_REPLY,123);
	p1.setErrMsg("oops!");
	doit();
	cout << "===================\n";

	setup(MOD_IFACE,REQUEST,123);
	p1.setAttr(IFACE_NUM,456);
	p1.setAttr(MAX_BIT_RATE,11);
	p1.setAttr(MAX_PKT_RATE,12);
	doit();

	setup(MOD_IFACE,POS_REPLY,123);
	doit();

	setup(MOD_IFACE,NEG_REPLY,123);
	p1.setErrMsg("oops!");
	doit();
	cout << "===================\n";

	setup(ADD_LINK,REQUEST,123);
	p1.setAttr(LINK_NUM,234);
	p1.setAttr(IFACE_NUM,456);
	p1.setAttr(PEER_TYPE,CLIENT);
	p1.setAttr(PEER_IP,Np4d::ipAddress("2.3.4.5"));
	p1.setAttr(PEER_ADR,Forest::forestAdr(5,6));
	doit();

	setup(ADD_LINK,POS_REPLY,123);
	doit();

	setup(ADD_LINK,NEG_REPLY,123);
	p1.setErrMsg("oops!");
	doit();
	cout << "===================\n";

	setup(GET_LINK,REQUEST,123);
	p1.setAttr(LINK_NUM,234);
	doit();

	setup(GET_LINK,POS_REPLY,123);
	p1.setAttr(LINK_NUM,234);
	p1.setAttr(IFACE_NUM,456);
	p1.setAttr(PEER_TYPE,CLIENT);
	p1.setAttr(PEER_IP,Np4d::ipAddress("2.3.4.5"));
	p1.setAttr(PEER_ADR,Forest::forestAdr(5,6));
	p1.setAttr(PEER_PORT,2345);
	p1.setAttr(PEER_DEST,Forest::forestAdr(7,8));
	p1.setAttr(BIT_RATE,400);
	p1.setAttr(PKT_RATE,500);
	doit();

	setup(GET_LINK,NEG_REPLY,123);
	p1.setErrMsg("oops!");
	doit();
	cout << "===================\n";

	setup(DROP_LINK,REQUEST,123);
	p1.setAttr(LINK_NUM,234);
	doit();

	setup(DROP_LINK,POS_REPLY,123);
	doit();

	setup(DROP_LINK,NEG_REPLY,123);
	p1.setErrMsg("oops!");
	doit();
	cout << "===================\n";

	setup(MOD_LINK,REQUEST,123);
	p1.setAttr(LINK_NUM,234);
	p1.setAttr(PEER_TYPE,CLIENT);
	p1.setAttr(PEER_PORT,2345);
	p1.setAttr(PEER_DEST,Forest::forestAdr(7,8));
	p1.setAttr(BIT_RATE,400);
	p1.setAttr(PKT_RATE,500);
	doit();

	setup(MOD_LINK,POS_REPLY,123);
	doit();

	setup(MOD_LINK,NEG_REPLY,123);
	p1.setErrMsg("oops!");
	doit();
	cout << "===================\n";

	setup(ADD_COMTREE,REQUEST,123);
	p1.setAttr(COMTREE_NUM,789);
	doit();

	setup(ADD_COMTREE,POS_REPLY,123);
	doit();

	setup(ADD_COMTREE,NEG_REPLY,123);
	p1.setErrMsg("oops!");
	doit();
	cout << "===================\n";

	setup(DROP_COMTREE,REQUEST,123);
	p1.setAttr(COMTREE_NUM,789);
	doit();

	setup(DROP_COMTREE,POS_REPLY,123);
	doit();

	setup(DROP_COMTREE,NEG_REPLY,123);
	p1.setErrMsg("oops!");
	doit();
	cout << "===================\n";

	setup(GET_COMTREE,REQUEST,123);
	p1.setAttr(COMTREE_NUM,789);
	doit();

	setup(GET_COMTREE,POS_REPLY,123);
	p1.setAttr(COMTREE_NUM,789);
	p1.setAttr(CORE_FLAG,0);
	p1.setAttr(PARENT_LINK,3);
	p1.setAttr(QUEUE_NUM,20);
	doit();

	setup(GET_COMTREE,NEG_REPLY,123);
	p1.setErrMsg("oops!");
	doit();
	cout << "===================\n";

	setup(MOD_COMTREE,REQUEST,123);
	p1.setAttr(COMTREE_NUM,789);
	p1.setAttr(CORE_FLAG,0);
	p1.setAttr(PARENT_LINK,3);
	p1.setAttr(QUEUE_NUM,20);
	doit();

	setup(MOD_COMTREE,POS_REPLY,123);
	doit();

	setup(MOD_COMTREE,NEG_REPLY,123);
	p1.setErrMsg("oops!");
	doit();
	cout << "===================\n";

	setup(ADD_COMTREE_LINK,REQUEST,123);
	p1.setAttr(COMTREE_NUM,789);
	p1.setAttr(LINK_NUM,7);
	doit();

	setup(ADD_COMTREE_LINK,POS_REPLY,123);
	doit();

	setup(ADD_COMTREE_LINK,NEG_REPLY,123);
	p1.setErrMsg("oops!");
	doit();
	cout << "===================\n";

	setup(DROP_COMTREE_LINK,REQUEST,123);
	p1.setAttr(COMTREE_NUM,789);
	p1.setAttr(LINK_NUM,7);
	doit();

	setup(DROP_COMTREE_LINK,POS_REPLY,123);
	doit();

	setup(DROP_COMTREE_LINK,NEG_REPLY,123);
	p1.setErrMsg("oops!");
	doit();
	cout << "===================\n";

	setup(RESIZE_COMTREE_LINK,REQUEST,123);
	p1.setAttr(COMTREE_NUM,789);
	p1.setAttr(LINK_NUM,7);
	p1.setAttr(BIT_RATE_DOWN,11);
	p1.setAttr(BIT_RATE_UP,12);
	p1.setAttr(PKT_RATE_DOWN,13);
	p1.setAttr(PKT_RATE_UP,14);
	doit();

	setup(RESIZE_COMTREE_LINK,POS_REPLY,123);
	doit();

	setup(RESIZE_COMTREE_LINK,NEG_REPLY,123);
	p1.setErrMsg("oops!");
	doit();
	cout << "===================\n";

	setup(ADD_ROUTE,REQUEST,123);
	p1.setAttr(COMTREE_NUM,789);
	p1.setAttr(DEST_ADR,Forest::forestAdr(11,12));
	p1.setAttr(LINK_NUM,8);
	p1.setAttr(QUEUE_NUM,5);
	doit();

	setup(ADD_ROUTE,POS_REPLY,123);
	doit();

	setup(ADD_ROUTE,NEG_REPLY,123);
	p1.setErrMsg("oops!");
	doit();
	cout << "===================\n";

	setup(DROP_ROUTE,REQUEST,123);
	p1.setAttr(COMTREE_NUM,789);
	p1.setAttr(DEST_ADR,Forest::forestAdr(11,12));
	doit();

	setup(DROP_ROUTE,POS_REPLY,123);
	doit();

	setup(DROP_ROUTE,NEG_REPLY,123);
	p1.setErrMsg("oops!");
	doit();
	cout << "===================\n";

	setup(MOD_ROUTE,REQUEST,123);
	p1.setAttr(COMTREE_NUM,789);
	p1.setAttr(DEST_ADR,Forest::forestAdr(11,12));
	p1.setAttr(LINK_NUM,8);
	p1.setAttr(QUEUE_NUM,5);
	doit();

	setup(MOD_ROUTE,POS_REPLY,123);
	doit();

	setup(MOD_ROUTE,NEG_REPLY,123);
	p1.setErrMsg("oops!");
	doit();
	cout << "===================\n";

	setup(ADD_ROUTE_LINK,REQUEST,123);
	p1.setAttr(COMTREE_NUM,789);
	p1.setAttr(DEST_ADR,Forest::forestAdr(11,12));
	p1.setAttr(LINK_NUM,11);
	doit();

	setup(ADD_ROUTE_LINK,POS_REPLY,123);
	doit();

	setup(ADD_ROUTE_LINK,NEG_REPLY,123);
	p1.setErrMsg("oops!");
	doit();
	cout << "===================\n";

	setup(DROP_ROUTE_LINK,REQUEST,123);
	p1.setAttr(COMTREE_NUM,789);
	p1.setAttr(DEST_ADR,Forest::forestAdr(11,12));
	p1.setAttr(LINK_NUM,8);
	doit();

	setup(DROP_ROUTE_LINK,POS_REPLY,123);
	doit();

	setup(DROP_ROUTE_LINK,NEG_REPLY,123);
	p1.setErrMsg("oops!");
	doit();
	cout << "===================\n";

}
