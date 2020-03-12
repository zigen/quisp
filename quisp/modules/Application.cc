/** \file Application.cc
 *  \todo clean Clean code when it is simple.
 *  \todo doc Write doxygen documentation.
 *  \authors cldurand,takaakimatsuo
 *  \date 2018/03/14
 *
 *  \brief Application
 */
#include <vector>
#include <omnetpp.h>
#include <classical_messages_m.h>

using namespace omnetpp;
using namespace quisp::messages; //?

namespace quisp {
namespace modules {

/** \class Application Application.cc
 *
 *  \brief Application
 */
class Application : public cSimpleModule{
    private:
        int myAddress; // identification
        cMessage *generatePacket; /**< Not the actual packet.
                                    Local message to invoke Events */
        //cPar *sendIATime;
        //bool isBusy; /**< Already requested a path selection
        //               for a Quantum app */

        int* Addresses_of_other_EndNodes = new int[1]; // address of counterpart
        int num_of_other_EndNodes; // The number of devices of endnodes
        bool EndToEndConnection; // is this a end to end connection or not
    protected:
        virtual void initialize() override; // override explicitly from cSimplemodule
        virtual void handleMessage(cMessage *msg) override; // virtal member function of cSimplemodule
        virtual void BubbleText(const char* txt); // This is for gui text (bubble is visualized photon packet)
        virtual int* storeEndNodeAddresses(); // storing end node address
        virtual int getOneRandomEndNodeAddress(); // get one random end onde address
        virtual cModule* getQNode(); // getting Qnode module?

