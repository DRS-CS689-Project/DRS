#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdexcept>
#include <strings.h>
#include <stropts.h>
#include <string.h>
#include <sys/select.h>
#include <stdio.h>
#include <stdexcept>
#include <iostream>
#include <cstdlib>
#include <algorithm>

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/integer/common_factor.hpp>
#include <thread>


#include "TCPClient.h"
#include "DivFinderServer.h"
#include "strfuncts.h"


/**********************************************************************************************
 * TCPClient (constructor) - Creates a Stdin file descriptor to simplify handling of user input. 
 *
 **********************************************************************************************/

TCPClient::TCPClient() {
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

}

/**********************************************************************************************
 * TCPClient (destructor) - No cleanup right now
 *
 **********************************************************************************************/

TCPClient::~TCPClient() {

}

/**********************************************************************************************
 * connectTo - Opens a File Descriptor socket to the IP address and port given in the
 *             parameters using a TCP connection.
 *
 *    Throws: socket_error exception if failed. socket_error is a child class of runtime_error
 **********************************************************************************************/

void TCPClient::connectTo(const char *ip_addr, unsigned short port) {
   if (!_sockfd.connectTo(ip_addr, port))
      throw socket_error("TCP Connection failed!");

}

/**********************************************************************************************
 * handleConnection - Performs a loop that checks if the connection is still open, then 
 *                    looks for user input and sends it if available. Finally, looks for data
 *                    on the socket and sends it.
 * 
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/

void TCPClient::handleConnection() {
      bool connected = true;
   int sin_bufsize = 0;
   ssize_t rsize = 0;

   timespec sleeptime;
   sleeptime.tv_sec = 0;
   sleeptime.tv_nsec = 1000000;


   // Loop while we have a valid connection
   while (connected) {

      // If the connection was closed, exit
      if (!_sockfd.isOpen())
         break;

      // Send any user input
      if ((sin_bufsize = readStdin()) > 0)  {
         std::string subbuf = _in_buf.substr(0, sin_bufsize+1);
         _sockfd.writeFD(subbuf);
         _in_buf.erase(0, sin_bufsize+1);
      }

      // Read any data from the socket and display to the screen and handle errors
      std::vector<uint8_t> buf;
      if (_sockfd.hasData()) {
         if (!getData(buf)) {
            //throw std::runtime_error("Read on client socket failed.");
            continue;
         }

         // Select indicates data, but 0 bytes...usually because it's disconnected
         // if (rsize == 0) {
         //    closeConn();
         //    break;
         // }

         // Display to the screen
         //if (rsize > 0) {
            if (getCmdData(buf, c_num, c_endnum)) {
               std::string numStr(buf.begin(), buf.end());

               this->inputNum = numStr;
               this->initMessage = false;

               //Used 563, 197, 197, 163, 163, 41, 41, 
               uint128_t num = static_cast<uint128_t>(this->inputNum);
               
               this->d = DivFinderServer(num);
               this->d.setVerbose(3);
               
               //runs pollards rho on the number to find the divisor in the separate thread
               this->th = std::make_unique<std::thread>(&DivFinderServer::factorThread, &this->d, num);

               //set flag to alert program is active
               this->activeThread = true;

            }
            else if (!(findCmd(buf, c_stop) == buf.end()))
            {
               //print for debugging
               printf("received stop command!!!!!\n");
               //recieved quit processes signal, shuts down thread
               //if (buf == "QuitCalc"){
                  this->d.setEndProcess(true);
                  this->activeThread = false;
                  this->th->join();
                  this->th.reset();
               //}
               //fflush(stdout);
            }
         //}
      }

      //check if thread was spawned
      if (this->activeThread){
         //send the found divisor to the main server
         if(this->d.getPrimeDivFound() != 0){
            std::cout << "Prime Divisor Found: " << this->d.getPrimeDivFound() << std::endl;
            this->activeThread = false;

            std::string bignumstr  = boost::lexical_cast<std::string>(d.getPrimeDivFound());

            // put it into a byte vector for transmission
            std::vector<uint8_t> buf(bignumstr.begin(), bignumstr.end());

            wrapCmd(buf, c_prime, c_endprime);
               
            std::string mesg = static_cast<std::string>(d.getPrimeDivFound());
            mesg = mesg + "\n"; 
            std::cout << "Sending: " << mesg << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));

               
            //_sockfd.writeFD(mesg);
            _sockfd.writeBytes<uint8_t>(buf);
         }
      }

      nanosleep(&sleeptime, NULL);
   }











   // bool connected = true;
   // int sin_bufsize = 0;
   // ssize_t rsize = 0;

   // timespec sleeptime;
   // sleeptime.tv_sec = 0;
   // sleeptime.tv_nsec = 1000000;


   // // Loop while we have a valid connection
   // while (connected) {

   //    // If the connection was closed, exit
   //    if (!_sockfd.isOpen())
   //       break;

   //    // Send any user input
   //    if ((sin_bufsize = readStdin()) > 0)  {
   //       std::string subbuf = _in_buf.substr(0, sin_bufsize+1);
   //       _sockfd.writeFD(subbuf);
   //       _in_buf.erase(0, sin_bufsize+1);
   //    }

   //    // Read any data from the socket and display to the screen and handle errors
   //    std::string buf;
   //    if (_sockfd.hasData()) {
   //       if ((rsize = _sockfd.readFD(buf)) == -1) {
   //          throw std::runtime_error("Read on client socket failed.");
   //       }

   //       // Select indicates data, but 0 bytes...usually because it's disconnected
   //       if (rsize == 0) {
   //          closeConn();
   //          break;
   //       }

   //       // Display to the screen
   //       if (rsize > 0) {
   //          std::string subbuf2 = buf.substr(0, 3);
   //          //std::cout << "subBuf2: " << subbuf2 << std::endl;//TESTING
   //          //Checks if a number has been sent by the server
   //          //TODO: replace with message wrapp
   //          if (subbuf2 == "NUM"){
   //             std::string left;
   //             std::string right;
   //             split(buf, left, right, ' ');
   //             //std::cout << "right: " << right << std::endl;//TESTING

   //             this->inputNum = right;
   //             this->initMessage = false;

   //             //Used 563, 197, 197, 163, 163, 41, 41, 
   //             uint128_t num = static_cast<uint128_t>(this->inputNum);

   //             boost::multiprecision::uint128_t bignum(this->inputNum);
               
   //             this->d = DivFinderServer(num);
   //             this->d.setVerbose(3);
               
   //             //runs pollards rho on the number to find the divisor in the separate thread
   //             this->th = std::make_unique<std::thread>(&DivFinderServer::factorThread, &this->d, num);

   //             //set flag to alert program is active
   //             this->activeThread = true;

   //          }
   //          else
   //          {
   //             //print for debugging
   //             printf("In else: %s\n", buf.c_str());
   //             //recieved quit processes signal, shuts down thread
   //             if (buf == "QuitCalc"){
   //                this->d.setEndProcess(true);
   //                this->activeThread = false;
   //                this->th->join();
   //                this->th.reset();
   //             }
   //             fflush(stdout);
   //          }
   //       }
   //    }

   //    //check if thread was spawned
   //    if (this->activeThread){
   //       //send the found divisor to the main server
   //       if(this->d.getPrimeDivFound() != 0){
   //          std::cout << "Prime Divisor Found: " << this->d.getPrimeDivFound() << std::endl;
   //          this->activeThread = false;
               
   //          std::string mesg = static_cast<std::string>(d.getPrimeDivFound());
   //          mesg = mesg + "\n"; 
   //          std::cout << "Sending: " << mesg << std::endl;
   //          std::this_thread::sleep_for(std::chrono::seconds(1));

               
   //          _sockfd.writeFD(mesg);
   //       }
   //    }

   //    nanosleep(&sleeptime, NULL);
   // }
}

/**********************************************************************************************
 * closeConnection - Your comments here
 *
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/

void TCPClient::closeConn() {
    _sockfd.closeFD(); 
}

/******************************************************************************
 * readStdin - takes input from the user and stores it in a buffer. We only send
 *             the buffer after a carriage return
 *
 *    Return: 0 if not ready to send, buffer length if ready
 *****************************************************************************/
