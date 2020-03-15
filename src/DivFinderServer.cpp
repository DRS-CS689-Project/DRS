#include "DivFinderServer.h"
#include <iostream>
#include <cstdlib>
#include <thread>
#include <algorithm>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/integer/common_factor.hpp>
//#include "threadSafeRandGen.h"



DivFinderServer::DivFinderServer() {
}

DivFinderServer::DivFinderServer(LARGEINT number) :_orig_val(number) {
}

DivFinderServer::~DivFinderServer() {
}


void DivFinderServer::setVerbose(int lvl) {
    if ((lvl < 0) || (lvl > 3))
        throw std::runtime_error("Attempt to set invalid verbosity level. Lvl: (0-3)\n");
    verbose = lvl;
}

/********************************************************************************************
 * modularPow - function to gradually calculate (x^n)%m to avoid overflow issues for
 *              very large non-prime numbers using the stl function pow (floats)
 *
 *    Params:  base - the base value x in the above function
 *             exponent - the exponent n
 *             modulus - the modulus m
 *
 *    Returns: resulting number
 ********************************************************************************************/
LARGEINT2X DivFinderServer::modularPow(LARGEINT2X base, int exponent, LARGEINT2X modulus) {
    LARGEINT2X result = 1;

    while (exponent > 0) {

        // If the exponent is odd, calculate our results 
        if (exponent & 1) {
            result = (result * base) % modulus;
        }

        // exponent = exponent / 2 (rounded down)
        exponent = exponent >> 1;

        base = (base * base) % modulus;
    }
    return result;

}

/**********************************************************************************************
 * calcPollardsRho - Do the actual Pollards Rho calculations to attempt to find a divisor
 *
 *    Params:  n - the number to find a divisor within
 *
 *    Returns: a divisor if found, otherwise n
 *
 *
 **********************************************************************************************/

LARGEINT DivFinderServer::calcPollardsRho(LARGEINT n) {
    //std::cout << "IN PR" << std::endl;

    if (n <= 3)
        return n;

    // Initialize our random number generator
    //srand(time(NULL));
    int seed = std::hash<std::thread::id>{}(std::this_thread::get_id()) + static_cast<long int>(time(NULL));
    srand(seed);

    //thread_local unsigned int seed1 = time(NULL) + rand();
    //thRandGen::Seed(time(0));
    //std::cout << "After seed" << std::endl;
    // pick a random number from the range [2, N)
    LARGEINT2X x = (rand() % (n - 2)) + 2;
    //LARGEINT2X x = (rand_r(&seed1) % (n - 2)) + 2;
    LARGEINT2X y = x;    // Per the algorithm

    //thread_local unsigned int seed2 = time(NULL) + rand();
    // random number for c = [1, N)
    LARGEINT2X c = (rand() % (n - 1)) + 1;
    //LARGEINT2X c = (rand_r(&seed1) % (n - 1)) + 1;

    LARGEINT2X d = 1;
    if (verbose == 3)
        std::cout << "x: " << x << ", y: " << y << ", c: " << c << std::endl;

    // Loop until either we find the gcd or gcd = 1
    while (d == 1 && !end_process) {
        // "Tortoise move" - Update x to f(x) (modulo n)
        // f(x) = x^2 + c f
        x = (modularPow(x, 2, n) + c + n) % n;


        // "Hare move" - Update y to f(f(y)) (modulo n)
        y = (modularPow(y, 2, n) + c + n) % n;
        y = (modularPow(y, 2, n) + c + n) % n;


        // Calculate GCD of |x-y| and tn
        LARGESIGNED2X z = (LARGESIGNED2X)x - (LARGESIGNED2X)y;
        if (verbose == 3)
            std::cout << "x: " << x << ", y: " << y << ", z: " << z << std::endl;

        if (z < 0)
            d = boost::math::gcd((LARGEINT2X)-z, (LARGEINT2X)n);
        else
            d = boost::math::gcd((LARGEINT2X)z, (LARGEINT2X)n);

        // If we found a divisor, factor primes out of each side of the divisor
        if ((d != 1) && (d != n)) {

            if (end_process) {
                return 0;
            }
            else {
                return (LARGEINT)d;
            }

        }

    }
    if (end_process) {
        return 0;
    }
    return (LARGEINT)d;
}



bool DivFinderServer::isPrimeBF(LARGEINT n, LARGEINT& divisor) {
    if (verbose >= 3)
        std::cout << "Checking if prime: " << n << std::endl;

    if (n == 316969)
        std::cout << "Checking if prime: " << n << std::endl;

    divisor = 0;

    // Take care of simple cases
    if (n <= 3) {
        return n > 1;
    }
    else if ((n % 2) == 0) {
        divisor = 2;
        return false;
    }
    else if ((n & 3) == 0) {
        divisor = 3;
        return false;
    }

    // Assumes all primes are to either side of 6k. Using 256 bit to avoid overflow
    // issues when calculating max range
    LARGEINT2X n_256t = n;
    for (LARGEINT2X k = 5; k * k < n_256t; k = k + 6) {
        if ((n_256t % k == 0) || (n_256t % (k + 2) == 0)) {
            divisor = (LARGEINT)k;
            return false;
        }
    }
    return true;
}

