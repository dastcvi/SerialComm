/*
 * SerialComm.cpp
 * Author:  Alex St. Clair
 * Created: August 2019
 *  
 * This file implements an Arduino library (C++ class) that implements a simple, robust
 * serial (UART) protocol for inter-Arduino messaging.
 * 
 * This class doesn't define specific messages, so any project using the protocol must
 * implement message definitions on top of this class.
 */

#include "SerialComm.h"