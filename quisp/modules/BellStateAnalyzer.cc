/** \file BellStateAnalyzer.cc
 *  \todo clean Clean code when it is simple.
 *  \todo doc Write doxygen documentation.
 *  \authors cldurand,takaakimatsuo
 *
 *  \brief BellStateAnalyzer
 */
#include <vector>
#include <omnetpp.h>
#include <classical_messages_m.h>
#include <PhotonicQubit_m.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <modules/HardwareMonitor.h>

using namespace omnetpp;
using namespace quisp::messages;

namespace quisp {
namespace modules {

/** \class BellStateAnalyzer BellStateAnalyzer.cc
 *  \todo Documentation of the class header.
 *
 *  \brief BellStateAnalyzer
 */
class BellStateAnalyzer : public cSimpleModule
{
    private:
        double darkcount_probability; // probability happning darkcount in each?
        double loss_rate; // photon loss rate
        double error_rate; // error rate of paulis
        //bool left_clicked;
        //bool right_click;
        bool left_last_photon_detected; // is photon detected in left channel
        bool right_last_photon_detected; // is photon detected in right channel
        bool send_result; // is seccess or fail ?
        double required_precision; //1.5ns (what is this for?)
        // left channels
        simtime_t left_arrived_at; // arrival time of right channel
        int left_photon_origin_node_address; // address of source node
        int left_photon_origin_qnic_address; // address of source qnic
        int left_photon_origin_qnic_type; // type of left source qnic (like packet type?)
        int left_photon_origin_qubit_address; // address of qubit
        bool left_photon_Xerr; // is there pauliX error in left channel
        bool left_photon_Zerr; // is there pauliZ error in left channel
        bool left_photon_lost; // is there photon lost in left channel
        stationaryQubit *left_statQubit_ptr; // left channel qubit?

        // right channels
        simtime_t right_arrived_at; // arrival time of left channel 
        int right_photon_origin_node_address; // address of source node
        int right_photon_origin_qnic_address; // address of source qnic
        int right_photon_origin_qnic_type; // type of address qnic
        int right_photon_origin_qubit_address; // address of source qubit
        bool right_photon_Xerr; // is there pauliX error in right channel 
        bool right_photon_Zerr; // is there pauliZ error in right channel
        bool right_photon_lost; // is there photon lost in right channel
        stationaryQubit *right_statQubit_ptr;

