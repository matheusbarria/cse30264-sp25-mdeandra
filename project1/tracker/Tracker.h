
#ifndef __TRACKER_H
#define __TRACKER_H

#include <vector>
using namespace std;

#include <stdint.h>

#include "Node.h"
#include "Message.h"

#define DEFAULT_REGISTER_EXPIRATION    300

class Tracker
{
   private:
      vector<Node> m_NodeTable;

      // The port that the tracker will be bound
      uint16_t m_nPort;

      bool     m_bVerbose;

      // The socket associated with the tracker (server)
      int      m_nSocket;

      /* What is our particular information for the server? */
      struct sockaddr_in m_AddressInfo;


      uint8_t  m_nNextID;

      /* Time that a node receives their lease (default = 300) */
      uint32_t    m_nLeaseTime;

   public:

      Tracker ();
      ~Tracker();

      bool initialize (char * pszIP);

      struct sockaddr_in * getAddressInfo ()
      { return &m_AddressInfo; }

      int   getSocket ()
      { return m_nSocket; }

      uint16_t getPort ()
      { return m_nPort; }

      void setPort (uint16_t nPort)
      { m_nPort = nPort; }

      bool isVerbose ()
      { return m_bVerbose; }

      void setVerbose (bool bVerbose)
      { m_bVerbose = bVerbose; }

      uint32_t getLeaseTime ()
      { return m_nLeaseTime; }

      void setLeaseTime (uint32_t nLeaseTime)
      { m_nLeaseTime = nLeaseTime; }

      /* Sit and loop */
      void  go ();

      /** Wait for a message on the server socket
       * @returns Pointer to a valid object if there was a message successfully read
       */
      Message * recvMessage ();

      bool  processEcho (Message * pEchoMessage);
      bool  processRegister (Message * pRegisterMessage);
      bool  processListNodes (Message * pListNodesMessage);

      bool  doEcho ();

      uint8_t  getNextNodeID ();

      int   findNodeIndexByID (uint8_t theID);

      void  dumpTable ();
};


#endif