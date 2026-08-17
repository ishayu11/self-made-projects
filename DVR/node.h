#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

using namespace std;

const int INF = 999; // Define an infinity value for Poisoned Reverse

class RoutingEntry {
 public:
  string dstip, nexthop;
  string ip_interface;
  int cost;
};

class Comparator {
 public:
  bool operator()(const RoutingEntry &R1,const RoutingEntry &R2) {
    if (R1.cost == R2.cost) {
      return R1.dstip.compare(R2.dstip)<0;
    }
    else if(R1.cost > R2.cost) {
      return false;
    }
    else {
      return true;
    }
  }
};

struct routingtbl {
  vector<RoutingEntry> tbl;
};

class RouteMsg {
 public:
  string from;
  struct routingtbl *mytbl;
  string recvip;
};

class NetInterface {
 private:
  string ip;
  string connectedTo;
  int linkCost; // Added to support weighted links

 public:
  string getip() { return this->ip; }
  string getConnectedIp() { return this->connectedTo; }
  int getCost() { return this->linkCost; }
  void setip(string ip) { this->ip = ip; }
  void setConnectedip(string ip) { this->connectedTo = ip; }
  void setCost(int c) { this->linkCost = c; }
};

class Node {
 private:
  string name;
 protected:
  vector<pair<NetInterface, Node*> > interfaces;
  struct routingtbl mytbl;

  virtual bool recvMsg(RouteMsg* msg) {
    cout<<"Base"<<endl;
    return false;
  }

  bool isMyInterface(string eth) {
    for (int i = 0; i < interfaces.size(); ++i) {
      if(interfaces[i].first.getip() == eth)
	return true;
    }
    return false;
  }

 public:
  void setName(string name) { this->name = name; }

  void addInterface(string ip, string connip, Node *nextHop, int cost) {
    NetInterface eth;
    eth.setip(ip);
    eth.setConnectedip(connip);
    eth.setCost(cost);
    interfaces.push_back({eth, nextHop});
  }

  void addTblEntry(string myip, int cost) {
    RoutingEntry entry;
    entry.dstip = myip;
    entry.nexthop = myip;
    entry.ip_interface = myip;
    entry.cost = cost;
    mytbl.tbl.push_back(entry);
  }

  void updateTblEntry(string dstip, int cost) {
    for (int i=0; i<mytbl.tbl.size(); i++){
      if (mytbl.tbl[i].dstip == dstip)
        mytbl.tbl[i].cost = cost;
    }
    for(int i=0; i<interfaces.size(); ++i){
      if (interfaces[i].first.getConnectedIp() == dstip) {
        interfaces.erase(interfaces.begin() + i);
      }
    }
  }

  void updateInterfaceCost(string ip, string connip, int cost) {
    for(int i=0; i<interfaces.size(); ++i) {
      if(interfaces[i].first.getip() == ip && interfaces[i].first.getConnectedIp() == connip) {
        interfaces[i].first.setCost(cost);
      }
    }
  }

  string getName() { return this->name; }
  struct routingtbl getTable() { return mytbl; }

  void printTable() {
    Comparator myobject;
    sort(mytbl.tbl.begin(),mytbl.tbl.end(),myobject);
    cout<<this->getName()<<":"<<endl;
    for (int i = 0; i < mytbl.tbl.size(); ++i) {
      cout<<mytbl.tbl[i].dstip<<" | "<<mytbl.tbl[i].nexthop<<" | "<<mytbl.tbl[i].ip_interface<<" | "<<mytbl.tbl[i].cost <<endl;
    }
  }

  bool sendMsg() {
    bool networkChanged = false;
    for (int i = 0; i < interfaces.size(); ++i) {
      RouteMsg msg;
      msg.from = interfaces[i].first.getip();
      msg.recvip = interfaces[i].first.getConnectedIp();

      struct routingtbl ntbl;
      // Poisoned Reverse Logic
      for (int j = 0; j < mytbl.tbl.size(); ++j) {
        RoutingEntry curEntry = mytbl.tbl[j];
        if (curEntry.nexthop == msg.recvip) {
            curEntry.cost = INF; // Advertise cost as infinity
        }
        ntbl.tbl.push_back(curEntry);
      }

      msg.mytbl = &ntbl;
      if (interfaces[i].second->recvMsg(&msg)) {
        networkChanged = true;
      }
    }
    return networkChanged;
  }
};

class RoutingNode: public Node {
 public:
  bool recvMsg(RouteMsg *msg);
};