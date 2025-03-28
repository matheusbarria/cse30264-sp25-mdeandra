// Node.cc : Source file for a node being tracked by the system

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <time.h>
#include <sys/time.h>
#include <cstring>

#include <string>
using namespace std;

#include "Node.h"

Node::Node ()
{
   m_IP_Address = 0;
   m_nPort = 0;
   m_nFiles = 0;

   m_LastRegistration.tv_sec = 0;
   m_LastRegistration.tv_usec = 0;

   m_RegistrationExpiry.tv_sec = 0;
   m_RegistrationExpiry.tv_usec = 0;

   m_nID = 0;
}

Node::~Node ()
{
   // Nothing to clean up - for now
}

void Node::updateRegistrationTime ()
{
   gettimeofday(&m_LastRegistration, 0);
}

uint16_t Node::constructRegistrationAck (uint8_t * pData)
{
   pData[0] = m_nID;

   /* We are assuming that the network address is stored in proper order
   since it is a byte-wise set of values anyway */
   memcpy(pData+1, &m_IP_Address, 4);

   /* Convert this to network order */
   uint16_t    nDataNetOrder;

   nDataNetOrder = htons(m_nPort);
   memcpy(pData+5, &nDataNetOrder, 2);

   /* Number of files */
   nDataNetOrder = htons(m_nFiles);
   memcpy(pData+7, &nDataNetOrder, 2);

   /* Put up the expiration time with proper network ordering */
   uint32_t    nTimeNetOrder;

   nTimeNetOrder = htonl(m_RegistrationExpiry.tv_sec);
   memcpy(pData+9, &nTimeNetOrder, 4);

   /* Magic numbers - whoot, whoot
      Though in all seriousness
        1 Byte - ID
        4 Bytes - IP Address
        2 Bytes - Port
        2 Bytes - Num Files
        4 Bytes - Expiration Time for Registration
   */
   return 13;
}

uint16_t Node::constructNodeData (uint8_t * pData)
{
   /* Code re-use FTW */
   return constructRegistrationAck(pData);
}
