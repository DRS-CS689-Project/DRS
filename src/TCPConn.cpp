#include <stdexcept>
#include <strings.h>
#include <unistd.h>
#include <cstring>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <memory>
#include "TCPConn.h"
#include "strfuncts.h"
#include <boost/lexical_cast.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include "DivFinderServer.h"
#include "config.h"

#include "PasswdMgr.h"

// The filename/path of the password file
const char pwdfilename[] = "passwd";

TCPConn::TCPConn(boost::multiprecision::uint128_t number) { // LogMgr &server_log):_server_log(server_log) {
   uint8_t slash = (uint8_t) '/';

   c_num.push_back((uint8_t) '<');
   c_num.push_back((uint8_t) 'N');
   c_num.push_back((uint8_t) 'U');
   c_num.push_back((uint8_t) 'M');
   c_num.push_back((uint8_t) '>');

   c_endnum = c_num;
   c_endnum.insert(c_endnum.begin()+1, 1, slash);

   c_prime.push_back((uint8_t) '<');
   c_prime.push_back((uint8_t) 'P');
   c_prime.push_back((uint8_t) 'R');
   c_prime.push_back((uint8_t) 'I');
   c_prime.push_back((uint8_t) 'M');
   c_prime.push_back((uint8_t) 'E');
   c_prime.push_back((uint8_t) '>');

   c_endprime = c_prime;
   c_endprime.insert(c_endprime.begin()+1, 1, slash);

   c_stop.push_back((uint8_t) '<');
   c_stop.push_back((uint8_t) 'S');
   c_stop.push_back((uint8_t) 'T');
   c_stop.push_back((uint8_t) 'O');
   c_stop.push_back((uint8_t) 'P');
   c_stop.push_back((uint8_t) '>');

   this->number = number;
}


TCPConn::~TCPConn() {

}

/**********************************************************************************************
 * accept - simply calls the acceptFD FileDesc method to accept a connection on a server socket.
 *
 *    Params: server - an open/bound server file descriptor with an available connection
 *
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/

bool TCPConn::accept(SocketFD &server) {
   return _connfd.acceptFD(server);
}


/**********************************************************************************************
 * startAuthentication - Sets the status to request username
 *
 *    Throws: runtime_error for unrecoverable types
 **********************************************************************************************/

void TCPConn::startAuthentication() {

   // this needs to be changed...doesnt make sense
   _status = s_sendNumber;
}

/**********************************************************************************************
 * handleConnection - performs a check of the connection, looking for data on the socket and
 *                    handling it based on the _status, or stage, of the connection
 *
 *    Throws: runtime_error for unrecoverable issues
 **********************************************************************************************/

bool TCPConn::handleConnection() {

   timespec sleeptime;
   sleeptime.tv_sec = 0;
   sleeptime.tv_nsec = 100000000;

   bool foundPrime = false;

   try {
      std::vector<uint8_t> buf;
      switch (_status) {
         // send current number to be factored to client
         case s_sendNumber:
            std::cout << "Node " << node << " In s_sendNuber" << std::endl;
            
            if (getData(buf))
            {
               std::string bufStr(buf.begin(), buf.end());
               //std::cout << "buf: " << bufStr  << std::endl;
               buf.clear();
            }
            sendNumber();
            break;

         // wait for the divisor to come back from client
         // if waitForDivisor() returns true it means that this TCPConn object
         // returned a prime and tells TCPServer to reset all connections
         case s_waitForReply:
            //std::cout << "Node " << node << " In s_waitForReply" << std::endl;
            foundPrime = waitForDivisor();
            break;

         // send the stop command to client and reset status to send the current
         // number to be factored
         case s_sendStop:
            std::cout << "Node " << node << " In s_sendStop" << std::endl;
            sendData(c_stop);
            //std::vector<uint8_t> buf;
            if (getData(buf))
            {
               std::string bufStr(buf.begin(), buf.end());
               //std::cout << "buf: " << bufStr  << std::endl;
               buf.clear();
            }

            _status = s_sendNumber;
            break;

         default:
            throw std::runtime_error("Invalid connection status!");
            break;
      }
   } catch (socket_error &e) {
      std::cout << "Socket error, disconnecting.";
      disconnect();
      return false;
   }

   //nanosleep(&sleeptime, NULL);
   return foundPrime;
}



// sends the current number to be factored to client
void TCPConn::sendNumber() {
   // Convert it to a string
   std::string bignumstr  = boost::lexical_cast<std::string>(this->number);

   // put it into a byte vector for transmission
   std::vector<uint8_t> buf(bignumstr.begin(), bignumstr.end());

   // wrap with num tags and send to client
   wrapCmd(buf, c_num, c_endnum);
   sendData(buf);

   // wait for reply
   _status = s_waitForReply;  




   //unsigned int num = 975851579543363;

   //std::string numStr;

   //numStr = "NUM 975851579543363";

   //_connfd.writeFD(numStr);
}

