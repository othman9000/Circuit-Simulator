// Circuit Simulator V2.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <string>
#include <sstream>
#include <variant>
#include <stdexcept>
#include <cmath>
using namespace std;
using ld = long double;


enum ElementType {
	R, CS, VS
};

struct Element {
	ld Value;
	ElementType Type;
	Element(ld v, ElementType t) : Value(v), Type(t) {}
};
struct Branch {
	bool RES;
	bool SOURCE;
	ElementType SourceType;
	ld Resistance;
	ld SourceValue;
	Branch() :RES(false), SOURCE(false), Resistance(0), SourceValue(0), SourceType(ElementType::R) {}
	bool IsSuperNodeBranch() const {
		return SOURCE and not RES and SourceType == ElementType::VS;
	}
};
struct Node {
	vector<pair<Branch, int>> Branches;
	int NodeID;
	Node() : NodeID(-1) {}
	Node(int id) : NodeID(id) {}
	void addBranch(Branch b, int connectedTO) { Branches.push_back({ b,connectedTO }); }
};
class Circuit {
	vector<Node> Nodes;
	vector<bool> visited;
	ld getEquation(int NodeID,vector<ld>& G) {
		ld constant = 0.0L;
		const Node& n = Nodes[NodeID];
		for (auto& p : n.Branches){ 
			auto& b = p.first;
			if (not b.IsSuperNodeBranch()) {
				if (b.SourceType == ElementType::CS) constant += b.SourceValue;
				else {
					//if it is not a supernode branch nor a current source branch then it must contain resistance
					G[NodeID-1]					+= 1 / b.Resistance;
					if(p.second != 0)
					G[p.second-1]				-= 1 / b.Resistance;
					if (b.SOURCE)	constant	+= b.SourceValue / b.Resistance;
				}
			}
		}
		return constant;
	}
	void visit(int mainID,int NodeID, vector<vector<ld>>& G, vector<ld>& C,bool& superNodeConnectedToGround,int prev) {
		if (visited[NodeID]) throw runtime_error("detected a voltage source loop (undefined behavior");
		visited[NodeID] = true;
		if(not superNodeConnectedToGround)
			C[mainID-1] += getEquation(NodeID, G[mainID-1]);
		for (auto& p : Nodes[NodeID].Branches) {
			if (p.second == prev) continue;
			auto& b = p.first;
			if (b.IsSuperNodeBranch()) {
				if(NodeID !=0 and p.second != 0){
					G[p.second - 1][NodeID - 1] += 1.0L;
					G[p.second - 1][p.second - 1] += -1.0L;
					C[p.second - 1] += b.SourceValue;
					visit(mainID, p.second, G, C, superNodeConnectedToGround, NodeID);
				}
				else if (p.second == 0) {
					//if we detect a supernode network connected to ground
					//then we have to remove the supernode equation
					//and put in its place the value of voltage source connecting the network to ground to equal to the its node value
					superNodeConnectedToGround = true;
					for (auto& i : G[mainID-1]) i = 0.0L;
					G[mainID - 1][NodeID - 1] = 1.0L;
					C[mainID - 1] = b.SourceValue;
					visit(mainID, p.second, G, C, superNodeConnectedToGround, NodeID);
				}
				else if (NodeID == 0) {
					//all supernodes connnected to ground will have their value determiend immediately
					G[p.second - 1][p.second - 1] = 1.0L;
					C[p.second - 1] = -b.SourceValue;
					visit(mainID, p.second, G, C, superNodeConnectedToGround, NodeID);
				}
			}
		}
	}
	
public:
	void getEquations(vector<ld>& C,vector<vector<ld>>& G) {
		G.resize(Nodes.size()-1,vector<ld>(Nodes.size()-1,0.0L));
		C.resize(Nodes.size()-1);
		visited.resize(Nodes.size(), false);
		bool superNodeConnectedToGround = false;
		for (int i = 1; i < Nodes.size(); ++i, superNodeConnectedToGround = false)
			if (not visited[i]) visit(i,i,G,C, superNodeConnectedToGround,i);
	}
	void setSizeOfNodes(int n) {
		Nodes.resize(n);
	}
	void addNode(Node n) {
		Nodes[n.NodeID] = n;
	}
	bool wrongCir(void) {
		if (Nodes.size() < 2)	return true;
		for (Node& n : Nodes)	if (n.Branches.size() < 2) return true;
		return false;
	}
};

