// Message.h : Message container for datagrams to / from the tracker

#ifndef __MESSAGE_H
#define __MESSAGE_H

#include <stdint.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <time.h>
#include <cstring>

#include <string>
using namespace std;


#define MSG_MAX_SIZE    1500

#define MSG_TYPE_UNKNOWN         0
#define MSG_TYPE_REGISTER        1
#define MSG_TYPE_REGISTER_ACK    2
#define MSG_TYPE_LIST_NODES      3
#define MSG_TYPE_LIST_NODES_DATA 4

// Testing only messages
#define MSG_TYPE_ECHO            5
#define MSG_TYPE_ECHO_RESPONSE   6

#define MSG_STATUS_FINE       0
#define MSG_STATUS_ISSUE      1

// Message came from the client
#define MSG_DIRECTION_CLIENT  0
// Message is being sent from the server
#define MSG_DIRECTION_SERVER  1

class Message
{
   private:
      uint8_t     m_byType;
      uint16_t    m_nDataLength;
      uint8_t     m_byErrorCode;
      uint8_t     m_byData [MSG_MAX_SIZE];

      // Address and port associated with the socket
      struct sockaddr_in   m_SrcInfo;

      // Time - when did the message arrive
      struct timeval m_timeArrival;

      // Which direction is the message going?
      uint8_t     m_byDirection;

   public:
      /** Default constructor */
      Message ();

      /** Destructor included - not really needed */
      ~Message();

      uint8_t  getType ()
      { return m_byType; }

      void setType (uint8_t byType)
      { m_byType = byType; }

      uint16_t  getLength ()
      { return m_nDataLength; }

      void setLength (uint16_t nLength)
      { m_nDataLength = nLength; }

      uint8_t * getData ()
      { return m_byData; }

      uint16_t getMaxLength ()
      { return MSG_MAX_SIZE; }

      /* Why might we not want to have an equivalent set? */

      /** Retrieve the type as a string rather than its value
       *  @returns Valid C++ string denoting the type
      */
      string    getTypeAsString ();

      /** Record the arrival time */
      void     recordArrival ();

      /** Get access to the arrival time */
      timeval *   getArrivalTime ()
      { return &m_timeArrival; }

      /** Get access to the address that sent the message */
      struct sockaddr_in * getAddress ()
      { return &m_SrcInfo; }

      /** Extract all the data (including type and length) into the
       * specified buffer location
       */
      int   extractBuffer (uint8_t * pBuffer, int nMaxSize);

      void  dumpData ();
};

#endif
