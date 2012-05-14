/** @file Console.cpp 
 *
 *  @author Jon Turner
 *  @date 2011
 *  This is open source software licensed under the Apache 2.0 license.
 *  See http://www.apache.org/licenses/LICENSE-2.0 for details.
 */

#include <list>
#include "CommonDefs.h"
#include "PacketHeader.h"
#include "CtlPkt.h"

const short NM_PORT = 30122; 		///< server port# for NetMgr

// various helper routines
bool sendReqPkt(int, CtlPkt&, fAdr_t, CtlPkt&);
bool setAttributes(CtlPkt&, CpTypeIndex, CtlPkt&);
void parseLine(string, list<string>&);
void buildWordList(string, list<string>&);
bool validTokenList(list<string>);
void processTokenList(list<string>, fAdr_t&, CpTypeIndex&, CtlPkt&);
void posResponse(CtlPkt&);

/** usage:
 *       Console NetMgrIp
 */
main(int argc, char *argv[]) {
	ipa_t nmIp;

	if (argc != 2 || (nmIp = Np4d::getIpAdr(argv[1])) == 0)
		fatal("usage: fConsole netMgrIp");

	int sock = Np4d::streamSocket();
	if (sock < 0 ||
	    !Np4d::connect4d(sock,nmIp,NM_PORT) ||
	    !Np4d::nonblock(sock))
		fatal("can't connect to NetMgr");

	// start processing command line input

	fAdr_t target = 0;	// forest address of request packet target
	CtlPkt cpTemplate;	// template used to store attributes

	while (true) {
		cout << "console: ";

		string line; getline(cin,line);
		if (line.find("quit") == 0) break;
		if (line.find("clear") == 0) {
			target = 0; cpTemplate.reset(); continue;
		}
		if (line.find("show") == 0) {
			// show target and all defined attributes
			if (target != 0) {
				cout << "target=";
				Forest::writeForestAdr(cout,target);
				cout << endl;
			}
			for (int i = CPA_START+1; i < CPA_END; i++) {
				CpAttrIndex ii = CpAttrIndex(i);
				if (cpTemplate.isSet(ii)) {
					cpTemplate.writeAvPair(cout,ii);
					cout << endl;
				}
			}
			continue;
		}

		list<string> tokenList;
		parseLine(line, tokenList);
		if (validTokenList(tokenList)) {
			CpTypeIndex reqType = CPT_START;
			processTokenList(tokenList,target,reqType,cpTemplate);
			if (reqType == CPT_START) continue;

			if (target == 0) {
				cout << "no target defined for command\n";
				continue;
			}

			CtlPkt reqPkt, replyPkt;
			reqPkt.reset();
			reqPkt.setCpType(reqType);
			reqPkt.setRrType(REQUEST);

			replyPkt.reset();

			if (!setAttributes(cpTemplate,reqType,reqPkt)) {
				cout << "missing one or more required "
					"attributes\n";
			} else if (!sendReqPkt(sock,reqPkt,target,replyPkt)) {
				cout << "no valid reply received\n";
			} else {
				if (replyPkt.getRrType() == POS_REPLY) {
					posResponse(replyPkt);
					//cout << "success\n";
				} else {
					cout << "error reported: "
					     << replyPkt.getErrMsg() << endl;
				}
			} 
		} else {
			cout << "cannot recognize command\n";
		}
	}
}

void posResponse(CtlPkt& cp) {
	bool printedSomething = false;
	CpTypeIndex cpType = cp.getCpType();
	for (int i = CPA_START+1; i < CPA_END; i++) {
		CpAttrIndex ii = CpAttrIndex(i);
		if (!CpType::isRepAttr(cpType,ii)) continue;
		printedSomething = true;
		cout << CpAttr::getName(ii) << "=";
		int32_t val = cp.getAttr(ii);
		if (ii == COMTREE_OWNER || ii == LEAF_ADR ||
		    ii == PEER_ADR || ii == PEER_DEST ||
		    ii == DEST_ADR) {
			Forest::writeForestAdr(cout,(fAdr_t) val);
		} else if (ii == LOCAL_IP || ii == PEER_IP || ii == RTR_IP) {
			string s; Np4d::addIp2string(s,val);
			cout << s;
		} else if (ii == PEER_TYPE) {
			string s; Forest::addNodeType2string(s,(ntyp_t) val);
			cout << s;
		} else {
			cout << val;
		}
		cout << " ";
	}
	if (printedSomething) cout << endl;
}

/** Send a request packet, then wait for and return reply.
 *  If no reply received after a second, try again up to five times.
 *  Print progress indicator on command line while waiting.
 *  @param sock is the socket on which request is to be sent
 *  @param reqPkt is the control packet to be sent
 *  @param target is the forest address of the control packet destination
 *  @param replyPkt will contain the reply packet on a successful return
 *  @return true on success, false on failure
 */