void parse(ifstream& inputFile,Circuit& RES) {
	int NodeID = 0;
	int lineCount = 0;
	using VAR = variant<monostate,Element, Branch, Node>;
	string KEYWORDS[] = { "add","branch","node","resistor","currents","voltages" };
	int keyWordsLen = 6;
	enum WORDTYPE {keyword,newIdentifier,alreadyDeclaredIdentifier,value,nothing};
	
	class identifiers {
		unordered_map<string, VAR> ids;
	public:
		bool doesExist(const string& id) {
			return ids.find(id) != ids.end();
		}
		void addID(const string& id, VAR var) {
			ids.insert({ id,var });
		}
		VAR& getVar(const string& id) {
			return ids[id];
		}
		void addNodesToCirc(Circuit& C,int NodeCount) {
			C.setSizeOfNodes(NodeCount);
			for (auto& p : ids)			if (holds_alternative<Node>(p.second))	C.addNode(get<Node>(p.second));
		}
	};
	identifiers IDS;
	string line;
	auto isLD = [](const string& s) {
		int i = 0;
		if (s[0] == '+' or s[0] == '-') i++;
		bool DOT = false;
		bool digit = false;
		for (; i < s.length();++i) {
			if (s[i] == '.') if (DOT)			return false; else DOT = true;
			else if (!isdigit(s[i]))			return false;
			else								digit = true;
		}
		return digit;
	};
	auto getType = [&](const string& word) {
		for (int i = 0; i < keyWordsLen; ++i)	if (word == KEYWORDS[i])		return WORDTYPE::keyword;
		if (isLD(word))															return WORDTYPE::value;
		if (IDS.doesExist(word))												return WORDTYPE::alreadyDeclaredIdentifier;
																				return WORDTYPE::newIdentifier;
	};
	auto correctIDWord = [](const string& s) {
		int i = 0;
		if (!isalpha(s[i++])) return false;
		for (; i < s.size(); ++i)	if (not isalpha(s[i]) and not isdigit(s[i])) return false;
		return true;
	};
	auto convertWordTypeIntoString = [](WORDTYPE wt) {
		switch (wt) {
		case WORDTYPE::keyword:{
			return string("keyword");
		}
		case WORDTYPE::newIdentifier: {
			return string("new identifier");
		}
		case WORDTYPE::alreadyDeclaredIdentifier: {
			return string("already declared identifier");
		}
		case WORDTYPE::value: {
			return string("value");
		}
		case WORDTYPE::nothing: {
			return string("nothing");
		}
		}
	};
	//looping through each line
	while (getline(inputFile, line)) {
		lineCount++;
		if (line.empty()) continue;
		for (char& c : line) c = tolower(static_cast<unsigned char>(c));
		stringstream words(line);
		string word;
		words >> word;
		if (word[0] == '#') continue;
		string keyword, id1, id2,id3;
		ld value;
		WORDTYPE expected = WORDTYPE::keyword;
		//looping through each word
		do {
			WORDTYPE wt = getType(word);
			if (wt == expected) {
				switch (wt) {
				case WORDTYPE::keyword: {
					keyword = word;
					if (word == "add")  expected = WORDTYPE::alreadyDeclaredIdentifier;
					else				expected = WORDTYPE::newIdentifier;
					break;
				}
				case WORDTYPE::newIdentifier: {
					id1 = word;
					if (not correctIDWord(id1)) throw runtime_error("SYNTAX ERROR AT LINE " + to_string(lineCount) + ", wrong identifier naming, (identifiers must begin with an alphabet and only contain characters and numebrs");
					if (keyword == "resistor" or keyword == "currents" or keyword == "voltages")	expected = WORDTYPE::value;
					else																			expected = WORDTYPE::nothing;
					break;
				}
				case WORDTYPE::value: {
					value = stold(word);
					expected = WORDTYPE::nothing;
					break;
				}
				case WORDTYPE::alreadyDeclaredIdentifier: {
					//manullay handeling the "add" statement-branches :
					//the correct syntax is
					//add branchID elementID
					//add nodeID nodeID branchID
					if (id1.empty()) {
						id1 = word;
						if (holds_alternative<Element>(IDS.getVar(word))) throw runtime_error("SYNTAX ERROR AT LINE " + to_string(lineCount) + ", expected a node or branch identifier but found element identifier after add keyword");
						expected = alreadyDeclaredIdentifier;
					}
					else if (id2.empty()) {
						id2 = word;
						if (holds_alternative<Branch>(IDS.getVar(id1)) and holds_alternative<Element>(IDS.getVar(id2))) 	expected = WORDTYPE::nothing;
						else if (holds_alternative<Node>(IDS.getVar(id1)) and holds_alternative<Node>(IDS.getVar(id2)))		expected = WORDTYPE::alreadyDeclaredIdentifier;
						else		throw runtime_error("SYNTAX ERROR AT LINE " + to_string(lineCount));
					}
					else {
						id3 = word;
						if (holds_alternative<Branch>(IDS.getVar(id3))) expected = WORDTYPE::nothing;
						else											throw runtime_error("SYNTAX ERROR AT LINE " + to_string(lineCount) + ", expected a branch identifier but didn't find it");
					}
					break;
				}
				}
			}
			else {
				/*cout << "word is " << word << endl;
				cout << "keyword is " << keyword << endl;
				cout << "id1 = " << id1 << endl;
				cout << "id2 = " << id2 << endl;*/
				throw runtime_error("SYNTAX ERROR AT LINE " + to_string(lineCount) + ", expected a " + convertWordTypeIntoString(expected) + " but found : " + word);
			}
		} while (words >> word);
		if (expected != WORDTYPE::nothing) 
			throw runtime_error("SYNTAX ERROR AT LINE " + to_string(lineCount) + ", expected a " + convertWordTypeIntoString(expected) + " but found a word");
		if (keyword == "resistor")
			IDS.addID(id1, Element(value, ElementType::R));
		else if (keyword == "currents")
			IDS.addID(id1, Element(value, ElementType::CS));
		else if (keyword == "voltages")
			IDS.addID(id1, Element(value, ElementType::VS));
		else if (keyword == "branch")
			IDS.addID(id1, Branch());
		else if (keyword == "node")
			IDS.addID(id1, Node(NodeID++));
		else if (keyword == "add") {
			if (id3.empty()) {
				//handeling adding elements to a branch
			    Branch& b = get<Branch>(IDS.getVar(id1));
				Element& e = get<Element>(IDS.getVar(id2));
				if (e.Type == ElementType::R) {
					b.Resistance += e.Value;
					if(not b.RES) b.RES = true;
				}
				else if (e.Type == ElementType::VS) {
					if (b.SourceType == ElementType::R)						b.SourceType = ElementType::VS,b.SOURCE = true;
					else if (b.SourceType == ElementType::CS)				throw runtime_error("LOGICAL ERROR AT LINE " + to_string(lineCount) + ", can't add voltage source and current source in the same branch ");
					b.SourceValue += e.Value;
				}
				else {
					if(b.SOURCE)
						if(b.SourceType == ElementType::CS)					throw runtime_error("LOGICAL ERROR AT LINE " + to_string(lineCount) + ", can't add two current sources in the same branch ");
						else												throw runtime_error("LOGICAL ERROR AT LINE " + to_string(lineCount) + ", can't add voltage source and current source in the same branch ");
					b.SOURCE = true;
					b.SourceType = ElementType::CS;
					b.SourceValue = e.Value;
				}
			}
			else {
				//handeling adding a branch between two nodes
				//the first node will have the positive terminal of the source while the second one will have the negative
				Node& node1 = get<Node>(IDS.getVar(id1));
				Node& node2 = get<Node>(IDS.getVar(id2));
				Branch b	= get<Branch>(IDS.getVar(id3));
				if (not b.RES and not b.SOURCE)		throw runtime_error("LOGICAL ERROR AT LINE " + to_string(lineCount) + ", can't empty branch between two nodes");
				if(node1.NodeID == node2.NodeID)	throw runtime_error("LOGICAL ERROR AT LINE " + to_string(lineCount) + ", can't add a branch between the same node");
				node1.addBranch(b, node2.NodeID);
				if (b.SOURCE) b.SourceValue = -b.SourceValue;
				node2.addBranch(b, node1.NodeID);
			}
		}
	}
	IDS.addNodesToCirc(RES,NodeID);
}

