#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <string>
#include "Client.h"
#include "FileDesc.h"
#include "DivFinderServer.h"

// The amount to read in before we send a packet
const unsigned int stdin_bufsize = 50;
const unsigned int socket_bufsize = 100;

class TCPClient : public Client
{
public:
   TCPClient();
   ~TCPClient();

   virtual void connectTo(const char *ip_addr, unsigned short port);
   virtual void handleConnection();

   virtual void closeConn();

   std::string inputNum;

   bool initMessage = true;
   bool activeThread = false;

   DivFinderServer d;

   std::unique_ptr<std::thread> th;

   //needs to be fixed
   bool getData(std::vector<uint8_t> &buf);
   bool getCmdData(std::vector<uint8_t> &buf, std::vector<uint8_t> &startcmd, 
                                                    std::vector<uint8_t> &endcmd);
   std::vector<uint8_t>::iterator findCmd(std::vector<uint8_t> &buf, std::vector<uint8_t> &cmd);
   void wrapCmd(std::vector<uint8_t> &buf, std::vector<uint8_t> &startcmd,
                                                    std::vector<uint8_t> &endcmd);

private:
   int readStdin();

   // Stores the user's typing
   std::string _in_buf;

   // Class to manage our client's network connection
   SocketFD _sockfd;
 
   // Manages the stdin FD for user inputs
   TermFD _stdin;




   std::vector<uint8_t> c_num, c_endnum, c_prime, c_endprime, c_stop;

};


#endif
