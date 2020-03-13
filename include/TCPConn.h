#ifndef TCPCONN_H
#define TCPCONN_H

#include "FileDesc.h"
#include "PasswdMgr.h"
#include <boost/multiprecision/cpp_int.hpp>


const int max_attempts = 2;

// Methods and attributes to manage a network connection, including tracking the username
// and a buffer for user input. Status tracks what "phase" of login the user is currently in
class TCPConn 
{
public:
   TCPConn(boost::multiprecision::uint128_t number/* LogMgr &server_log*/);
   ~TCPConn();

   bool accept(SocketFD &server);

   int sendText(const char *msg);
   int sendText(const char *msg, int size);

   bool handleConnection();
   void startAuthentication();
   
   bool getUserInput(std::string &cmd);

   void disconnect();
   bool isConnected();

   unsigned long getIPAddr() { return _connfd.getIPAddr(); };
   void getIPAddrStr(std::string &buf);

   bool isNewIPAllowed(std::string inputIP);



   bool getData(std::vector<uint8_t> &buf);
   bool sendData(std::vector<uint8_t> &buf);
   std::vector<uint8_t>::iterator findCmd(std::vector<uint8_t> &buf, std::vector<uint8_t> &cmd);
   bool hasCmd(std::vector<uint8_t> &buf, std::vector<uint8_t> &cmd);
   bool getCmdData(std::vector<uint8_t> &buf, std::vector<uint8_t> &startcmd, std::vector<uint8_t> &endcmd);
   void wrapCmd(std::vector<uint8_t> &buf, std::vector<uint8_t> &startcmd, std::vector<uint8_t> &endcmd);

   void sendNumber();
   bool waitForDivisor();
   void stopProcessing(int newNum);

   bool foundAllPrimeFactors = false;

   boost::multiprecision::uint128_t getPrimeFactor(){ return this->primeFactor;};
   boost::multiprecision::uint128_t getNumber(){ return this->number;};
   
   int node = 0;
   int dbNum = 0;
private:
   boost::multiprecision::uint128_t primeFactor;

   boost::multiprecision::uint128_t refNumber;   

   boost::multiprecision::uint128_t number;

   enum statustype { s_connected, s_sendNumber, s_waitForReply, s_primeFound, s_sendStop};

   std::vector<uint8_t> c_num, c_endnum, c_prime, c_endprime, c_stop;

   statustype _status = s_sendNumber;

   SocketFD _connfd;

   std::string _inputbuf;


};


#endif
