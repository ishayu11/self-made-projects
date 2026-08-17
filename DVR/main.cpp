#include "node.h"

vector<RoutingNode*> distanceVectorNodes;

int routingAlgo(vector<RoutingNode*> distanceVectorNodes);

int main() {
	int n;
	cin>>n;
	string name;
	distanceVectorNodes.clear();
	for (int i = 0 ; i < n; i++) {
		RoutingNode *newnode = new RoutingNode();
		cin>>name;
		newnode->setName(name);
		distanceVectorNodes.push_back(newnode);
	}
	cin>>name;

	while(name != "EOE") {
		for (int i =0 ; i < distanceVectorNodes.size(); i++) {
			if(distanceVectorNodes[i]->getName() == name) {
				string myeth, oeth, oname;
				int cost;
				cin >> myeth >> oeth >> oname >> cost;

				for(int j = 0 ; j < distanceVectorNodes.size(); j++) {
					if(distanceVectorNodes[j]->getName() == oname) {
						distanceVectorNodes[i]->addInterface(myeth, oeth, distanceVectorNodes[j], cost);
						distanceVectorNodes[i]->addTblEntry(myeth, 0);
						break;
					}
				}
			}
		}
		cin>>name;
	}

	cout << "\n--- Initial Convergence ---\n";
	routingAlgo(distanceVectorNodes);

	// Simulate Link Failure
	cout << "\n--- Simulating Link Failure (A <-> B) ---\n";
	for(int i = 0; i < distanceVectorNodes.size(); ++i) {
		if (distanceVectorNodes[i]->getName() == "A") {
			distanceVectorNodes[i]->updateInterfaceCost("10.0.0.1", "10.0.0.21", INF);
		}
		if (distanceVectorNodes[i]->getName() == "B") {
			distanceVectorNodes[i]->updateInterfaceCost("10.0.0.21", "10.0.0.1", INF);
		}
	}

	// Continue running algorithm for multiple rounds
	int failure_rounds = routingAlgo(distanceVectorNodes);
	cout << "Reconvergence after link failure took " << failure_rounds << " rounds.\n";
}