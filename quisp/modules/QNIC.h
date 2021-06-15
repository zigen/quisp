/** \file QNIC.h
 *  \authors cldurand
 *  \date 2018/06/29
 *
 *  \brief QNIC
 */
#ifndef QUISP_MODULES_QNIC_H_
#define QUISP_MODULES_QNIC_H_

#include <omnetpp.h>
#include <unordered_map>
using namespace omnetpp;

namespace quisp {
namespace modules {

enum class QNIC_type : int {
  QNIC_E = 0, /**< Emitter QNIC          */
  QNIC_R, /**< Receiver QNIC         */
  QNIC_RP, /**< Passive Receiver QNIC */
  Count, /** Just to make the size of the array = the number of qnics*/
};

static const std::unordered_map<QNIC_type, const char*> QNIC_names = {
    {QNIC_type::QNIC_E, "qnic"},
    {QNIC_type::QNIC_R, "qnic_r"},
    {QNIC_type::QNIC_RP, "qnic_rp"},
};

struct QNIC_id {
  QNIC_type type;
  int index;
  int address;
  bool isReserved;
};

struct QNIC_id_pair {
  QNIC_id fst;
  QNIC_id snd;
};

struct QNIC : QNIC_id {
  cModule* pointer;  // Pointer to that particular QNIC.
  int address;
};

// Table to check the qnic is reserved or not.
typedef std::map<int, std::map<int, bool>> QNIC_reservation_table;

}  // namespace modules
}  // namespace quisp

#endif  // QUISP_MODULES_QNIC_H_
