#include "node.h"
#include <iostream>

using namespace std;

void printRT(vector<RoutingNode*> nd) {
  for (int i = 0; i < nd.size(); i++) {
    nd[i]->printTable();
  }
}

int routingAlgo(vector<RoutingNode*> nd) {
  bool saturation = false;
  int rounds = 0;

  // Continue running until tables no longer change
  while (!saturation && rounds < 100) {
    saturation = true;
    for (RoutingNode* node : nd) {
      if (node->sendMsg()) {
        saturation = false;
      }
    }
    rounds++;
  }

  printf("Printing the routing tables after the convergence \n");
  printRT(nd);
  return rounds;
}

bool RoutingNode::recvMsg(RouteMsg *msg) {
  routingtbl *recvRoutingTable = msg->mytbl;

  // Find link cost
  int link_cost = 1;
  for (int i = 0; i < interfaces.size(); ++i) {
      if (interfaces[i].first.getip() == msg->recvip && interfaces[i].first.getConnectedIp() == msg->from) {
          link_cost = interfaces[i].first.getCost();
          break;
      }
  }

  bool tableChanged = false;

  for (RoutingEntry entry : recvRoutingTable->tbl) {
    bool entryExists = false;
    int new_cost = entry.cost + link_cost;
    if (new_cost > INF) new_cost = INF;

    for (int i = 0; i < mytbl.tbl.size(); ++i) {
      if (mytbl.tbl[i].dstip == entry.dstip) {
        entryExists = true;

        // Update if route strictly uses Y as next hop OR if we found a strictly better path
        if (mytbl.tbl[i].nexthop == msg->from || new_cost < mytbl.tbl[i].cost) {
            if (mytbl.tbl[i].cost != new_cost || mytbl.tbl[i].nexthop != msg->from) {
                mytbl.tbl[i].cost = new_cost;
                mytbl.tbl[i].nexthop = msg->from;
                tableChanged = true;
            }
        }
      }
    }

    if (!entryExists && new_cost < INF) {
      RoutingEntry newEntry;
      newEntry.dstip = entry.dstip;
      newEntry.nexthop = msg->from;
      newEntry.ip_interface = msg->recvip;
      newEntry.cost = new_cost;
      mytbl.tbl.push_back(newEntry);
      tableChanged = true;
    }
  }
  return tableChanged;
}