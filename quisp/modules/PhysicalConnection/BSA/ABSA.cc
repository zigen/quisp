/** \file ABSA.cc
This file explains the behaviour of the ABSA node
*/

#include <vector>
#include <omnetpp.h>
#include <classical_messages_m.h>
#include <PhotonicQubit_m.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

using namespace omnetpp;
using namespace quisp::messages;

namespace quisp {
namespace modules {

/** \class ABSA */

class ABSA : public cSimpleModule
{
	/* check if we need the same parameters, less or more */
private:
        // for performance analysis
        int n_res = 0;
        simsignal_t GOD_num_resSignal;
        double darkcount_probability;
        double loss_rate;
        double error_rate;
        bool left_last_photon_detected;
        bool right_last_photon_detected;
        bool send_result;
        double required_precision;//1.5ns
        simtime_t left_arrived_at;
        int left_photon_origin_node_address;
        int left_photon_origin_qnic_address;
        int left_photon_origin_qnic_type;
        int left_photon_origin_qubit_address;
        bool left_photon_Xerr;
        bool left_photon_Zerr;
        StationaryQubit *left_statQubit_ptr;
        simtime_t right_arrived_at;
        int right_photon_origin_node_address;
        int right_photon_origin_qnic_address;
        int  right_photon_origin_qnic_type;
        int right_photon_origin_qubit_address;
        bool right_photon_Xerr;
        bool right_photon_Zerr;
        bool right_photon_lost;
        bool left_photon_lost;
        StationaryQubit *right_statQubit_ptr;
        int count_X=0, count_Y=0, count_Z=0, count_I=0, count_L=0, count_total=0;//for debug
        bool this_trial_done = false;
        double ABSAsuccess_rate = 0.5 * 0.8 * 0.8; //detector probability = 0.8
        int left_count, right_count = 0;
        int DEBUG_darkcount_left = 0;
        int DEBUG_darkcount_right = 0;
        int DEBUG_darkcount_both = 0;
        int DEBUG_success = 0;
        int DEBUG_total = 0;
    protected:
        virtual void initialize(); // this one handles the variables in the ned file
        virtual void finish();
        virtual void handleMessage(cMessage *msg); // the msg is the photons
        virtual bool isPhotonLost(cMessage *msg);
        virtual void forDEBUG_countErrorTypes(cMessage *msg);
        virtual void sendABSAresult(bool result, bool last);
        virtual void initializeVariables();
        virtual void GOD_setCompletelyMixedDensityMatrix();
        virtual void GOD_updateEntangledInfoParameters_of_qubits();
        // virtual void PauliCorrection(); //what are potential inputs
        virtual void singleQubitMeasure(StationaryQubit * qubit); //How to add the qubit pointer
        
};

Define_Module(ABSA);

void ABSA::initialize()
{
   GOD_num_resSignal = registerSignal("Num_ABSA");
   darkcount_probability = par("darkcount_probability");
   loss_rate = par("loss_rate");
   error_rate = par("error_rate");
   required_precision = par("required_precision");
   left_arrived_at = -1;
   right_arrived_at = -1;
   left_last_photon_detected = false;
   right_last_photon_detected = false;
   send_result = false;
   left_photon_origin_node_address = -1;
   left_photon_origin_qnic_address = -1;
   left_photon_origin_qubit_address = -1;
   left_photon_origin_qnic_type = -1;
   right_photon_origin_node_address = -1;
   right_photon_origin_qnic_address = -1;
   right_photon_origin_qubit_address = -1;
   right_photon_origin_qnic_type = -1;
   left_photon_Xerr = false;
   left_photon_Zerr = false;
   right_photon_Xerr = false;
   right_photon_Zerr = false;
   left_statQubit_ptr = nullptr;
   right_statQubit_ptr = nullptr;
   right_photon_lost = false;
   left_photon_lost = false;
}

void ABSA::handleMessage(cMessage *msg) {
  PhotonicQubit *photon = check_and_cast<PhotonicQubit *>(msg);
  if (photon->getFirst() && this_trial_done == true) {  // Next round started
    this_trial_done = false;
    left_arrived_at = -1;
    right_arrived_at = -1;
    right_last_photon_detected = false;
    left_last_photon_detected = false;
    send_result = false;
    right_count = 0;
    left_count = 0;
    EV << "------------------Next round!\n";
    bubble("Next round!");
  }

  if (msg->arrivedOn("fromHoM_quantum_port$i", 0)) {
    left_arrived_at = simTime();
    left_photon_origin_node_address = photon->getNodeEntangledWith();
    left_photon_origin_qnic_address = photon->getQNICEntangledWith();
    left_photon_origin_qubit_address = photon->getStationaryQubitEntangledWith();
    left_photon_origin_qnic_type = photon->getQNICtypeEntangledWith();
    left_statQubit_ptr = check_and_cast<StationaryQubit *>(photon->getEntangled_with());
    left_photon_Xerr = photon->getPauliXerr();
    left_photon_Zerr = photon->getPauliZerr();
    left_photon_lost = photon->getPhotonLost();
    if (photon->getFirst()) {
      left_last_photon_detected = false;
    }
    if (photon->getLast()) {
      left_last_photon_detected = true;
    }
    left_count++;
  } else if (msg->arrivedOn("fromHoM_quantum_port$i", 1)) {
    right_arrived_at = simTime();
    right_photon_origin_node_address = photon->getNodeEntangledWith();
    right_photon_origin_qnic_address = photon->getQNICEntangledWith();
    right_photon_origin_qubit_address = photon->getStationaryQubitEntangledWith();
    right_photon_origin_qnic_type = photon->getQNICtypeEntangledWith();
    right_statQubit_ptr = check_and_cast<StationaryQubit *>(photon->getEntangled_with());
    right_photon_Xerr = photon->getPauliXerr();
    right_photon_Zerr = photon->getPauliZerr();
    right_photon_lost = photon->getPhotonLost();
    if (photon->getFirst()) {
      right_last_photon_detected = false;
    }
    if (photon->getLast()) {
      right_last_photon_detected = true;
    }
    right_count++;
  } else {
    error("This shouldn't happen....! Only 2 connections to the BSA allowed");
  }
  if ((right_last_photon_detected || left_last_photon_detected)) {
    send_result = true;
  }
  // Just for debugging purpose
  forDEBUG_countErrorTypes(msg);
  double difference = (left_arrived_at - right_arrived_at).dbl();  // Difference in arrival time
  if (this_trial_done == true) {
    bubble("dumping result");
    // No need to do anything. Just ignore the BSA result for this shot 'cause the trial is over and photons will only arrive from a single node anyway.
    delete msg;
    return;
  } else if ((left_arrived_at != -1 && right_arrived_at != -1) && std::abs(difference) <= (required_precision)) {
    // Both arrived perfectly fine
    double rand = dblrand();  // Even if we have 2 photons, whether we success entangling the qubits or not is probablistic.
    double darkcount_left = dblrand();
    double darkcount_right = dblrand();
    if ((rand < ABSAsuccess_rate && !right_photon_lost && !left_photon_lost) /*No qubit lost*/ ||
        (!right_photon_lost && left_photon_lost && darkcount_left < darkcount_probability) /*Got right, darkcount left*/ ||
        (right_photon_lost && !left_photon_lost && darkcount_right < darkcount_probability) /*Got left, darkcount right*/ ||
        (right_photon_lost && left_photon_lost && darkcount_left < darkcount_probability && darkcount_right < darkcount_probability) /*Darkcount right left*/) {
      if (!right_photon_lost && (left_photon_lost && darkcount_left <= darkcount_probability)) {
        DEBUG_darkcount_left++;
        GOD_setCompletelyMixedDensityMatrix();
        sendABSAresult(false, send_result);
      } else if (!left_photon_lost && (right_photon_lost && darkcount_right <= darkcount_probability)) {
        DEBUG_darkcount_right++;
        GOD_setCompletelyMixedDensityMatrix();
        sendABSAresult(false, send_result);
      } else if ((left_photon_lost && darkcount_left <= darkcount_probability) && (right_photon_lost && darkcount_right <= darkcount_probability)) {
        DEBUG_darkcount_both++;
        GOD_setCompletelyMixedDensityMatrix();
        sendABSAresult(false, send_result);
      } else {
        bubble("Success...!");
        DEBUG_success++;
        GOD_updateEntangledInfoParameters_of_qubits();
        sendABSAresult(false, send_result);  // succeeded because both reached, and both clicked
      }

    }
    else {
      bubble("Failed...!");
      sendABSAresult(true, send_result);  // just failed because only 1 detector clicked while both reached
    }
    DEBUG_total++;

    initializeVariables();

  } else if ((left_arrived_at != -1 && right_arrived_at != -1) && std::abs(difference) > (required_precision)) {
    // Both qubits arrived, but the timing was bad.
    bubble("Emission Timing Failed");
    initializeVariables();
    sendABSAresult(true, send_result);
  } else {
    bubble("Waiting...");
    // Just waiting for the other qubit to arrive.
  }

  delete msg;
}

void ABSA::initializeVariables() {
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

void ABSA::sendABSAresult(bool result, bool sendresults) {
  // result could be false positive (actually ok but recognized as ng),
  // false negative (actually ng but recognized as ok) due to darkcount
  // true positive and true negative is no problem.
  if (!sendresults) {
    BSAresult *pk = new BSAresult;
    pk->setEntangled(result);
    send(pk, "toHoMController_port");
  } else {
    BSAfinish *pk = new BSAfinish();
    pk->setKind(7);
    pk->setEntangled(result);
    send(pk, "toHoMController_port");
    bubble("trial done now");
    this_trial_done = true;
  }
}

void ABSA::finish() {
  std::cout << "total = " << DEBUG_total << "\n";
  std::cout << "Success = " << DEBUG_success << "\n";
  std::cout << "darkcount_count_left = " << DEBUG_darkcount_left << ", darkcount_count_right =" << DEBUG_darkcount_right << ", darkcount_count_both = " << DEBUG_darkcount_both
            << "\n";
}

void ABSA::forDEBUG_countErrorTypes(cMessage *msg) {
  PhotonicQubit *q = check_and_cast<PhotonicQubit *>(msg);
  if (q->getPauliXerr() && q->getPauliZerr()) {
    count_Y++;
  } else if (q->getPauliXerr() && !q->getPauliZerr()) {
    count_X++;
  } else if (!q->getPauliXerr() && q->getPauliZerr()) {
    count_Z++;
  } else if (q->getPhotonLost()) {
    count_L++;
  } else {
    count_I++;
  }
  count_total++;
  EV << "Y%=" << (double)count_Y / (double)count_total << ", X%=" << (double)count_X / (double)count_total << ", Z%=" << (double)count_Z / (double)count_total
     << ", L%=" << (double)count_L / (double)count_total << ", I% =" << (double)count_I / (double)count_total << "\n";
}

bool ABSA::isPhotonLost(cMessage *msg) {
  PhotonicQubit *q = check_and_cast<PhotonicQubit *>(msg);
  if (q->getPhotonLost()) {
    return true;  // Lost
  } else {
    return false;
  }
  delete msg;
}

void ABSA::GOD_setCompletelyMixedDensityMatrix() {
  left_statQubit_ptr->setCompletelyMixedDensityMatrix();
  right_statQubit_ptr->setCompletelyMixedDensityMatrix();
}

void ABSA::GOD_updateEntangledInfoParameters_of_qubits() {

  left_statQubit_ptr->setEntangledPartnerInfo(right_statQubit_ptr);
  // If Photon had an X error, Add X error to the stationary qubit.
  if (left_photon_Xerr) left_statQubit_ptr->addXerror();
  if (left_photon_Zerr) left_statQubit_ptr->addZerror();

  right_statQubit_ptr->setEntangledPartnerInfo(left_statQubit_ptr);
  if (right_photon_Xerr) right_statQubit_ptr->addXerror();
  if (right_photon_Zerr) right_statQubit_ptr->addZerror();
  if (right_statQubit_ptr->entangled_partner == nullptr || left_statQubit_ptr->entangled_partner == nullptr) {
    std::cout << "Entangling failed\n";
    error("Entangling failed");
  }
  n_res++;
  emit(GOD_num_resSignal, n_res);
}
void ABSA::singleQubitMeasure(StationaryQubit * qubit){

}


}  // namespace modules
}  // namespace quisp