bool TCPConn::waitForDivisor(){
   if (_connfd.hasData()) {
      std::vector<uint8_t> buf;

      if (!getData(buf))
         return false;

      if (!getCmdData(buf, c_prime, c_endprime)) {
         //std::stringstream msg;
         //msg << "Replication data possibly corrupted from" << getNodeID() << "\n";
         //_server_log.writeLog(msg.str().c_str());
         //disconnect();
         return false;
      }

      std::string primeStr(buf.begin(), buf.end());
      boost::multiprecision::uint128_t prime(primeStr);

      std::cout << "##########################################" << "\n";
      std::cout << "Node " << node << " got a prime " << primeStr << "\n";
      std::cout << "##########################################" << "\n";
      
      if (prime == 563){
         int val = 1;
      } 

      this->primeFactor = prime;
      

      this->number = this->number / prime;
      
      

      std::cout << "Node " << node << " Number: " << this->number << "\n";

      DivFinderServer df;
      LARGEINT l;

      if(this->number == 1) {
         //df.isPrimeBF(this->number, l);
         //std::cout << "Node " << node << " prime BF returned true\n";
         //what do i do now???
         //std::cout << "##########################################" << "\n";
         //std::cout << "Node " << node << " got a prime " << prime << "\n";
         //std::cout << "##########################################" << "\n";

         foundAllPrimeFactors = true;

      } else {
         std::cout << "Node " << node << " prime BF returned false\n";
         

         //might not be needed
         _status = s_sendNumber;
      }
      return true;
   }
   return false;





   // this->dbNum++;
   
   // if(this->dbNum == 20){
   //    std::string Str("QuitCalc");
   //    _connfd.writeFD(Str);
   // }

   // if(this->dbNum == 30){
   //    std::string Str("NUM 975851579543363");
   //    _connfd.writeFD(Str);
   // }

   // if (this->dbNum < 2){
   //    std::cout << "Waiting for Divisor" << std::endl;
   // }

   // if (!_connfd.hasData())
   // {
   //    //std::cout << "No Data" << std::endl;
   //    return;
   // }
   // std::string cmd;
   // if (!getUserInput(cmd))
   //    return;
   // //lower(cmd);

   // std::cout << "returned: " << cmd << std::endl;

   //_status = s_menu;
}

/**********************************************************************************************
 * getData - Reads in data from the socket and checks to see if there's an end command to the
 *           message to confirm we got it all
 *
 *    Params: None - data is stored in _inputbuf for retrieval with GetInputData
 *
 *    Returns: true if the data is ready to be read, false if they lost connection
 *
 *    Throws: runtime_error for unrecoverable issues
 **********************************************************************************************/

bool TCPConn::getData(std::vector<uint8_t> &buf) {

   std::vector<uint8_t> readbuf;
   size_t count = 0;

   buf.clear();

   while (_connfd.hasData()) {
      // read the data on the socket up to 1024
      count += _connfd.readBytes<uint8_t>(readbuf, 1024);

      // check if we lost connection
      if (readbuf.size() == 0) {
         //std::stringstream msg;
         //std::string ip_addr;
         //msg << "Connection from server " << _node_id << " lost (IP: " << getIPAddrStr(ip_addr) << ")"; 

        // _server_log.writeLog(msg.str().c_str());
         //disconnect();
         return false;
      }

      buf.insert(buf.end(), readbuf.begin(), readbuf.end());

      // concat the data onto anything we've read before
//      _inputbuf.insert(_inputbuf.end(), readbuf.begin(), readbuf.end());
   }
   return true;
}


/**********************************************************************************************
 * sendData - sends the data in the parameter to the socket
 *
 *    Params:  msg - the string to be sent
 *             size - if we know how much data we should expect to send, this should be populated
 *
 *    Throws: runtime_error for unrecoverable errors
 **********************************************************************************************/

bool TCPConn::sendData(std::vector<uint8_t> &buf) {
   
   _connfd.writeBytes<uint8_t>(buf);
   
   return true;
}


/**********************************************************************************************
 * findCmd - returns an iterator to the location of a string where a command starts
 * hasCmd - returns true if command was found, false otherwise
 *
 *    Params: buf = the data buffer to look for the command within
 *            cmd - the command string to search for in the data
 *
 *    Returns: iterator - points to cmd position if found, end() if not found
 *
 **********************************************************************************************/

std::vector<uint8_t>::iterator TCPConn::findCmd(std::vector<uint8_t> &buf, std::vector<uint8_t> &cmd) {
   return std::search(buf.begin(), buf.end(), cmd.begin(), cmd.end());
}

bool TCPConn::hasCmd(std::vector<uint8_t> &buf, std::vector<uint8_t> &cmd) {
   return !(findCmd(buf, cmd) == buf.end());
}


