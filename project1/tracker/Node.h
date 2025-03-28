// Node.h : Definition file for tracking a node that has registered with
//          the tracker

#ifndef __NODE_H
#define __NODE_H

#include <stdint.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <time.h>

#include <string>
using namespace std;

/** Node is the object to hold information related to a node that is being tracked
 * by the P2P system.  Each Node has a unique identifier and the tracker keeps track of
 * the IP address and port number for each node.
 */
class Node
{
   private:
      /* IP address where the node is located */
      uint32_t    m_IP_Address;

      /* Port number where the node is listening */
      uint16_t    m_nPort;

      /* Number of files present */
      uint16_t    m_nFiles;

      /* When was the node registered */
      struct timeval    m_LastRegistration;

      /* Registration expiry - when will it expire from the tracker */
      struct timeval    m_RegistrationExpiry;

      /* The ID as assigned by the tracker */
      uint8_t     m_nID;

   public:
      Node ();
      ~Node ();

      void setIPAddress (uint32_t theAddress)
      { m_IP_Address = theAddress; }

      uint32_t getIPAddress ()
      { return m_IP_Address; }

      /* To quote Bilbo Baggins, after all, why not? */
      uint32_t * getIPAddressAsPointer ()
      { return &m_IP_Address; }

      void setPort (uint16_t nPort)
      { m_nPort = nPort; }

      uint16_t getPort ()
      { return m_nPort; }

      void setFiles (uint16_t nFiles)
      { m_nFiles = nFiles; }

      uint16_t getFiles ()
      { return m_nFiles; }

      void setID (uint8_t nID)
      { m_nID = nID;}

      uint8_t getID ()
      { return m_nID; }

      void setExpirationTime (struct timeval theExp)
      { m_RegistrationExpiry = theExp; }

      struct timeval getExpirationTime ()
      { return m_RegistrationExpiry; }

      struct timeval * getExpirationTimeAsPointer ()
      { return &m_RegistrationExpiry; }

      /** Update the node registration information, e.g. when did the
       *  node last attempt to register
       */
      void updateRegistrationTime ();

      struct timeval * getRegistrationTime ()
      { return &m_LastRegistration; }

      /** Construct / fill the byte array with the registration
       *  ACK information
       *  @param pData The byte buffer location to place the data
       *  @returns The number of bytes filled in by this function
       */
      uint16_t constructRegistrationAck (uint8_t * pData);

      /** Construct / fill the byte array with the node data
       *  information
       *  @param pData The byte buffer location to place the data
       *  @returns The number of bytes filled in by this function
       */
      uint16_t constructNodeData (uint8_t * pData);
};


#endif