bool sendReqPkt(int sock, CtlPkt& reqPkt,
			   fAdr_t target, CtlPkt& replyPkt) {
	buffer_t reqBuf, replyBuf;
	PacketHeader reqHdr, replyHdr;

	int pleng = Forest::HDR_LENG + sizeof(uint32_t) +
		    reqPkt.pack(&reqBuf[Forest::HDR_LENG/sizeof(uint32_t)]);

	reqHdr.setLength(pleng);
	reqHdr.setPtype(NET_SIG);
	reqHdr.setFlags(0);
	reqHdr.setComtree(Forest::CLIENT_SIG_COMT);
	reqHdr.setSrcAdr(0);		// to be filled in by NetMgr
	reqHdr.setDstAdr(target);

	reqHdr.pack(reqBuf);

	if (Np4d::sendBuf(sock, (char *) &reqBuf[0], pleng) != pleng)
		fatal("can't send control packet to NetMgr");

	for (int i = 0; i < 3; i++) {
		usleep(1000000);
		// if there is a reply, unpack and return true
		int nbytes = Np4d::recvBuf(sock, (char *) &replyBuf[0],
			        	  	 Forest::BUF_SIZ);
		if (nbytes <= 0) {
			cout << "."; cout.flush();
			Np4d::sendBuf(sock, (char *) &reqBuf[0], pleng);
			continue;
		}
		replyHdr.unpack(replyBuf);
		if (!replyPkt.unpack(&replyBuf[Forest::HDR_LENG/sizeof(uint32_t)],
				nbytes-(Forest::HDR_LENG + sizeof(uint32_t))))
			return false;
		return replyHdr.getSrcAdr() == target;
	}
	return false;
}

/** Set request packet attributes, based on template.
 *  @param cpTemplate is a control packet from which attributes are copied
 *  @param type is the type of the control packet to be sent
 *  @param reqPkt is the formatted control packet on return
 *  @return false if some attribute that is required for the given type
 *  has not been set in the cpTemplate; otherwise return true
 */
bool setAttributes(CtlPkt& cpTemplate, CpTypeIndex type, CtlPkt& reqPkt) {
	for (int i = CPA_START+1; i < CPA_END; i++) {
		CpAttrIndex ii = CpAttrIndex(i);
		if (CpType::isReqAttr(type, ii)) {
			if (cpTemplate.isSet(ii)) {
				reqPkt.setAttr(ii,cpTemplate.getAttr(ii));
			} else if (CpType::isReqReqAttr(type, ii)) {
				return false;
			}
		}
	}
	return true;
}

/** Parse an input line and produce a list of tokens.
 *  If the line starts with a command abbreviation or phrase,
 *  the first token will be that abbreviation/phrase with no
 *  extra white space.
 *  Subsequent tokens will take the form word=word.
 */
void parseLine(string line, list<string>& tokenList) {
	// build a list of words, where a word may be the string "="
	// or a string containing no white space and no equal sign
	list<string> wordList;
	buildWordList(line, wordList);

	tokenList.clear();

	// find first "=" in wordList
	list<string>::iterator p = wordList.begin();
	int pos = 0;
	while (p != wordList.end()) {
		if ((*p)[0] == '=') break;
		p++; pos++;
	}

	// now check for a command abbreviation or phrase
	if (pos == 0) return;
	if (p != wordList.end() && pos == 1) {
		// no command abbreviation or phrase
		p = wordList.begin();
	} else {
		int last = (p == wordList.end() ? pos-1 : pos-2);
		// words 0 1 .. last, could be command abbreviation or phrase
		string s;
		p = wordList.begin();
		for (int i = 0; i <= last; i++) {
			s += (*p++);
			if (i < last) s += " ";
		}
		tokenList.push_back(s);
		s.clear();
	}

	// at this point, remaining words should consist of triples of the form
	// attribute = value
	while (p != wordList.end()) {
		list<string>::iterator pp, ppp;
		pp  = p;  pp++;  if (pp  == wordList.end()) return;
		ppp = pp; ppp++; if (ppp == wordList.end()) return;
		if ((*pp)[0] != '=') return;
		string s;
		s += *p; s += "="; s+= *ppp;
		tokenList.push_back(s);
		p = ppp; p++;
		s.clear();
	}
}

/** Build a list of words from a line.
 *  Creates a list of words from the line, where a word is either
 *  an '=' sign, or a string of characters containing neither
 *  white space of an '=' sign.
 *  @param line is the line to be processed,
 *  @param wordList is the list in which the words are returned
 */
