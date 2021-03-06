#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <list>
#include <memory>
#include "Server.h"
#include "FileDesc.h"
#include "TCPConn.h"
#include <boost/multiprecision/cpp_int.hpp>
#include <chrono>
#include <boost/optional.hpp>
#include <vector>

class TCPServer : public Server 
{
public:
   TCPServer(boost::multiprecision::uint128_t number, int numNodes);
   ~TCPServer();

   virtual void bindSvr(const char *ip_addr, unsigned short port);
   void listenSvr();
   virtual void runServer();

   void shutdown();

   TCPConn *handleSocket();
   virtual bool handleConnections();

   int nodes = 1;
   unsigned int serverTask = 0;

   std::chrono::system_clock::time_point start;

   std::vector<boost::multiprecision::uint128_t> primeFactorsVector; 

private:
   // Class to manage the server socket
   SocketFD _sockfd;
 
   // List of TCPConn objects to manage connections
   std::list<std::unique_ptr<TCPConn>> _connlist;

   

   boost::multiprecision::uint128_t number;

   int numOfNodes = 3;

   

};


#endif
