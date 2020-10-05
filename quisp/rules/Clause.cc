/** \file Clause.cc
 *
 *  \authors cldurand,,takaakimatsuo
 *  \date 2018/07/03
 *
 *  \brief Clause
 */
#include "Clause.h"
#include "tools.h"

namespace quisp {
namespace rules {

/*
bool FidelityClause::check(qnicResources* resources) const {
    stationaryQubit* qubit = NULL;
    checkQnic();//This is not doing anything...
    if(qubit = getQubit(resources, qnic_type, qnic_id, partner, resource)){
        return (qubit->getFidelity() >= threshold);
    }
    return false;
}*/

bool FidelityClause::check(std::multimap<int, StationaryQubit*> resource) const {
  StationaryQubit* qubit = nullptr;
  /*checkQnic();//This is not doing anything...
  if(qubit = getQubit(resources, qnic_type, qnic_id, partner, resource)){
      return (qubit->getFidelity() >= threshold);
  }
  return false;*/
}

bool EnoughResourceClause::check(std::multimap<int, StationaryQubit*> resource) const {
  // std::cout<<"!!In enough clause \n";
  bool enough = false;
  int num_free = 0;

  for (std::multimap<int, StationaryQubit*>::iterator it = resource.begin(); it != resource.end(); ++it) {
    if (it->first == partner) {
      if (!it->second->isLocked()) {  // here must have loop
        num_free++;
      }
      if (num_free >= num_resource_required) {
        enough = true;
      }
    }
  }
  // std::cout<<"Enough = "<<enough<<"\n";
  return enough;
}

bool EnoughResourceClauseLeft::check(std::multimap<int, StationaryQubit*> resource) const {
  // std::cout<<"!!In enough clause \n";
  bool enough = false;
  int num_free = 0;

  for (std::multimap<int, StationaryQubit*>::iterator it = resource.begin(); it != resource.end(); ++it) {
    if (it->first == partner_left) {
      if (!it->second->isLocked()) {  // here must have loop
        num_free++;
      }
      if (num_free >= num_resource_required_left) {
        enough = true;
      }
    }
  }
  if (enough) {
    EV << "You have enough resource between " << partner_left << "\n";
  } else {
    EV << "You don't have enough resource between " << partner_left << "\n";
  }
  // std::cout<<"Enough = "<<enough<<"\n";
  return enough;
}

bool EnoughResourceClauseRight::check(std::multimap<int, StationaryQubit*> resource) const {
  // std::cout<<"!!In enough clause \n";
  bool enough = false;
  int num_free = 0;

  for (std::multimap<int, StationaryQubit*>::iterator it = resource.begin(); it != resource.end(); ++it) {
    if (it->first == partner_right) {
      if (!it->second->isLocked()) {  // here must have loop
        num_free++;
      }
      if (num_free >= num_resource_required_right) {
        enough = true;
      }
    }
  }
  if (enough) {
    EV << "You have enough resource between " << partner_right << "\n";
  } else {
    EV << "You don't have enough resource between " << partner_right << "\n";
  }
  // std::cout<<"Enough = "<<enough<<"\n";
  return enough;
}

/*
bool MeasureCountClause::check(qnicResources* resources) const {
    //EV<<"MeasureCountClause invoked!!!! \n";
    if(current_count<max_count){
        current_count++;//Increment measured counter.
        EV<<"Measurement count is now "<<current_count<<" < "<<max_count<<"\n";
        return true;
    }
    else{
        EV<<"Count is enough";
        return false;
    }
}*/

bool MeasureCountClause::check(std::multimap<int, StationaryQubit*> resources) const {
  // std::cout<<"MeasureCountClause invoked!!!! \n";
  if (current_count < max_count) {
    current_count++;  // Increment measured counter.
    // std::cout<<"Measurement count is now "<<current_count<<" < "<<max_count<<"\n";
    return true;
  } else {
    // std::cout<<"Count is enough\n";
    return false;
  }
}

bool MeasureCountClause::checkTerminate(std::multimap<int, StationaryQubit*> resources) const {
  EV << "Tomography termination clause invoked.\n";
  bool done = false;
  if (current_count >= max_count) {
    // EV<<"TRUE: Current count = "<<current_count<<" >=  "<<max_count<<"(max)\n";
    done = true;
  }
  return done;
}

/*
bool MeasureCountClause::checkTerminate(qnicResources* resources) const {
    EV<<"Tomography termination clause invoked.\n";
    bool done = false;
    if(current_count >=max_count){
        EV<<"TRUE: Current count = "<<current_count<<" >=  "<<max_count<<"(max)\n";
        done = true;
    }
    return done;
}*/

/*
bool PurificationCountClause::check(qnicResources* resources) const {
    stationaryQubit* qubit = NULL;
    //checkQnic();//This is not doing anything...

    qubit = getQubitPurified(resources, qnic_type, qnic_id, partner, num_purify_must);
    if(qubit != nullptr){
        return true;//There is a qubit that has been purified "num_purify_must" times.
    }else{
        return false;
    }
}*/

bool PurificationCountClause::check(std::multimap<int, StationaryQubit*> resource) const {
  StationaryQubit* qubit = nullptr;
}

//ABSA clauses start here
//Algorithm 1 Clause
//do we need a list if we are only using the 1st index?
bool initConditionalClause::check(std::arrivalTime<int){
  initTime = false;
  //get current time
  currentTime = simTime();
  if (currentTime < arrivalTime){
    initTime = true;
  }
  return initTime;
}
  
//Algorithm 3 Clause
//Is arrival time a list? why two values?
bool MeasureConditionalClause::check(std::arrivalTime<int){
  //how to get current time?
  currentTime = simTime();
  measurementNeeded = false;
  if (arrivalTime <= currentTime){
    measurementNeeded = true;
  }
  return measurementNeeded;
}
  
//Algorithm 4 Clause
//what is the size of the list?
//What are the content of the list? ints? bools?
//Can we represent the basis are ints, e.g. 1 >> encodeX?
int postBellConditionalClause::check(std::outList<int*, std::successBell<bool){
  //basis = "encodeX";
  basis = 1;
  if (successBell == true or outList[-1] == false){
    //basis = "encodeZ";
    basis = 2;
  }
  return basis;
}
  
  
 //Algorithm 6 Clause
bool finalConditionalClause::check(std::arrivalTime<int, std::msgSent<bool){
  //get current time
  currentTime = simTime();
  msgNeeded = false;
  if (currentTime> arrivalTime and msgSent == false){
    msgNeeded = true;
  }
  return msgNeeded;
}
  
 
//Algorithm 8 Clause
bool qkdInitConditionalClause(std::arrivalTime<int*){
  initNeeded = false;
  //get current time
  currentTime = ;
  if (currentTime < arrivalTime[0]){
    initNeeded = true;
  }
  return initNeeded;
}
  
  

//Algorithm 10 Clause
//needs to be divided into two clause >> multiple returns
//If we have only two meaurement basis, can we just use true and false for them
bool *qkdMeasureConditionClause(std::arrivalTimeList<int*, *basisList){
  measurementNeeded = false;
  //get current time
  currentTime = ;
  //get_index??
  index = get_index(currentTime, int *arrivalTimeList);
  if (arrivalTimeList[0] <= currentTime and currentTime <= arrivalTimeList[-1]){
    measurementNeeded = true;
    basis = basisList[index];
  }
  return measurementNeeded, basis;
}
  
  
//Algorithm 12 Clause
bool qkdFinalConditionClause(std::Algorithm<int*){
  currentTime = ;
  finalNeeded = false;
  if(currentTime > arrivalTimeList[-1]){
    finalNeeded = true;
  }
  return finalNeeded;
}

}  // namespace rules
}  // namespace quisp