/**********************************************************************************************
 * getCmdData - looks for a startcmd and endcmd and returns the data between the two 
 *
 *    Params: buf = the string to search for the tags
 *            startcmd - the command at the beginning of the data sought
 *            endcmd - the command at the end of the data sought
 *
 *    Returns: true if both start and end commands were found, false otherwisei
 *
 **********************************************************************************************/

bool TCPConn::getCmdData(std::vector<uint8_t> &buf, std::vector<uint8_t> &startcmd, 
                                                    std::vector<uint8_t> &endcmd) {
   std::vector<uint8_t> temp = buf;
   auto start = findCmd(temp, startcmd);
   auto end = findCmd(temp, endcmd);

   if ((start == temp.end()) || (end == temp.end()) || (start == end))
      return false;

   buf.assign(start + startcmd.size(), end);
   return true;
}

/**********************************************************************************************
 * wrapCmd - wraps the command brackets around the passed-in data
 *
 *    Params: buf = the string to wrap around
 *            startcmd - the command at the beginning of the data
 *            endcmd - the command at the end of the data
 *
 **********************************************************************************************/

void TCPConn::wrapCmd(std::vector<uint8_t> &buf, std::vector<uint8_t> &startcmd,
                                                    std::vector<uint8_t> &endcmd) {
   std::vector<uint8_t> temp = startcmd;
   temp.insert(temp.end(), buf.begin(), buf.end());
   temp.insert(temp.end(), endcmd.begin(), endcmd.end());

   buf = temp;
}

/**********************************************************************************************
 * stopProcessing - sets the status of this TCPConn object to s_sendStop to force it to send
 *                  stop command to client.
 *
 **********************************************************************************************/
void TCPConn::stopProcessing(int newNum) {
   std::cout << "Node " << node << " In TCPConn - divisor: " << newNum << std::endl;
   std::cout << "Node " << node << ", In stopProcessing " << newNum << std::endl;

   //this->primeFactor = newNum;
   this->number = this->number/newNum;
   _status = s_sendStop;
}

/**********************************************************************************************
 * getUserInput - Gets user data and includes a buffer to look for a carriage return before it is
 *                considered a complete user input. Performs some post-processing on it, removing
 *                the newlines
 *
 *    Params: cmd - the buffer to store commands - contents left alone if no command found
 *
 *    Returns: true if a carriage return was found and cmd was populated, false otherwise.
 *
 *    Throws: runtime_error for unrecoverable issues
 **********************************************************************************************/

bool TCPConn::getUserInput(std::string &cmd) {
   std::string readbuf;

   // read the data on the socket
   _connfd.readFD(readbuf);

   // concat the data onto anything we've read before
   _inputbuf += readbuf;

   // If it doesn't have a carriage return, then it's not a command
   int crpos;
   if ((crpos = _inputbuf.find("\n")) == std::string::npos)
      return false;

   cmd = _inputbuf.substr(0, crpos);
   _inputbuf.erase(0, crpos+1);

   // Remove \r if it is there
   clrNewlines(cmd);

   return true;
}



/**********************************************************************************************
 * disconnect - cleans up the socket as required and closes the FD
 *
 *    Throws: runtime_error for unrecoverable issues
 **********************************************************************************************/
void TCPConn::disconnect() {
   _connfd.closeFD();
}


/**********************************************************************************************
 * isConnected - performs a simple check on the socket to see if it is still open 
 *
 *    Throws: runtime_error for unrecoverable issues
 **********************************************************************************************/
bool TCPConn::isConnected() {
   return _connfd.isOpen();
}

/**********************************************************************************************
 * getIPAddrStr - gets a string format of the IP address and loads it in buf
 *
 **********************************************************************************************/
void TCPConn::getIPAddrStr(std::string &buf) {
   return _connfd.getIPAddrStr(buf);
}

/**********************************************************************************************
 * sendText - simply calls the sendText FileDesc method to send a string to this FD
 *
 *    Params:  msg - the string to be sent
 *             size - if we know how much data we should expect to send, this should be populated
 *
 *    Throws: runtime_error for unrecoverable errors
 **********************************************************************************************/

int TCPConn::sendText(const char *msg) {
   return sendText(msg, strlen(msg));
}

int TCPConn::sendText(const char *msg, int size) {
   if (_connfd.writeFD(msg, size) < 0) {
      return -1;  
   }
   return 0;
}

bool TCPConn::isNewIPAllowed(std::string inputIP){
   std::ifstream whitelistFile("whitelist");
   if(!whitelistFile){
      std::cout << "whitelist file not found" << std::endl;
      return false;
   }
   
   if (whitelistFile.is_open()){
      std::string line;
      while(whitelistFile >> line){
         //std::cout << "IP: " << inputIP << ", line: " << line << std::endl;//
         if (inputIP == line){
            std::cout << "New connection IP: "<< inputIP << " , authorized from whitelist" << std::endl;
            return true;
         }
      }
   }

   std::cout << "Match NOT FOUND!" << std::endl;
   return false;

}