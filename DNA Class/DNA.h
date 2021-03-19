//******************************************//
// DNA class definition
//
// Holds a sequence of DNA (as a vector of DNA bases A,C,G,T)
// Allows user to splice new sequences of DNA into the original sequence
// Splicing can only occur where there are certain DNA base patterns
//******************************************//



#ifndef _DNA_H_INCLUDED_
#define _DNA_H_INCLUDED_

#include <string>
#include <vector>
#include <array>




// The DNA is a vector of "DNA bases"
enum DNABase
{
	Adenosine, // A
	Cytosine,  // C
	Guanine,   // G
	Thymine,   // T
};

// We can splice additional sequences of DNA into the existing DNA, but only at locations
// where the  following pattern is found. The added DNA sequence will overwrite this pattern
// The added DNA sequence could itself contain the splice pattern
const std::array<DNABase, 5> SPLICE_PATTERN = { Cytosine, Adenosine, Cytosine, Guanine, Thymine };



class DNA
{
private:
	// The DNA sequence stored as a collection of DNABases
	vector<DNABase> mSequence;

	// Indexes into the above sequence where SPLICE_PATTERN is found
	vector<int> mSpliceLocations;

public:
	DNA() {}

	void SetSequence(string dnaString);
	string GetSequence();

	int GetNumberSpliceLocations()
	{
		return mSpliceLocations.size();
	}

private:
	// Find all the instances of SPLICE_PATTERN in the dna sequence and store each index in mSpliceLocations
	// Needs to be called after every change to the dna sequence
	void UpdateSpliceLocations();

public:
	// Splice the given dna string into the given location (index should be from 0 to GetNumberSpliceLocations() - 1)
	// Returns false if invalid index provided
	bool Splice(int index, string spliceString);
};

#endif