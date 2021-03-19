#include <iostream>
#include "DNA.h"

using namespace std;

int main()
{
    DNA humanDNA;
	humanDNA.SetSequence("AGTCAGGCCACGTAGTAGAAGTTCTCACACGTTGACGTATCGATCTGACACGTCTAGATCAGTCTCACGTCGTATGCATG");

	DNA frogDNA;
	frogDNA.SetSequence("AGGTACCTGAGTC");

	// Copy monster from human
	DNA monsterDNA;
	monsterDNA.SetSequence(humanDNA.GetSequence());

	// Insert as much frog DNA as possible
	while (monsterDNA.GetNumberSpliceLocations() > 0)
	{
		monsterDNA.Splice(0, frogDNA.GetSequence());
	};

	cout << "Human-frog DNA is: " << monsterDNA.GetSequence() << endl;
}