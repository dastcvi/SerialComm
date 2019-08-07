/*
 *  MCBComm.cpp
 *  Author:  Alex St. Clair
 *  Created: August 2019
 *  
 *  This file implements an Arduino library (C++ class) that implements the communication
 *  between the MCB and the DIB/PIB. The class inherits its protocol from the SerialComm
 *  class.
 */

#include "MCBComm.h"

MCBComm::MCBComm(Stream * serial_port)
    : SerialComm(serial_port)
{
}

// MCB -> DIB/PIB (with params) ---------------------------

// -- MCB Motion status

bool MCBComm::TX_Motion_Status(float reel_pos, float lw_pos, float reel_torque, float reel_temp, float lw_temp)
{
    if (!Add_float(reel_pos)) return false;
    if (!Add_float(lw_pos)) return false;
    if (!Add_float(reel_torque)) return false;
    if (!Add_float(reel_temp)) return false;
    if (!Add_float(lw_temp)) return false;

    TX_ASCII(MCB_MOTION_STATUS);

    return true;
}

bool MCBComm::RX_Motion_Status(float * reel_pos, float * lw_pos, float * reel_torque, float * reel_temp, float * lw_temp)
{
    float temp1, temp2, temp3, temp4, temp5;

    if (!Get_float(&temp1)) return false;
    if (!Get_float(&temp2)) return false;
    if (!Get_float(&temp3)) return false;
    if (!Get_float(&temp4)) return false;
    if (!Get_float(&temp5)) return false;

    // only update variables if the parsing succeeds
    *reel_pos = temp1;
    *lw_pos = temp2;
    *reel_torque = temp3;
    *reel_temp = temp4;
    *lw_temp = temp5;

    return true;
}

// -- MCB error string

bool MCBComm::TX_Error(const char * error)
{
    if (Add_string(error)) return false;

    TX_ASCII(MCB_ERROR);

    return true;
}

bool MCBComm::RX_Error(char * error, uint8_t buffer_size)
{
    return Get_string(error, buffer_size);
}


// DIB/PIB -> MCB (with params) ---------------------------

// -- Reel out command

bool MCBComm::TX_Reel_Out(float num_revs, float speed)
{
    if (!Add_float(num_revs)) return false;
    if (!Add_float(speed)) return false;

    TX_ASCII(MCB_REEL_OUT);

    return true;
}

bool MCBComm::RX_Reel_Out(float * num_revs, float * speed)
{
    float temp1, temp2;

    if (!Get_float(&temp1)) return false;
    if (!Get_float(&temp2)) return false;

    *num_revs = temp1;
    *speed = temp2;

    return true;
}

// -- Reel in command

bool MCBComm::TX_Reel_In(float num_revs, float speed)
{
    if (!Add_float(num_revs)) return false;
    if (!Add_float(speed)) return false;

    TX_ASCII(MCB_REEL_IN);

    return true;
}

bool MCBComm::RX_Reel_In(float * num_revs, float * speed)
{
    float temp1, temp2;

    if (!Get_float(&temp1)) return false;
    if (!Get_float(&temp2)) return false;

    *num_revs = temp1;
    *speed = temp2;

    return true;
}

// -- Dock command

bool MCBComm::TX_Dock(float num_revs, float speed)
{
    if (!Add_float(num_revs)) return false;
    if (!Add_float(speed)) return false;

    TX_ASCII(MCB_DOCK);

    return true;
}

bool MCBComm::RX_Dock(float * num_revs, float * speed)
{
    float temp1, temp2;

    if (!Get_float(&temp1)) return false;
    if (!Get_float(&temp2)) return false;

    *num_revs = temp1;
    *speed = temp2;

    return true;
}

// -- Reel out acceleration

bool MCBComm::TX_Out_Acc(float acceleration)
{
    if (!Add_float(acceleration)) return false;

    TX_ASCII(MCB_OUT_ACC);

    return true;
}

bool MCBComm::RX_Out_Acc(float * acceleration)
{
    if (!Get_float(acceleration)) return false;

    return true;
}

// -- Reel in acceleration

bool MCBComm::TX_In_Acc(float acceleration)
{
    if (!Add_float(acceleration)) return false;

    TX_ASCII(MCB_IN_ACC);

    return true;
}

bool MCBComm::RX_In_Acc(float * acceleration)
{
    if (!Get_float(acceleration)) return false;

    return true;
}

// -- Dock acceleration

bool MCBComm::TX_Dock_Acc(float acceleration)
{
    if (!Add_float(acceleration)) return false;

    TX_ASCII(MCB_DOCK_ACC);

    return true;
}

bool MCBComm::RX_Dock_Acc(float * acceleration)
{
    if (!Get_float(acceleration)) return false;

    return true;
}