void buildWordList(string line, list<string>& wordList) {
	char white[] = " \t\n"; char delim[] = " =\t\n";

	wordList.clear();
	int pos = 0;
	while (true) {
		int start = line.find_first_not_of(white,pos);
		if (start == string::npos) return;
		int end;
		if (line[start] == '=') {
			wordList.push_back("=");
			pos = start+1;
		} else if (isalnum(line[start]) || line[start] == '-') {
			end = line.find_first_of(delim,start);
			string s = (end == string::npos ?
				    line.substr(start) :
				    line.substr(start,end-start));
			wordList.push_back(s);
			pos = end;
		} else return; // only legal possibility is #
	}
}

/** Verify that a tokenList is valid
 *  Specificialy, check that if it starts with a command phrase or abbreviation,
 *  then it's a valid command phrase or abbreviation.
 *  Also, check each attribute-value assignment to verify that the
 *  attribute is valid
 *  @param tokenList is a list of tokens
 *  @return true if the token list passes all checks, else false
 */
bool validTokenList(list<string> tokenList) {
	// if first token is not an assignment verify that it's a valid
	// command abbreviation or phrase
	list<string>::iterator p = tokenList.begin();
	if (p == tokenList.end()) return true;
	if ((*p).find('=') == string::npos) {
		// so first token must be a command phrase or abbreviation
		for (int i = CPT_START+1; true ; i++) {
			CpTypeIndex ii = CpTypeIndex(i);
			if (i == CPT_END) return false;
			if ((*p).compare(CpType::getName(ii)) == 0)
				break; 
			if ((*p).compare(CpType::getAbbrev(ii)) == 0)
				break; 
		}
		p++;
	}

	// for each remaining token, verify that left part is a
	// valid control packet attribute name or a CLI "pseudo-attribute"
	while (p != tokenList.end()) {
		int pos = (*p).find('=');
		string attrib = (*p).substr(0,pos);
		for (int i = CPA_START+1; true ; i++) {
			CpAttrIndex ii = CpAttrIndex(i);

			if (i == CPA_END) {
				if (attrib.compare("target") == 0) break;
				else return false;
			}
			if (attrib.compare(CpAttr::getName(ii)) == 0)
				break;
		}
		p++;
	}
	return true;
}

/** Process a list of tokens that has passed basic checks.
 *  @param tokenList is the list of tokens to be processed.
 *  @param target if the token list includes an assignment of
 *  the form "target=forest address", target will be set to the
 *  specified forest address on return
 *  @param cpType if the token list starts with a command phrase 
 *  or abbreviation, cpType will be set to the CpTypeIndex for
 *  the corresponding request packet type on return
 *  @param cpTemplate is a control packet in which attribute values
 *  are returned; every attribute assignment in the token list
 *  updates the corresponding attribute in cpTemplate
 */
void processTokenList(list<string> tokenList, fAdr_t& target,
				 CpTypeIndex& cpType, CtlPkt& cpTemplate) {
	// get the control packet index if there is one
	list<string>::iterator p = tokenList.begin();
	if ((*p).find('=') == string::npos) {
		for (int i = CPT_START+1; i < CPT_END ; i++) {
			CpTypeIndex ii = CpTypeIndex(i);
			if ((*p).compare(CpType::getName(ii)) == 0) {
				cpType = ii; break; 
			}
			if ((*p).compare(CpType::getAbbrev(ii)) == 0) {
				cpType = ii; break; 
			}
		}
		p++;
	}

	// process all the assignments storing attributes
	// in the control packet template
	for (/* nada */; p != tokenList.end(); p++) {
		int pos = (*p).find('=');
		string leftSide = (*p).substr(0,pos);
		string rightSide = (*p).substr(pos+1);
		CpAttrIndex attr = CPA_START;
		for (int i = CPA_START+1; i < CPA_END ; i++) {
			CpAttrIndex ii = CpAttrIndex(i);
			if (leftSide.compare(CpAttr::getName(ii)) == 0) {
				attr = ii; break;
			}
		}
		
		if ((*p).find("target") == 0) {
			target = Forest::forestAdr(rightSide.c_str()); 
			continue;
		}
		switch (attr) {
			// set the attribute appropriately
			// in the control packet template
			case DEST_ADR: case LEAF_ADR:
			case PEER_ADR: case PEER_DEST:
				fAdr_t fa;
				fa = Forest::forestAdr(rightSide.c_str());
				if (fa != 0) cpTemplate.setAttr(attr, fa);
				break;
			case LOCAL_IP: case PEER_IP: case RTR_IP:
				ipa_t ipa;
				ipa = Np4d::ipAddress(rightSide.c_str());
				if (ipa != 0) cpTemplate.setAttr(attr, ipa);
				break;
			case PEER_TYPE:
				ntyp_t nt;
				nt = Forest::getNodeType(rightSide);
				if (nt != UNDEF_NODE)
					cpTemplate.setAttr(attr, nt);
				break;
			default: // remaining attributes have integer values
				uint32_t value = atoi(rightSide.c_str());
				cpTemplate.setAttr(attr, value);
				break;
		}
	}
}
