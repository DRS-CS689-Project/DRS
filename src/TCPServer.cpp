#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdexcept>
#include <strings.h>
#include <vector>
#include <iostream>
#include <memory>
#include <sstream>
#include "TCPServer.h"
#include <boost/multiprecision/cpp_int.hpp>
#include <chrono>

TCPServer::TCPServer(boost::multiprecision::uint128_t number, int numNodes){ // :_server_log("server.log", 0) {
   this->number = number;
   this->numOfNodes = numNodes;
}


TCPServer::~TCPServer() {

}

/**********************************************************************************************
 * bindSvr - Creates a network socket and sets it nonblocking so we can loop through looking for
 *           data. Then binds it to the ip address and port
 *
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/

void TCPServer::bindSvr(const char *ip_addr, short unsigned int port) {

   struct sockaddr_in servaddr;

   // _server_log.writeLog("Server started.");

   // Set the socket to nonblocking
   _sockfd.setNonBlocking();

   _sockfd.setReusable();

   // Load the socket information to prep for binding
   _sockfd.bindFD(ip_addr, port);
 
}

/**********************************************************************************************
 * listenSvr - Performs a loop to look for connections and create TCPConn objects to handle
 *             them. Also loops through the list of connections and handles data received and
 *             sending of data. 
 *
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/

void TCPServer::listenSvr() {


   _sockfd.listenFD(5);

   std::string ipaddr_str;
   std::stringstream msg;
   _sockfd.getIPAddrStr(ipaddr_str);
   msg << "Server listening on IP " << ipaddr_str << "' port '" << _sockfd.getPort() << "'";
   //_server_log.writeLog(msg.str().c_str());
}


/**********************************************************************************************
 * runSvr - Performs a loop to look for connections and create TCPConn objects to handle
 *          them. Also loops through the list of connections and handles data received and
 *          sending of data. 
 *
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/

void TCPServer::runServer() {
   bool online = true;
   timespec sleeptime;
   sleeptime.tv_sec = 0;
   sleeptime.tv_nsec = 100000000;

   // Start the server socket listening
   listenSvr();

   while (online) {
      // wait until all nodes have connected
      if(_connlist.size() != numOfNodes)
         handleSocket();

      // if all nodes have connected, start processing
      if(_connlist.size() == numOfNodes)
         // if handleConnections returns a true it means all primes have been
         // found so shut down server
         if(handleConnections()) {
            
            online = false;
         }
            

      // So we're not chewing up CPU cycles unnecessarily
      //nanosleep(&sleeptime, NULL);
   }
}


/**********************************************************************************************
 * handleSocket - Checks the socket for incoming connections and validates against the whitelist.
 *                Accepts valid connections and adds them to the connection list.
 *
 *    Returns: pointer to a new connection if one was found, otherwise NULL
 *
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/

TCPConn *TCPServer::handleSocket() {
  
   // The socket has data, means a new connection 
   if (_sockfd.hasData()) {

      struct sockaddr_in cliaddr;
      socklen_t len = sizeof(cliaddr);

      if (_sockfd.hasData()) {
         TCPConn *new_conn = new TCPConn(this->number);
         if (!new_conn->accept(_sockfd)) {
            // _server_log.strerrLog("Data received on socket but failed to accept.");
            return NULL;
         }
         
         // Get their IP Address string to use in logging
         std::string ipaddr_str;
         new_conn->getIPAddrStr(ipaddr_str);

         std::cout << "Client IP: " << ipaddr_str << std::endl;//testing

         //check is the client ip address on the whitelist
         // if ( !new_conn->isNewIPAllowed(ipaddr_str) ){
         //    std::cout << "This IP address is not authorized" << std::endl;
         //    new_conn->sendText("Not Authorized To Log into System\n");
         //    new_conn->disconnect();
         //    return NULL;  
         // }

         std::cout << "***Got a connection***\n";

         _connlist.push_back(std::unique_ptr<TCPConn>(new_conn));

         if(_connlist.size() != numOfNodes) {
            //new_conn->sendText("Waiting for all nodes to connect...\n");
         }

         // Change this later
         //new_conn->startAuthentication();
         new_conn->node = nodes;
         nodes++;

         return new_conn;
      }
      
   }
   return NULL;
}

/**********************************************************************************************
 * handleConnections - Loops through the list of clients, running their functions to handle the
 *                     clients input/output.
 *
 *    Returns: number of new connections accepted
 *
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/

bool TCPServer::handleConnections() {

   // Loop through our connections, handling them
      std::list<std::unique_ptr<TCPConn>>::iterator tptr = _connlist.begin();
      if(!(start < std::chrono::high_resolution_clock::now())) {
         std::cout << "changeing time \n";
         auto start = std::chrono::high_resolution_clock::now();
      }

      while (tptr != _connlist.end())
      {

         if((*tptr)->foundAllPrimeFactors) {
            auto stop = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
            std::cout << "TIME: " << duration.count() << " microseconds\n";
            return true;
         }

         // If the user lost connection
         if (!(*tptr)->isConnected()) {
            // Log it

            // Remove them from the connect list
            tptr = _connlist.erase(tptr);
            std::cout << "Connection disconnected.\n";
            continue;
         }

         // Process any user inputs
         if((*tptr)->handleConnection()) {
            auto primeFound = (*tptr)->getPrimeFactor();
            std::cout << "In TCPServer - primeFound: " << primeFound << std::endl;
            //a prime was found so send stop message to all other connections
            std::list<std::unique_ptr<TCPConn>>::iterator tptr2 = _connlist.begin();
            for(; tptr2 != _connlist.end(); tptr2++){
               if (tptr2 != tptr){
                  (*tptr2)->stopProcessing(int(primeFound));
               }
            }
               
         }


         // Increment our iterator
         tptr++;
      }
      return false;

}

/**********************************************************************************************
 * shutdown - Cleanly closes the socket FD.
 *
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/

void TCPServer::shutdown() {
   //_server_log.writeLog("Server shutting down.");
   _sockfd.closeFD();
}