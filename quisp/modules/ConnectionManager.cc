/** \file ConnectionManager.cc
 *  \todo clean Clean code when it is simple.
 *  \todo doc Write doxygen documentation.
 *  \authors cldurand,takaakimatsuo
 *
 *  \brief ConnectionManager
 */
#include <vector>
#include <omnetpp.h>
#include "RoutingDaemon.h"
#include "HardwareMonitor.h"
#include <rules/RuleSet.h>
#include <classical_messages_m.h>

using namespace omnetpp;
using namespace quisp::messages;
using namespace quisp::rules;

namespace quisp {
namespace modules {

/** \class ConnectionManager ConnectionManager.cc
 *  \todo Documentation of the class header.
 *
 *  \brief ConnectionManager
 */
class ConnectionManager : public cSimpleModule{
  private:
        int myAddress; // source address
        RoutingDaemon *routingdaemon; // from routing daemon
        HardwareMonitor *hardwaremonitor; // from hardware monitor
  protected:
        virtual void initialize() override; 
        virtual void handleMessage(cMessage *msg) override; 
        // generating RuleSet for entanglement swapping
        // virtual RuleSet* generateRuleSet_EntanglementSwapping(unsigned int RuleSet_id, int owner, int left_node, int right_node);
        virtual RuleSet* generateRuleSet_EntanglementSwapping(unsigned int RuleSet_id, int owner, int left_node, QNIC_type lqnic_type, int lqnic_id, int lresource, int right_node, QNIC_type rqnic_type, int rqnic_id, int rresource);
        // create unique id with hash or something?
        virtual unsigned long createUniqueId();
};

Define_Module(ConnectionManager);

// initialize
void ConnectionManager::initialize(){
  EV<<"ConnectionManager booted\n";
  cModule *rd = getParentModule()->getSubmodule("rd");
  routingdaemon = check_and_cast<RoutingDaemon *>(rd);
  cModule *hm = getParentModule()->getSubmodule("hm");
  hardwaremonitor = check_and_cast<HardwareMonitor *>(hm);
  myAddress = par("address");
}



unsigned long ConnectionManager::createUniqueId(){
    std::string time = SimTime().str();
    std::string address = std::to_string(myAddress);
    std::string random = std::to_string(intuniform(0,10000000));
    std::string hash_seed = address+time+random;
    std::hash<std::string> hash_fn;
    size_t  t = hash_fn(hash_seed);
    unsigned long RuleSet_id = static_cast<long>(t);
    std::cout<<"Hash is "<<hash_seed<<", t = "<<t<<", long = "<<RuleSet_id<<"\n";
    return RuleSet_id;
}



// might just be 2*l
static int compute_path_division_size (int l /**< number of links (path length, number of nodes -1) */) {
  if (l > 1) {
    int hl = (l>>1);
    return compute_path_division_size(hl) + compute_path_division_size(l-hl) + 1;
  }
  return l;
}

/** Treat subpath [i:...] of length l */
static int fill_path_division (int * path /**< Nodes on the connection setup path */,
    int i /**< Left of the subpath to consider */, int l /**< Length of the subpath */,
    int * link_left /**< Left part of the list of "links" */, int * link_right /**< Right part */,
    int * swapper /**< Swappers to create those links (might be -1 for real links) */,
    int fill_start /** [0:fill_start[ is already filled */) {
  if (l > 1) {
    int hl = (l>>1);
    fill_start = fill_path_division (path, i, hl, link_left, link_right, swapper, fill_start);
    fill_start = fill_path_division (path, i+hl, l-hl, link_left, link_right, swapper, fill_start);
    swapper[fill_start] = path[i+hl];
  }
  if (l > 0) {
    link_left[fill_start] = path[i];
    link_right[fill_start] = path[i+l];
    if (l == 1) swapper[fill_start] = -1;
    fill_start++;
  }
  return fill_start;
}

void ConnectionManager::handleMessage(cMessage *msg){

  if(dynamic_cast<ConnectionSetupRequest *>(msg)!= nullptr){
    ConnectionSetupRequest *pk = check_and_cast<ConnectionSetupRequest *>(msg);

    int actual_dst = pk->getActual_destAddr(); // the number of destination address (e.g.3)
    
    if(actual_dst == myAddress){// myAddress is current position of packet
      // FIXME: is the destination in the stack? Should I add it manually in the end?
      //In Destination node

      // hop_count: number of nodes on the path excluding me (destination node)
      // Should be the same as pk->getStack_of_linkCostsArraySize()
      // Let's store the path in an array to limit indirections
      int hop_count = pk->getStack_of_QNodeIndexesArraySize();
      int * path = new int[hop_count+1];

      // FIXME: maybe it's better to have a map, indexed by node addresses
      // ruleset map(for all nodes)
      // RuleSet **rulesets = new RuleSet * [hop_count];
      for (int i = 0; i<hop_count; i++) {
        path[i] = pk->getStack_of_QNodeIndexes(i);
        // TODO: initialize rulsets
        // The RuleSet class needs to store the address of the node it is being sent to.
        // rulesets[i] = new RuleSet(path[i]);// how to write like this
        EV << "     Qnode on the path => " << path[i] << std::endl;
        }
        path[hop_count] = myAddress;
        // rulesets[hop_count] = new RuleSet(myAddress);
        EV << "Last Qnode on the path => " << path[hop_count] << std::endl;
        // std::map<int, RuleSet*>rulesets = {};
        for (int ind = 0; ind<=hop_count; ind++){
          EV<<"path!!!!!!!"<<path[ind]<<"\n";
        }
        // Number of division elements
        int divisions = compute_path_division_size(hop_count);

        int *link_left = new int[divisions],
            *link_right = new int[divisions],
            *swapper = new int[divisions];
        // fill_path_division should yield *exactly* the anticipated number of divisions
        if (fill_path_division(path, 0, hop_count, link_left, link_right, swapper, 0) < divisions){
             error("Something went wrong in path division computation.");
        }

        /* TODO: Remember you have link costs <3
        for(int i = 0; i<hop_count; i++){
            //The link cost is just a dummy variable (constant 1 for now and how it is set in a bad way (read from the channel but from only 1 channels from Src->BSA and ignoring BSA->Dest).
            //If you need to test with different costs, try changing the value.
            //But we need to implement actual link-tomography for this eventually.
            EV<<"\nThis is one of the stacked link costs....."<<pk->getStack_of_linkCosts(i)<<"\n";
        }
        */
        //Change int to RuleSet*
        std::map<std::string, RuleSet*> rulesets = {};
        QNIC_id_pair qnic_pairs;
        for (int i=0; i<divisions; i++) {
          if(swapper[i]>0){
            EV<<"generate ruleset for " << swapper[i]<<"\n";
            // 1. Create swapping rules for swapper[i]
            // Rule * swaprule = new SwapRule(...);
            // TODO: IMPLEMENT SwapRule that will have two FidelityClause to
            // check the fidelity of the qubit, and one SwapAction.
            // rulesets[i] = EntanglementSwapping_Ruleset;
            // rulesets->setRuleSet(EntanglementSwapping_Ruleset);
            RuleSet* EntanglementSwapping_Ruleset = this->generateRuleSet_EntanglementSwapping(createUniqueId(), swapper[i], link_left[i], qnic_pairs.fst.type,qnic_pairs.fst.index, 1, link_right[i], qnic_pairs.snd.type, qnic_pairs.snd.index, 1);
            rulesets[std::to_string(swapper[i])] = EntanglementSwapping_Ruleset;
            // send this to swapper node.
            ConnectionSetupResponse *pkr = new ConnectionSetupResponse;
            pkr->setDestAddr(swapper[i]);
            pkr->setSrcAddr(myAddress);
            pkr->setKind(2);
            pkr->setRuleSet(rulesets[std::to_string(swapper[i])]);
            // this might not correct
            EV<<"set ruleset to packet!"<<"\n";
            send(pkr, "RouterPort$o");
            EV<<"sent!"<<"\n";
          }else{
            // not swapper
          }
        }                
        // Go over every division
        for (int i=0; i<divisions; i++) {
          if (swapper[i]>0){
            EV << "Division: " << link_left[i] << " ---( " << swapper[i] << " )--- " << link_right[i]<< std::endl;
          }else{
            EV << "Division: " << link_left[i] << " -------------- " << link_right[i] << std::endl;
          }
        }
            // 2. Create swapping tracking rules for link_left[i] and link_right[i]
            //    Right now, those might be empty. In the end they are used to make sure that the left and right
            //    nodes are correctly tracking the estimation of the state of the qubits that get swapped.

        // Whatever happens, this 'i' line is also a 'link' relationship
        // 3. Create the purification rules for link_left[i] and link_right[i]
        // Rule * purifyrule = ...;
        // Rule * discardrule = ...; // do we need it or is it hardcoded?
        // ruleset_of_link_left_i.rules.append(purifyrule);
        // ...
//#endif
      //Packet returning Rule sets
      //Rule set includes Objects composed of clauses and actions.
      //Clauses and actions have functions, like check and
      /**\todo
      * Document how to use Rules, and it would be helpful if you could implement the part where it actually uses it.
      * Dummy non-practical usage example...
      * (For example, do fidelity check and do swap between two local qubits in a single qnic.)
      * Create the response
      * Full 1yr email-support (maybe tele-communication too).
      * Psychological support. Financial support.
      */
      // error("Yay!");
      delete msg;
      return;
    }else{
      EV<<"Dist is different from my Address! actual_dist :"<<actual_dst<<"\n";
      EV<<"This packet is from "<<myAddress<<" to "<<actual_dst<<"\n";
      int local_qnic_address_to_actual_dst = routingdaemon->return_QNIC_address_to_destAddr(actual_dst);
      if(local_qnic_address_to_actual_dst==-1){//is not found
          error("QNIC to destination not found");
        }else{
          //Use the QNIC address to find the next hop QNode, by asking the Hardware Monitor (neighbor table).
          EV<<"\n"<<local_qnic_address_to_actual_dst<<"|||||||||||||||||||||||||||||||||||||||||||||||||\n";
          connection_setup_inf dst_inf = hardwaremonitor->return_setupInf(local_qnic_address_to_actual_dst);
          EV << "DST_INF " << dst_inf.qnic.type << "," << dst_inf.qnic.index << "\n";
          connection_setup_inf src_inf = hardwaremonitor->return_setupInf(pk->getSrcAddr());
          EV << "SRC_INF " << src_inf.qnic.type << "," << src_inf.qnic.index << "\n";

          // accumulate info
          int num_accumulated_nodes = pk->getStack_of_QNodeIndexesArraySize();
          int num_accumulated_costs = pk->getStack_of_linkCostsArraySize();
          int num_accumulated_pair_info = pk->getStack_of_QNICsArraySize();

          //Update information and send it to the next Qnode.
          pk->setDestAddr(dst_inf.neighbor_address);
          pk->setSrcAddr(myAddress);
          pk->setStack_of_QNodeIndexesArraySize(num_accumulated_nodes+1);
          pk->setStack_of_linkCostsArraySize(num_accumulated_costs+1);
          pk->setStack_of_QNodeIndexes(num_accumulated_nodes, myAddress);
          pk->setStack_of_linkCosts(num_accumulated_costs, dst_inf.quantum_link_cost);
          pk->setStack_of_QNICsArraySize(num_accumulated_pair_info+1);
          QNIC_id_pair pair_info = {
              .fst = src_inf.qnic,
              .snd = dst_inf.qnic
          };
          pk->setStack_of_QNICs(num_accumulated_pair_info, pair_info);
          // pk->setStack_of_QNICs(pair_info);
          pair_info = pk->getStack_of_QNICs(num_accumulated_pair_info);
          EV << "PAIR_INF " << pair_info.fst.type << "," << pair_info.fst.index << " : " << pair_info.snd.type << "," << pair_info.snd.index << "\n";
          send(pk,"RouterPort$o");
        }
      }
  }else if (dynamic_cast<ConnectionSetupResponse *>(msg)!= nullptr){
    // write handling pattern when node get setup request ack
  }
}

/**generateRuleSet_EntanglementSwapping
 * 
 * brief: generating entanglement swapping rule for swappers
 * Practically, this is just swapping the pointers for left node target and right node target. 
*/
// RuleSet* ConnectionManager::generateRuleSet_EntanglementSwapping(unsigned int RuleSet_id, int owner, int left_node, int right_node){
  RuleSet* ConnectionManager::generateRuleSet_EntanglementSwapping(unsigned int RuleSet_id, int owner, int left_node, QNIC_type lqnic_type, int lqnic_id, int lresource, int right_node, QNIC_type rqnic_type, int rqnic_id, int rresource){
    EV<<"Making entanglement swapping rule!"<<"\n";
    int rule_index = 0; // first rule

    RuleSet* EntanglementSwapping = new RuleSet(RuleSet_id, owner, left_node, right_node); // Create empty ruleset for entanglement swapping

    // actual rules
    Rule* EnSWAP = new Rule(RuleSet_id, rule_index); // empty rule for entanglement swapping
    Condition* EnSWAP_condition = new Condition(); // trigger for entanglement swapping
    Clause* resource_clause_left = new EnoughResourceClauseLeft(1); // condition clause?
    Clause* resource_clause_right = new EnoughResourceClauseRight(1); // freeing resource in this func?

    // Conditions for entanglement swapping (we have enough resources on right and left)
    EnSWAP_condition->addClause(resource_clause_left);
    EnSWAP_condition->addClause(resource_clause_right);
    EnSWAP->setCondition(EnSWAP_condition); 

    // Actions for entanglement swapping
    quisp::rules::Action* enswap_action = new SwappingAction(left_node, lqnic_type, lqnic_id, lresource, right_node, rqnic_type, rqnic_id, rresource);
    EnSWAP->setAction(enswap_action);
    EntanglementSwapping->addRule(EnSWAP); // we need to return the result of bsa
    rule_index++;

    // After entanglement swapping, apply tomography.
    Rule *Tomography = new Rule(RuleSet_id, rule_index); // initialize rules
    Condition* Tomography_condition = new Condition(); // tomography between 


    return EntanglementSwapping;
}

} // namespace modules
} // namespace quisp
