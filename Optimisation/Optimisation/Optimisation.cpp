/*******************************************
	Optimisation.cpp

	Program to accurately time a function
********************************************/

//#define VERY_SHORT_TEST
//#define SHORT_TEST

#include <conio.h>
#include <iostream>
#include <iomanip>
#include <list>
#include <vector>
using namespace std;

#include "CTimer.h"  // Timer class


/////////////////////////
// Constants

// Number and range of balls, and number of draws held in the container
const int NumBalls = 25;
const int MaxBall = 49;
const int NumDraws = 512;

// Different test lengths and results
#if defined VERY_SHORT_TEST
const int NumIterations = 20000;
const int CorrectResult = 19833669;
#elif defined SHORT_TEST
const int NumIterations = 500000;
const int CorrectResult = -432188177;
#else
const int NumIterations = 10000000;
const int CorrectResult = -1683098354;
#endif


/////////////////////////
// Data structures

// Data structure for function below - a set of lottery ball numbers
struct LotteryDraw
{
	int balls[NumBalls];
};

// A collection of lottery draws
vector<LotteryDraw> Draws;


// Initialise the lottery draw data for the functions below
// Don't optimise this function - it is not timed
void InitialiseFunction()
{
	// Set up random values for each of the draws
	for (int i = 0; i < NumDraws; ++i)
	{
		LotteryDraw draw;
		for (int j = 0; j < NumBalls; j++)
		{
			draw.balls[j] = 1 + (rand() % MaxBall);
		}
		Draws.push_back( draw );
	}
}



/**********************************/
/*     Functions to optimise      */

// Test if i is an odd number
inline bool IsOdd( int i )
{
	return (i&1) == 1;  
}

// Check if given lottery draw has 2+ matching pairs that are even but don't divide by 4, 6 or 8.
// Quite arbitrary conditions - to provide considerable scope for optimisation
bool TestPairs( LotteryDraw draw )
{
	int numPairs = 0;

	// For each ball
	for (int i = 0; i < NumBalls; i++)
	{
		// For each other ball
		for (int j = i+1; j < NumBalls; j++)
		{
			// Assume the balls have valid numbers to start
			


			// Ensure the first ball is before the second ball or we will count pairs twice
			// E.g. if ball 3 and ball 5 are a pair we don't want to count ball 5 and ball 3
			// as another pair
			
			// If the two balls match 
			if (draw.balls[i] == draw.balls[j] && draw.balls[i] % 4 != 0 && draw.balls[i] % 6 != 0 && !IsOdd(draw.balls[i]))
			{
				numPairs++;


				// Check if the first ball divides by 4, 6 or 8

				//if (draw.balls[i] % 4 == 0 || draw.balls[i] % 6 == 0)
				//{
				//	validNumbers = false;
				//	continue;
				//}

				//// Check if either ball is odd
				//if (IsOdd(draw.balls[i]))
				//{
				//	validNumbers = false;
				//	continue;
				//}

				//// ...then increase the number of found pairs
				//if(validNumbers)
				//numPairs++;
			}			
		}
	}

	// Return true if the number of pairs found was >= 2
	return (numPairs >= 2);
}

// The base function to optimise - no real purpose, just an optimisation exercise
int TimedFunction()
{
	// Accumulate a result - checks the optimisation hasn't broken the functionality
	int result = 0;

	// Repeat for many iterations
	for (int i = 0; i < NumIterations; ++i)
	{
		// Select a draw from 0 to the number of draws
		int draw = i % NumDraws;

		// If the iteration is odd and the given draw satisfies the function above...
		if (TestPairs( Draws[draw] ) && IsOdd(i))
		{
			//...then add the iteration to the result
			result += i;
		}
	}

	return result;
}

/*  End of functions to optimise  */
/**********************************/


/////////////////////////
// Test harness

// Main function is a test harness to time the functions above
void main()
{
	// Prepare timer
	CTimer timer;
	cout << fixed << setprecision( 0 );
	cout << "Timer running at " << timer.GetFrequency() << " counts per second" << endl;

	// Initialise data for the functions - no need to optimise this, it is not timed
	InitialiseFunction();
	cout << "Press a key to start process" << endl;
	while (!_kbhit());
	cout << "Processing " << NumIterations << " iterations..." << endl << endl;
	
	timer.Reset();
	int result = TimedFunction();
	float time = timer.GetLapTime();
	cout << "Result: " << result << endl;
	if (result == CorrectResult)
	{
		cout << "Correct result" << endl;
	}
	else
	{
		cout << "****Incorrect Result****" << endl;
	}

	cout << setprecision( 6 );
	cout << "Time passed: " << time << endl << endl;

	system( "pause" );
}