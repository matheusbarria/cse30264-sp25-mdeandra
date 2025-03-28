
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/types.h>

#include "Message.h"

Message::Message ()
{
   m_byType = MSG_TYPE_UNKNOWN;
   m_nDataLength = 0;
   memset(m_byData, 0, MSG_MAX_SIZE);
}

Message::~Message ()
{

}

string Message::getTypeAsString ()
{
   switch(getType())
   {
      case MSG_TYPE_UNKNOWN:
         return "Unknown";
      case MSG_TYPE_REGISTER:
         return "Register";
      case MSG_TYPE_REGISTER_ACK:
         return "Register-Ack";
      case MSG_TYPE_LIST_NODES:
         return "List-Nodes";
      case MSG_TYPE_LIST_NODES_DATA:
         return "List-Nodes-Data";
      case MSG_TYPE_ECHO:
         return "Echo";
      case MSG_TYPE_ECHO_RESPONSE:
         return "Echo-Response";
      default:
         return "Undefined";
   }
}

void Message::recordArrival ()
{
   gettimeofday(getArrivalTime(), 0);
}

int  Message::extractBuffer (uint8_t * pBuffer, int nMaxSize)
{
   /* First byte is always the buffer */
   pBuffer[0] = getType();

   /* Convert things to big endian (if needed) */
   uint16_t  properLength;

   properLength = htons(getLength());
   memcpy(pBuffer+1,&properLength,2);

   /* Put all of the actual data in */
   for(int j=0; j<getLength(); j++)
   {
      pBuffer[j+3] = m_byData[j];
   }

   /* Length is inclusive of the type and length field */
   return getLength();
}

void Message::dumpData ()
{
   printf("Message with length (%d bytes)\n", getLength());
   printf("  Message arrived at %ld.%d\n", m_timeArrival.tv_sec, m_timeArrival.tv_usec);
   printf("  Type = 0x%02X\n", m_byData[0]);

   uint16_t    nHostOrderLength;

   memcpy(&nHostOrderLength, m_byData+1, 2);
   nHostOrderLength = ntohs(nHostOrderLength);

   printf("  Length Field = 0x%02X%02X (Value = %d)\n", m_byData[1], m_byData[2], nHostOrderLength);

   for (int j=0; j<getLength(); j++)
   {
      printf("Byte %02d: %02X", j, m_byData[j]);

      switch(j)
      {
         case 0:
            printf("  Type\n");
            break;
         case 1:
            printf("  Length - MSB\n");
            break;
         case 2:
            printf("  Length - LSB\n");
            break;

         default:
            printf("\n");
            break;
      }

   }
}