/*******************************************************************************
 *
 * factor - Calculates a single prime of the given number and recursively calls
 *          itself to continue calculating primes of the remaining number. Variation
 *          on the algorithm by Yash Varyani on GeeksForGeeks. Uses a single
 *          process
 *
 *
 ******************************************************************************/

void DivFinderServer::factorThread(LARGEINT n) {

    // already prime
    if (n == 1) {
        return;
    }

    if (verbose >= 2)
        std::cout << "Factoring: " << n << std::endl;

    bool div_found = false;
    unsigned int iters = 0;

    while (!div_found && !end_process) {
        if (verbose >= 3)
            std::cout << "Starting iteration: " << iters << std::endl;

        // If we have tried Pollards Rho a specified number of times, run the
        // costly prime check to see if this is a prime number. Also, increment
        // iters after the check
        if (iters++ == primecheck_depth) {
            if (verbose >= 2)
                std::cout << "Pollards rho timed out, checking if the following is prime: " << n << std::endl;
            LARGEINT divisor;
            if (!isPrimeMR(n, 10)){
                if (isPrimeBF(n, divisor)) {
                    if (verbose >= 2)
                        std::cout << "Prime found: " << n << std::endl;
                    this->primeDivFound = n;
                    return;
                }
                else {   // We found a prime divisor, save it and keep finding primes
                    if (verbose >= 2)
                        std::cout << "Prime found: " << divisor << std::endl;
                    this->primeDivFound = divisor;
                    return;
                }
            }
            else{
                this->primeDivFound = n;
                return;
            }
        }

        // We try to get a divisor using Pollards Rho
        LARGEINT d = calcPollardsRho(n);
        //std::this_thread::sleep_for(std::chrono::microseconds(20));
        if (d == 0) {
            return;
        }

        else if (d != n) {
            if (verbose >= 1)
                std::cout << "Divisor found: " << d << std::endl;

            // Factor the divisor
            factorThread(d);

            return;
        }

        // If d == n, then we re-randomize and continue the search up to the prime check depth
    }
    std::cout << "process end signal detected" << std::endl;
    return;
}


LARGEINT2X DivFinderServer::power(LARGEINT2X x, LARGEINT2X y, LARGEINT2X p) 
{ 
    LARGEINT2X res = 1;      // Initialize result 
    x = x % p;  // Update x if it is more than or 
                // equal to p 
    while (y > 0) 
    { 
        // If y is odd, multiply x with result 
        if (y & 1) 
            res = (res*x) % p; 
  
        // y must be even now 
        y = y>>1; // y = y/2 
        x = (x*x) % p; 
    } 
    return res; 
} 
  
// This function is called for all k trials. It returns 
// false if n is composite and returns false if n is 
// probably prime. 
// d is an odd number such that  d*2<sup>r</sup> = n-1 
// for some r >= 1 
bool DivFinderServer::millerTest(LARGEINT2X d, LARGEINT2X n) 
{ 
    // Pick a random number in [2..n-2] 
    // Corner cases make sure that n > 4 
    LARGEINT2X a = 2 + rand() % (n - 4); 
  
    // Compute a^d % n 
    LARGEINT2X x = power(a, d, n); 
  
    if (x == 1  || x == n-1) 
       return true; 
  
    // Keep squaring x while one of the following doesn't 
    // happen 
    // (i)   d does not reach n-1 
    // (ii)  (x^2) % n is not 1 
    // (iii) (x^2) % n is not n-1 
    while (d != n-1) 
    { 
        x = (x * x) % n; 
        d *= 2; 
  
        if (x == 1)      return false; 
        if (x == n-1)    return true; 
    } 
  
    // Return composite 
    return false; 
} 
  
// It returns false if n is composite and returns true if n 
// is probably prime.  k is an input parameter that determines 
// accuracy level. Higher value of k indicates more accuracy. 
//bool isPrime(int n, int k) 
bool DivFinderServer::isPrimeMR(LARGEINT n, int k) 
{ 
    std::cout << "Checking if prime: " << n << std::endl;
    // Corner cases 
    if (n <= 1 || n == 4)  return false; 
    if (n <= 3) return true; 
  
    // Find r such that n = 2^d * r + 1 for some r >= 1 
    LARGEINT2X d = n - 1; 
    while (d % 2 == 0) 
        d /= 2; 
  
    // Iterate given nber of 'k' times 
    //LARGEINT2X k_256t = k;
    for (LARGEINT2X i = 0; i < k; i++) 
         if (!millerTest(d, n)) 
              return false; 
  
    return true; 
}