    public:
        Application();
        int getAddress();
};

Define_Module(Application);

Application::Application(){
    // null pointing generate Packet initially
    generatePacket = nullptr;
}

/**
 * \brief Initialize module.
 *
 * If we're not in and end node, this module is not necessary.
 */
void Application::initialize(){
    cGate *toRouterGate = gate("toRouter"); // which other they are connected and what are the channel objects associated with the links(from omnetpp doc)
    if(!toRouterGate->isConnected()){
        //Since we only need this module in EndNode, delete it otherwise.
        deleteThisModule *msg = new deleteThisModule;
        scheduleAt(simTime(),msg); // scheduling an event
    }else{
        myAddress = getParentModule()->par("address"); // pointer getParentModule pointing address parameter
        EndToEndConnection = par("EndToEndConnection"); // End to End boolian
        Addresses_of_other_EndNodes = storeEndNodeAddresses(); // int of address other end nodes

        //cModule *qnode = getQNode();
        if(myAddress == 1 && EndToEndConnection){//hard-coded for now TODO
            int endnode_destination_address = getOneRandomEndNodeAddress(); // getting random address as a distination
            EV<<"Connection setup request will be sent from"<<myAddress<<" to "<<endnode_destination_address<<"\n"; // event log 
            ConnectionSetupRequest *pk = new ConnectionSetupRequest(); // make connection setup request 

            pk->setActual_srcAddr(myAddress); // calling setActual_srtAddr: setting source address
            pk->setActual_destAddr(endnode_destination_address); // setting destination address
            pk->setDestAddr(myAddress); // 
            pk->setSrcAddr(myAddress); // 
            pk->setKind(7); // the type of packet
            scheduleAt(simTime(),pk); // schedule packet
        }
    }
}

void Application::handleMessage(cMessage *msg){

    if(dynamic_cast<deleteThisModule *>(msg) != nullptr){
        // if delete this modle is triggerd?
        deleteModule();
        //  delete module and message
        delete msg;
    }else if(dynamic_cast<ConnectionSetupRequest *>(msg) != nullptr){
        // if got connection setup request
        EV<<"got setup request!\n";
        send(msg, "toRouter"); // sent message to destination
        // setup response is in RuleEngine because it include swapping and application ruleset
    }else{
        // other type packet
        delete msg;
        error("Application not recognizing this packet");
    }

    /*if(msg == generatePacket){
        header *pk = new header("PathRequest");
        pk->setSrcAddr(1);//packet source setting
        pk->setDestAddr(3);//packet destination setting
        pk->setKind(1);
        send(pk, "toRouter");//send to port out. connected to local routing module (routing.localIn).
        scheduleAt(simTime() + sendIATime->doubleValue(), generatePacket);
        //scheduleAt(simTime() + 10, generatePacket);//In 10 seconds, another msg send gets invoked
    }
    else if(msg->getKind()==1 && strcmp("PathRequest", msg->getName())==0){
        BubbleText("Path Request received!");

        EV << "Deleting path request\n";
    }
    else{//A message was reached from another node to here
        delete msg;
        //cModule *mod = getSimulation()->getModule(4);
        //int ad = mod->par("address");
        //QNode *aa = check_and_cast<QNode*>(mod);//Cast not working
        //EV<<"------------------------------"<<mod->getModuleType()<<"\n";

        EV << "Deleting msg\n";
    }*/
}

/* storeEndNodeAddress
 * 
 * brief: string end node address
 * 
*/
int* Application::storeEndNodeAddresses(){
    
    cTopology *topo = new cTopology("topo"); // cTopology is for routing in communiation networks
    topo->extractByParameter("nodeType", getParentModule()->par("nodeType").str().c_str()); //like topo.extractByParameter("nodeType","EndNode")
    num_of_other_EndNodes = topo->getNumNodes()-1; // the number of endnodes besides me (src)
    Addresses_of_other_EndNodes = new int[num_of_other_EndNodes]; // address of other endnodes

    int index = 0; // init index
    // topo calling getNumNoodes() which is the member func of cTopology
    for (int i = 0; i < topo->getNumNodes(); i++) {
        cTopology::Node *node = topo->getNode(i);
        EV<<"\n\n\nEnd node address is "<<node->getModule()->par("address").str()<<"\n";

        // if the node number if not mine, 
        if((int) node->getModule()->par("address") != myAddress){
            // then, substitute address to address of other endnodes index
            Addresses_of_other_EndNodes[index] = (int) node->getModule()->par("address");
            EV<<"\n Is it still "<<node->getModule()->par("address").str()<<"\n";
            index++;
        }
     }

     //Just so that we can see the data from the IDE
     std::stringstream ss; // more understandable name
     for(int i=0; i < num_of_other_EndNodes; i++){
        // add endnodes name to ss
         ss << Addresses_of_other_EndNodes[i] <<", ";
     }
     std::string s = ss.str();
     par("Other_endnodes_table") = s;
     delete topo;
     return Addresses_of_other_EndNodes;
}

/** getOneRandomEndNodeAddress()
 * 
 * brief: returning random node address in network
 * 
 * output: returning endnode address(int)
*/
int Application::getOneRandomEndNodeAddress(){
    int random_index = intuniform(0,num_of_other_EndNodes-1);
    return Addresses_of_other_EndNodes[random_index];
}

/** BubbleText()
 * 
 * brief: text for describing gui packet
 * This function is only for GUI environment
*/
void Application::BubbleText(const char* txt){
    if (hasGUI()) {
      char text[32];
      sprintf(text, "%s", txt);
      bubble(text);
    }
}

/** getAddress()
 * 
 * brief: get my address
*/
int Application::getAddress()
{
    return myAddress;
}

/** getQNode()
 * 
 * brief: get crrent module
*/
cModule* Application::getQNode(){
    cModule *currentModule = getParentModule();//We know that Connection manager is not the QNode, so start from the parent.
    try {
        cModuleType *QNodeType = cModuleType::get("networks.QNode");//Assumes the node in a network has a type QNode
        while(currentModule->getModuleType()!=QNodeType){
            
            currentModule = currentModule->getParentModule();
        }
    } catch (std::exception& e) {
        error("No module with QNode type found. Have you changed the type name in ned file?");
        endSimulation();
    }
    return currentModule;
}

} // namespace modules
} // namespace quisp