        int count_X=0, count_Y=0, count_Z=0, count_I=0, count_L=0, count_total=0;//for debug
        //bool handshake = false;
        bool this_trial_done = false; // ?
        double BSAsuccess_rate = 0.5 * 0.8 * 0.8; //detector probability = 0.8 success rate of Bell state analysis
        int left_count, right_count = 0; // detection counts?
        int DEBUG_darkcount_left = 0; // darkcount in left channel
        int DEBUG_darkcount_right = 0; // darkcount in right channel
        int DEBUG_darkcount_both = 0; // darcount in both channel
        int DEBUG_success = 0; // success count
        int DEBUG_total = 0; // ?
    protected:
        virtual void initialize(); // initialize bell state analyzer
        virtual void finish(); // finish analysing
        virtual void handleMessage(cMessage *msg);
        virtual bool isPhotonLost(cMessage *msg);
        virtual void forDEBUG_countErrorTypes(cMessage *msg);
        virtual void sendBSAresult(bool result, bool last); // sending bell state analysis
        virtual void initializeVariables(); // initialize all variables
        virtual void GOD_setCompletelyMixedDensityMatrix(); // make completely mixed dm
        virtual void GOD_updateEntangledInfoParameters_of_qubits(); // ?
};

Define_Module(BellStateAnalyzer);

/** BellStateAnalyzer::initialize
 *  constructor of variables
 * 
 * */ 
void BellStateAnalyzer::initialize(){
   darkcount_probability = par("darkcount_probability"); // get parameter "darkcount_probability"
   loss_rate = par("loss_rate"); // get parameter loss rate
   error_rate = par("error_rate"); // get parameter error rate
   //duration = par("duration");
   required_precision = par("required_precision"); // get parameter requiered precision
   left_arrived_at = -1; // address? why negative?
   right_arrived_at = -1; // same as left
   left_last_photon_detected = false; // is photon detected in left channel
   right_last_photon_detected = false; // is photon detected in right channel
   send_result = false; // sending result flag
   left_photon_origin_node_address = -1; // left node address
   left_photon_origin_qnic_address = -1; // left qnic address
   left_photon_origin_qubit_address = -1; // left qubit address
   left_photon_origin_qnic_type = -1; // left qnic type
   right_photon_origin_node_address = -1; // right node address
   right_photon_origin_qnic_address = -1; // right qnic address
   right_photon_origin_qubit_address = -1; // right qubit address
   right_photon_origin_qnic_type = -1; // right qnic type
   left_photon_Xerr = false; 
   left_photon_Zerr = false;
   right_photon_Xerr = false;
   right_photon_Zerr = false;
   left_statQubit_ptr = nullptr;
   right_statQubit_ptr = nullptr;
   right_photon_lost = false;
   left_photon_lost = false;
}

/*
 * Execute the BSA operation.
 * 
 * Input: msg is a "photon", with a few bits of info of its status.  msg arrives even if photon is lost; this msg will
 * indicate that.  Photon is assumed to be entangled with a stationary memory somewhere.  Photon has been updated to
 * include errors from the channel just prior to this call.  The memory does not need to be updated for memory
 * errors either before or during this operation; memory errors are only applied when gates are applied to that qubit or
 * when it is measured.
 * 
 * Output: none
 * Side effects: based on results of the BSA op, the density matrices of the two partner qubits are modified.
 * If the entanglement succeeds, each d.m. is updated with a pointer to its new entangled partner (our so-called "god channel"),
 * but the two d.m.s are _not_ merged into a single two-qubit d.m., allowing the sim to continue updating them
 * individually with errors as necessary.  If entanglement fails, the qubits are left independent.
 * Called twice: each photon arriving @BSA triggers this.  First one sets things provisionally using variables
 * local to this object (the BSA itself), second one completes and updates the actual qubits.
 */
void BellStateAnalyzer::handleMessage(cMessage *msg){
    PhotonicQubit *photon = check_and_cast<PhotonicQubit *>(msg);
    // if first photon is detected and first trial is done?
    if(photon->getFirst() && this_trial_done == true){//Next round started
        this_trial_done = false; // flip trial flag
        left_arrived_at = -1; // revert counter left
        right_arrived_at = -1; // revert counter right
        right_last_photon_detected = false; // final photon flag -> no (jusg first one)
        left_last_photon_detected = false; // same as right one
        send_result = false; // this is just first photon, do not send result
        right_count = 0; // why not right_count++?
        left_count = 0;
        EV<<"------------------Next round!\n";
        bubble("Next round!");
    }
    // from second photon to final?
    if(msg->arrivedOn("fromHoM_quantum_port$i", 0)){
        // this is for left detector (port:0)
        left_arrived_at = simTime(); // arrival time of photon
        left_photon_origin_node_address = photon->getNodeEntangledWith(); // call getNodeEntangleWith() (info which photon is entangle?)
        left_photon_origin_qnic_address = photon->getQNICEntangledWith(); // call getQNICEntangleWith() 
        left_photon_origin_qubit_address = photon->getStationaryQubitEntangledWith(); // call getStationaryQubitEntangledWith()
        left_photon_origin_qnic_type = photon->getQNICtypeEntangledWith(); // call getQNICtypeEntangleWith()
        left_statQubit_ptr = check_and_cast<stationaryQubit *>( photon->getEntangled_with()); // pointer for calling getEntangled_with
        left_photon_Xerr = photon->getPauliXerr(); // call getPauliXerr()
        left_photon_Zerr = photon->getPauliZerr(); // call getPauliZerr()
        left_photon_lost = photon->getPhotonLost(); //call getPhotonLost()
        //photon->setGODfree();
        if(photon->getFirst()){ // when the first photon is detected
            left_last_photon_detected = false;
            //send_result = false;
        }
        if(photon->getLast()){ // when the last photon is detected
            left_last_photon_detected = true;
            //send_result = true;
        }
        left_count++; // add count the number of detected photons
        //EV<<"Photon from Left arrived at = "<<simTime()<<"\n";
    }else if(msg->arrivedOn("fromHoM_quantum_port$i",1)){
        // this is for right detector (port:1)
        right_arrived_at = simTime();
        right_photon_origin_node_address = photon->getNodeEntangledWith();
        right_photon_origin_qnic_address = photon->getQNICEntangledWith();
        right_photon_origin_qubit_address = photon->getStationaryQubitEntangledWith();
        right_photon_origin_qnic_type = photon->getQNICtypeEntangledWith();
        //right_statQubit_ptr = photon->getEntangled_with();
        right_statQubit_ptr = check_and_cast<stationaryQubit *>( photon->getEntangled_with());
        right_photon_Xerr = photon->getPauliXerr();
        right_photon_Zerr = photon->getPauliZerr();
        right_photon_lost = photon->getPhotonLost();
        if(photon->getFirst()){
            right_last_photon_detected = false;
            //send_result = false;
        }
        if(photon->getLast()){
            right_last_photon_detected = true;
            //send_result = true;
        }
        right_count++;
        //EV<<"Right = "<<simTime()<<"\n";
    }else{
        error("This shouldn't happen....! Only 2 connections to the BSA allowed");
    }

    if((right_last_photon_detected || left_last_photon_detected)){
        // When both last photns are detected, send result!
        send_result = true;
    }

    //Just for debugging purpose
    forDEBUG_countErrorTypes(msg);

    double difference = (left_arrived_at-right_arrived_at).dbl();//Difference in arrival time
    //EV<<"!!!!!!!!!!!!!!!!!!!!!!!!!!this_trial_done == "<<this_trial_done<<"\n";
    if(this_trial_done == true){
        bubble("dumping result");
        //No need to do anything. Just ignore the BSA result for this shot 'cause the trial is over and photons will only arrive from a single node anyway.
        delete msg;
        return;
    }else if((left_arrived_at != -1 && right_arrived_at != -1) && std::abs(difference)<=(required_precision)){
        //Both arrived perfectly fine
        //bool lost = isPhotonLost(msg);

            double rand = dblrand(); //Even if we have 2 photons, whether we success entangling the qubits or not is probablistic. (entanglemnet prob)
            double darkcount_left = dblrand(); //darkcount probability of left detector
            double darkcount_right = dblrand(); // darkcount probability of right detector

            //  qubit loss || darkcounts right || darkcounts left || darkcounts both
            if((rand < BSAsuccess_rate && !right_photon_lost && !left_photon_lost) /*No qubit lost*/ || (!right_photon_lost && left_photon_lost && darkcount_left < darkcount_probability) /*Got rigt, darkcount left*/ || ( right_photon_lost && !left_photon_lost && darkcount_right < darkcount_probability) /*Got left, darkcount right*/ || ( right_photon_lost && left_photon_lost && darkcount_left < darkcount_probability && darkcount_right < darkcount_probability)/*Darkcount right left*/){
                // right photon detected but left darkcount
                if(!right_photon_lost && (left_photon_lost && darkcount_left <= darkcount_probability)){
                    //error("Dark count :)");
    				//std::cout<<"CM Entangling "<<left_statQubit_ptr->getFullName()<<" in "<<left_statQubit_ptr->getParentModule()->getFullName()<<"in node["<<left_statQubit_ptr->node_address<<"] with "<<right_statQubit_ptr->getFullName()<<" in "<<left_statQubit_ptr->getParentModule()->getFullName()<<"in node["<<right_statQubit_ptr->node_address<<"]\n";
                    DEBUG_darkcount_left++; // increment the number of darkcount in left detector
                    GOD_setCompletelyMixedDensityMatrix(); // completely mixed d.m.
                    sendBSAresult(false, send_result); // send fail info as a result

                //  left photon detected but right photon darkcount
                }else if(!left_photon_lost && (right_photon_lost && darkcount_right <= darkcount_probability)){
                    //error("Dark count :)");
    				//std::cout<<"CM Entangling "<<left_statQubit_ptr->getFullName()<<" in "<<left_statQubit_ptr->getParentModule()->getFullName()<<"in node["<<left_statQubit_ptr->node_address<<"] with "<<right_statQubit_ptr->getFullName()<<" in "<<left_statQubit_ptr->getParentModule()->getFullName()<<"in node["<<right_statQubit_ptr->node_address<<"]\n";
                    DEBUG_darkcount_right++; // increment the number of darkcount in right detector
                    GOD_setCompletelyMixedDensityMatrix(); // completely mixed d.m.
                    sendBSAresult(false, send_result); // send fail info as a result
                // Both of detector has darkcount
                }else if((left_photon_lost && darkcount_left <= darkcount_probability) &&  (right_photon_lost &&  darkcount_right <= darkcount_probability)){
                    //error("Dark count :)");
    				//std::cout<<"CM Entangling "<<left_statQubit_ptr->getFullName()<<" in "<<left_statQubit_ptr->getParentModule()->getFullName()<<"in node["<<left_statQubit_ptr->node_address<<"] with "<<right_statQubit_ptr->getFullName()<<" in "<<left_statQubit_ptr->getParentModule()->getFullName()<<"in node["<<right_statQubit_ptr->node_address<<"]\n";
                    DEBUG_darkcount_both++; // increment the number of darkcount of both detector
                    GOD_setCompletelyMixedDensityMatrix(); // completely mixed d.m.
                    sendBSAresult(false, send_result); // send fail info as a result
                // success (BSA success and no darkcount)
                }else{
                    bubble("Success...!"); // Success!
                    DEBUG_success++; // the number of success
                    GOD_updateEntangledInfoParameters_of_qubits(); // upgrade info parameters
                    sendBSAresult(false, send_result);//succeeded because both reached, and both clicked
                }

            }// we also need else if for darkcount.... TODO
            else{
                bubble("Failed...!"); // Fail!
                //EV<<"rand = "<<rand<<" <"<<BSAsuccess_rate;
                
    		   //std::cout<<"Failed Entangling "<<left_statQubit_ptr->getFullName()<<" in "<<left_statQubit_ptr->getParentModule()->getFullName()<<"in node["<<left_statQubit_ptr->node_address<<"] with "<<right_statQubit_ptr->getFullName()<<" in "<<right_statQubit_ptr->getParentModule()->getFullName()<<"in node["<<right_statQubit_ptr->node_address<<"]\n";
				
				sendBSAresult(true, send_result);//just failed because only 1 detector clicked while both reached (why true?)
            }DEBUG_total++;

        initializeVariables(); // initialize variable again
    // both detected but, the time duration is too much comparing to requirements
    }else if((left_arrived_at != -1 && right_arrived_at != -1) && std::abs(difference)>(required_precision)){
        //Both qubits arrived, but the timing was bad.
        bubble("Emission Timing Failed");
        initializeVariables(); // initialize
        sendBSAresult(true, send_result);
    // not detected yet?
    }else{
        bubble("Waiting...");
        /*if(photon->getLast() && (left_count == 0 || right_count == 0)){//This is wrong, because it will ignore the 2nd photon arrival if both of the photons are the first and last.
            //sendBSAresult(true,send_result);
            bubble("Ehhh");
            EV<<"left_count = "<<left_count<<", right_count = "<<right_count;
        }*/
        //Just waiting for the other qubit to arrive.
    }

    delete msg;
}
/** initializeVariables
 * 
 * brief: initialize all variables to initial
 * 
 * when one trial is done, this is called
*/
void BellStateAnalyzer::initializeVariables(){
    left_arrived_at = -1;
    left_photon_origin_node_address = -1;
    left_photon_origin_qnic_address = -1;
    left_photon_origin_qubit_address = -1;
    left_photon_origin_qnic_type = -1;
    right_arrived_at = -1;
    right_photon_origin_node_address = -1;
    right_photon_origin_qnic_address = -1;
    right_photon_origin_qubit_address = -1;
    right_photon_origin_qnic_type = -1;
    left_photon_lost = false;
    left_photon_lost = false;
    right_photon_Xerr = false;
    right_photon_Zerr = false;
    left_photon_Xerr = false;
    left_photon_Zerr = false;
    left_count = 0;
    right_count = 0;
    left_statQubit_ptr = nullptr;
    right_statQubit_ptr = nullptr;
}

/** sendBSAresult()
 * 
 * brief: send bell state analysis result
 * 
 * input: 
 *  bool result (the result of BSA success or fail)
 *  bool sendresult (if send result or not)
 * */ 
void BellStateAnalyzer::sendBSAresult(bool result, bool sendresults){
    //result could be false positive (actually ok but recognized as ng),
    //false negative (actually ng but recognized as ok) due to darkcount
    //true positive and true negative is no problem.
    //std::cout<<"send?="<<sendresults<<"___________________________________\n";
    // if don't send result,
    if(!sendresults){
        // create packet for BSA result
        BSAresult *pk = new BSAresult;
    	//std::cout<<"send result to HoM___\n";
        pk->setEntangled(result); // call setEntangled
        send(pk, "toHoMController_port"); // send?
    }else{//Was the last photon. End pulse detected.
        //  create packet for telling bsa finish
        BSAfinish *pk = new BSAfinish();
        // black visual packet
        pk->setKind(7);
    	//std::cout<<"send last result to HoM___\n";
        pk->setEntangled(result);
        send(pk, "toHoMController_port");
        bubble("trial done now");
        this_trial_done = true;
        //EV<<"!!!!!!!!!!!!!!!over!!!!!!!!!!!this_trial_done == "<<this_trial_done<<"\n";
    }
}

void BellStateAnalyzer::finish(){
    std::cout<<"total = "<<DEBUG_total<<"\n";
    std::cout<<"Success = "<<DEBUG_success<<"\n";
    std::cout<<"darkcount_count_left = "<<DEBUG_darkcount_left<<", darkcount_count_right ="<<DEBUG_darkcount_right<<", darkcount_count_both = "<<DEBUG_darkcount_both<<"\n";
}

void BellStateAnalyzer::forDEBUG_countErrorTypes(cMessage *msg){
    PhotonicQubit *q = check_and_cast<PhotonicQubit *>(msg);
    if(q->getPauliXerr() && q->getPauliZerr()){
        count_Y++;
    }else if(q->getPauliXerr() && !q->getPauliZerr()){
        count_X++;
    }else if(!q->getPauliXerr() && q->getPauliZerr()){
        count_Z++;
    // if there are photon loss
    }else if(q->getPhotonLost()){
        count_L++;
    }else{
        count_I++;
    }count_total++;
    EV<<"Y%="<<(double)count_Y/(double)count_total<<", X%="<<(double)count_X/(double)count_total<<", Z%="<<(double)count_Z/(double)count_total<<", L%="<<(double)count_L/(double)count_total<<", I% ="<<(double)count_I/(double)count_total<<"\n";
}
//  is there are photon lost or not
bool BellStateAnalyzer::isPhotonLost(cMessage *msg){
    PhotonicQubit *q = check_and_cast<PhotonicQubit *>(msg);
    if(q->getPhotonLost()){
        return true; //Lost
    }else{
        return false;
    }
    delete msg;
}

// set completely mixed density matrix into both qubit
void BellStateAnalyzer::GOD_setCompletelyMixedDensityMatrix(){
    //error("Hrtr");
	//std::cout<<"Darkcount CM "<<left_statQubit_ptr<<", "<<right_statQubit_ptr<<"\n";
    left_statQubit_ptr->setCompletelyMixedDensityMatrix();
    right_statQubit_ptr->setCompletelyMixedDensityMatrix();
}

/*Error on flying qubit with a successful BSA propagates to its original stationary qubit. */
void BellStateAnalyzer:: GOD_updateEntangledInfoParameters_of_qubits(){

    //std::cout<<"Entangling "<<left_statQubit_ptr->getFullName()<<" in "<<left_statQubit_ptr->getParentModule()->getFullName()<<"in node["<<left_statQubit_ptr->node_address<<"] with "<<right_statQubit_ptr->getFullName()<<" in "<<right_statQubit_ptr->getParentModule()->getFullName()<<"in node["<<right_statQubit_ptr->node_address<<"]\n";

    left_statQubit_ptr->setEntangledPartnerInfo(right_statQubit_ptr);
    if(left_photon_Xerr){//If Photon had an X error
        left_statQubit_ptr->addXerror();//Add X error to the stationary qubit.
    }
    if(left_photon_Zerr){
        left_statQubit_ptr->addZerror();
    }

    right_statQubit_ptr->setEntangledPartnerInfo(left_statQubit_ptr);
    if(right_photon_Xerr){
        right_statQubit_ptr->addXerror();
    }
    if(right_photon_Zerr){
        right_statQubit_ptr->addZerror();
    }
    // if either right or left of entanglement flag is null pointer
    if(right_statQubit_ptr->entangled_partner==nullptr || left_statQubit_ptr->entangled_partner==nullptr){
        std::cout<<"Entangling failed\n";
        error("Entangling failed");
    }
    //std::cout<<right_statQubit_ptr<<", node["<<right_statQubit_ptr->node_address<<"] from qnic["<<right_statQubit_ptr->qnic_index<<"]\n";
    //std::cout<<(bool)(right_statQubit_ptr->entangled_partner==nullptr)<<" Right Entangled if ("<<false<<")\n";
    //std::cout<<left_statQubit_ptr<<", node["<<left_statQubit_ptr->node_address<<"] from qnic["<<left_statQubit_ptr->qnic_index<<"]\n";
   // std::cout<<(bool)(left_statQubit_ptr->entangled_partner==nullptr)<<" Left Entangled if ("<<false<<")\n";
}

} // namespace modules
} // namespace quisp
