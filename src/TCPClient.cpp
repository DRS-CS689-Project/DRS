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
      std::string buf;
      if (_sockfd.hasData()) {
         if ((rsize = _sockfd.readFD(buf)) == -1) {
            throw std::runtime_error("Read on client socket failed.");
         }

         // Select indicates data, but 0 bytes...usually because it's disconnected
         if (rsize == 0) {
            closeConn();
            break;
         }

         // Display to the screen
         if (rsize > 0) {
            std::string subbuf2 = buf.substr(0, 3);
            std::cout << "subBuf2: " << subbuf2 << std::endl;
            if (subbuf2 == "NUM"){
               std::cout << "1st Message" << std::endl;
               std::string left;
               std::string right;
               split(buf, left, right, ' ');
               std::cout << "right: " << right << std::endl;

               this->inputNum = right;
               this->initMessage = false;

               	//Used 563, 197, 197, 163, 163, 41, 41, 
               uint128_t num = static_cast<uint128_t>(this->inputNum);
               
               //auto d = DivFinderServer(num);
               this->d = DivFinderServer(num);
               this->d.setVerbose(3);

               
               //std::thread th(&DivFinderServer::simple, &d);
               //std::thread th(&DivFinderServer::factorThread, &this->d, num);
               this->th = new std::thread(&DivFinderServer::factorThread, &this->d, num);
               //d.factorThread(num);

               //std::this_thread::sleep_for(std::chrono::seconds(15));
               //std::cout << "Ended Process" << std::endl;
               //d.setEndProcess(true);
               this->activeThread = true;
               
	            //th.join();
               /*
	            auto primeFound = d.getPrimeDivFound();

               std::cout << "Prime Divisor Found : " << primeFound << std::endl;
               
               std::string mesg = static_cast<std::string>(d.getPrimeDivFound());
               mesg = mesg + "\n"; 
               std::cout << "Sending: " << mesg << std::endl;
               std::this_thread::sleep_for(std::chrono::seconds(1));

               
               _sockfd.writeFD(mesg);*/

            }
            //std::cout << "Recieved string: " << this->inputNum << std::endl;
            else{
               printf("In else: %s\n", buf.c_str());
               if (buf == "QuitCalc")
                  this->d.setEndProcess(true);
               fflush(stdout);
            }
         }
      }

      if (this->activeThread){
         if(this->d.getPrimeDivFound() != 0){
            std::cout << "Prime Divisor Found: " << this->d.getPrimeDivFound() << std::endl;
            this->activeThread = false;
               
            std::string mesg = static_cast<std::string>(d.getPrimeDivFound());
            mesg = mesg + "\n"; 
            std::cout << "Sending: " << mesg << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));

               
            _sockfd.writeFD(mesg);
         }
      }

      nanosleep(&sleeptime, NULL);
   }
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