void Gauss(vector<vector<ld>>& Matrix, vector<ld>& Constants, vector<ld>& res) {
	int MatrixDim = Constants.size();
	res.resize(MatrixDim, 0);
	//forward elimination O(n^3)
	for (int i = 0; i < MatrixDim; ++i) {
		//partial pivoting
		int j = i;
		ld EPS = 1e-12L;
		while (j < MatrixDim and fabs(Matrix[j][i]) < EPS) j++;
		if (j == MatrixDim) throw runtime_error("ERROR: singluar matrix (no solution)");
		if (j != i) swap(Matrix[i], Matrix[j]);
		//forward elimination
		for (j = i + 1; j < MatrixDim; ++j) {
			if (fabs(Matrix[j][i]) > EPS) {
				ld ratio = Matrix[j][i] / Matrix[i][i];
				Matrix[j][i] = 0.0L;
				for (int k = i+1; k < MatrixDim; ++k) 
					Matrix[j][k] -= ratio * Matrix[i][k];
				Constants[j] -= ratio * Constants[i];
			}
		}
	}
	//backward substitution O(n^2)
	for (int i = MatrixDim - 1; i >= 0; --i) {
		ld num = Constants[i];
		for (int j = i + 1; j < MatrixDim; ++j) num -= Matrix[i][j] * res[j];
		res[i] = num / Matrix[i][i];
	}
}

int main()
{
	string readPath;
	cout << "Please enter the path of the circuit file:" << endl;
	getline(cin,readPath);
	ifstream file(readPath);
	try {
		if (!file) 			throw runtime_error("ERROR: can't open the file");
		Circuit CIR;
		parse(file, CIR);
		if(CIR.wrongCir())  throw runtime_error("the circuit has a logical error: either a floathing node was detected or the number of nodes in the circuit is less than 2");
		vector<vector<ld>> G; vector<ld> C;
		CIR.getEquations(C, G);
		for (int i = 0; i < C.size();++i) {
			for (ld& j : G[i]) cout << j << ' '; cout << C[i] << endl;
		}
		vector<ld> res;
		Gauss(G, C, res);
		for (int i = 0; i < res.size();i++)
			cout << "V" << i + 1 << " = " << res[i] << endl;
	}
	catch (runtime_error& e) { cout << e.what() << endl; }
	file.close();
}
