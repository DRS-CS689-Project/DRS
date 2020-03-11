#pragma once

#ifndef DIVFINDERSERVER_H
#define DIVFINDERSERVER_H

#include <list>
#include <string>
#include <thread>
#include <boost/multiprecision/cpp_int.hpp>

using namespace boost::multiprecision;

const unsigned int primecheck_depth = 10;

/* "Unsigned int type to hold original value and calculations" */
#define LARGEINT uint128_t
//#define LARGEINT uint256_t uncomment for 256 bits

/* "Unsigned int twice as large as LARGEINT (bit-wise)" */
#define LARGEINT2X uint256_t

/* "Signed int made of twice the bits as LARGEINT2X" */
#define LARGESIGNED2X int512_t


class DivFinderServer {
public:
    DivFinderServer();
    DivFinderServer(LARGEINT input_value);
    ~DivFinderServer();

    // Overload me 
    //virtual void PolRho() = 0;

    LARGEINT getOrigVal() { return _orig_val; }

    virtual void combinePrimes(std::list<LARGEINT>& dest);
    LARGEINT calcPollardsRho(LARGEINT n);
 
    void setVerbose(int lvl);

    std::list<LARGEINT> primes;

    bool isPrimeBF(LARGEINT n, LARGEINT& divisor);

    void simple();

    void factor();

    void factorSuper();

    void factor(LARGEINT n); 
    void factorThread(LARGEINT n);

    void setEndProcess(bool inputBool) { this->end_process = inputBool; };

    bool end_process = false;

    LARGEINT getPrimeDivFound() { return this->primeDivFound; };




protected:

    LARGEINT2X modularPow(LARGEINT2X base, int exponent, LARGEINT2X modulus);
    
    int verbose = 0;

    LARGEINT primeDivFound = 0;

    // Do not forget, your constructor should call this constructor

private:
    // Prevent instantiation without calling initialization 

    LARGEINT _orig_val;

    // Stuff to be left alone
};

#endif