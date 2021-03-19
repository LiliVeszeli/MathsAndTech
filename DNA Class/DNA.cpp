#include <algorithm>
#include <string>
#include "DNA.h"
 
using namespace std;

void DNA::SetSequence(string dnaString)
{
	mSequence.clear();
	for (auto base : dnaString) // For each character in the string
		switch (base) //...convert to base and add to dna sequence
		{
			case 'A': mSequence.push_back(Adenosine); break;
			case 'C': mSequence.push_back(Cytosine);  break;
			case 'G': mSequence.push_back(Guanine);   break;
			case 'T': mSequence.push_back(Thymine);   break;
		}

	UpdateSpliceLocations();
}

string DNA::GetSequence()
{
	string sequenceString;
	for (auto base : mSequence) // For each base in the DNA sequence...
		switch (base) //...convert to character and add to string
		{
			case Adenosine: sequenceString.push_back('A'); break;
			case Cytosine:  sequenceString.push_back('C'); break;
			case Guanine:   sequenceString.push_back('G'); break;
			case Thymine:   sequenceString.push_back('T'); break;
		}
	return sequenceString;
}


// Find all the instances of SPLICE_PATTERN in the dna sequence and store each index in mSpliceLocations
// Needs to be called after every change to the dna sequence
void DNA::UpdateSpliceLocations()
{
	mSpliceLocations.clear();
	
	// Search for the splice pattern from start of the DNA sequence
	auto spliceLocation = search(mSequence.begin(), mSequence.end(), SPLICE_PATTERN.begin(), SPLICE_PATTERN.end());
	while (spliceLocation != mSequence.end()) // If found
	{
		mSpliceLocations.push_back(spliceLocation - mSequence.begin());

		// Search again from the end of the pattern we just found
		++spliceLocation;
		spliceLocation = search(spliceLocation, mSequence.end(), SPLICE_PATTERN.begin(), SPLICE_PATTERN.end());
	}
}


// Splice the given string into the given location (index should be from 0 to GetNumberSpliceLocations() - 1)
// Returns false if invalid index provided
bool DNA::Splice(int index, string spliceString)
{
	if (index < 0 || index >= mSpliceLocations.size())  return false;

	// Convert string to vector of bases
	vector<DNABase> replaceSequence;
	for (auto base : spliceString) // For each character in the string
		switch (base) //...convert to base and add to sequence
		{
			case 'A': replaceSequence.push_back(Adenosine); break;
			case 'C': replaceSequence.push_back(Cytosine);  break;
			case 'G': replaceSequence.push_back(Guanine);   break;
			case 'T': replaceSequence.push_back(Thymine);   break;
		}

	// Replace splice pattern with given dna sequence
	auto spliceLocation = mSequence.begin() + mSpliceLocations[index];
	spliceLocation = mSequence.erase(spliceLocation, spliceLocation + SPLICE_PATTERN.size());
	mSequence.insert(spliceLocation, replaceSequence.begin(), replaceSequence.end());

	UpdateSpliceLocations();

	return true;
}
