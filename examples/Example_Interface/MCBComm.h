/*
 *  MCBComm.h
 *  Author:  Alex St. Clair
 *  Created: August 2019
 *  
 *  This file declares an Arduino library (C++ class) that implements the communication
 *  between the MCB and the DIB/PIB. The class inherits its protocol from the SerialComm
 *  class.
 */

#ifndef MCBCOMM_H
#define MCBCOMM_H

#include "SerialComm.h"

enum MCBMessages_t : uint8_t {
    MCB_NO_MESSAGE = 0,

    // MCB -> DIB/PIB (no params)
    MCB_MOTION_FINISHED,

    // MCB -> DIB/PIB (with params)
    MCB_MOTION_STATUS,
    MCB_ERROR,

    // DIB/PIB -> MCB (no params)
    MCB_CANCEL_MOTION, // ACK expected
    MCB_GO_LOW_POWER,  // ACK expected

    // DIB/PIB -> MCB (with params)
    MCB_REEL_OUT,
    MCB_REEL_IN,
    MCB_DOCK,
    MCB_OUT_ACC,
    MCB_IN_ACC,
    MCB_DOCK_ACC,
};


class MCBComm : public SerialComm {
public:
    MCBComm(Stream * serial_port);
    ~MCBComm() { };

    // MCB -> DIB/PIB (with params) -----------------------

    bool TX_Motion_Status(float reel_pos, float lw_pos, float reel_torque, float reel_temp, float lw_temp); // todo: voltages? timestamp?
    bool RX_Motion_Status(float * reel_pos, float * lw_pos, float * reel_torque, float * reel_temp, float * lw_temp);

    bool TX_Error(const char * error);
    bool RX_Error(char * error, uint8_t buffer_size);

    // DIB/PIB -> MCB (with params) -----------------------

    bool TX_Reel_Out(float num_revs, float speed);
    bool RX_Reel_Out(float * num_revs, float * speed);

    bool TX_Reel_In(float num_revs, float speed);
    bool RX_Reel_In(float * num_revs, float * speed);

    bool TX_Dock(float num_revs, float speed);
    bool RX_Dock(float * num_revs, float * speed);

    bool TX_Out_Acc(float acceleration);
    bool RX_Out_Acc(float * acceleration);

    bool TX_In_Acc(float acceleration);
    bool RX_In_Acc(float * acceleration);

    bool TX_Dock_Acc(float acceleration);
    bool RX_Dock_Acc(float * acceleration);

};

#endif /* MCBCOMM_H */