int TCPClient::readStdin() {

   if (!_stdin.hasData()) {
      return 0;
   }

   // More input, get it and concat it to the buffer
   std::string readbuf;
   int amt_read;
   if ((amt_read = _stdin.readFD(readbuf)) < 0) {
      throw std::runtime_error("Read on stdin failed unexpectedly.");
   }
   
   _in_buf += readbuf;

   // Did we either fill up the buffer or is there a newline/carriage return?
   int sendto;
   if (_in_buf.length() >= stdin_bufsize)
      sendto = _in_buf.length();
   else if ((sendto = _in_buf.find("\n")) == std::string::npos) {
      return 0;
   }
   
   return sendto;
}










bool TCPClient::getData(std::vector<uint8_t> &buf) {

   std::vector<uint8_t> readbuf;
   size_t count = 0;

   buf.clear();

   while (_sockfd.hasData()) {
      // read the data on the socket up to 1024
      count += _sockfd.readBytes<uint8_t>(readbuf, 1024);

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





std::vector<uint8_t>::iterator TCPClient::findCmd(std::vector<uint8_t> &buf, std::vector<uint8_t> &cmd) {
   return std::search(buf.begin(), buf.end(), cmd.begin(), cmd.end());
}

bool TCPClient::getCmdData(std::vector<uint8_t> &buf, std::vector<uint8_t> &startcmd, 
                                                    std::vector<uint8_t> &endcmd) {
   std::vector<uint8_t> temp = buf;
   auto start = findCmd(temp, startcmd);
   auto end = findCmd(temp, endcmd);

   if ((start == temp.end()) || (end == temp.end()) || (start == end))
      return false;

   buf.assign(start + startcmd.size(), end);
   return true;
}

void TCPClient::wrapCmd(std::vector<uint8_t> &buf, std::vector<uint8_t> &startcmd,
                                                    std::vector<uint8_t> &endcmd) {
   std::vector<uint8_t> temp = startcmd;
   temp.insert(temp.end(), buf.begin(), buf.end());
   temp.insert(temp.end(), endcmd.begin(), endcmd.end());

   buf = temp;
}