
#include <iostream>
using namespace std;

#include <stdlib.h>
#include <arpa/inet.h>

#include "Tracker.h"


void showHelp ()
{
   cout << "Usage: tracker port options" << endl;
   cout << "  port     The port number on which to receive UDP packets" << endl;
   cout << "  -debug   Turn on extra verbose debugging" << endl;
}


int main (int argc, char** argv)
{
   bool bDoDebug = false;

   Tracker  theTracker;


   if (argc != 2) {
      if (argc < 2)
      {
         showHelp();
         exit(-1);
      }
      else
      {
         for(int j=2; j<argc; j++)
         {
            if(strcmp("-debug", argv[j]) == 0)
            {
               theTracker.setVerbose(true);
            }
         }
      }
   }

   /* Register / properly shutdown on a Control-C */
   // TODO: Add that code


   /* Take the port from the first argument */
   /*   Is this properly protected? */
   theTracker.setPort(atoi(argv[1]));

   /* Try to initialize the tracker */
   if (!theTracker.initialize(NULL)) {
      cerr << "Error: Failed to initializer tracker - exiting" << endl;
      exit(-1);
   }

   /* Enter the loop for the tracker */
   theTracker.go();
}
