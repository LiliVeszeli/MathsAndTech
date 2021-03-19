//******************************************//
// The use of bitwise operators for flags
//******************************************//

#include "Utility.h"
#include <iostream>

// This header file gives you standard integer types with known sizes
// E.g. uint32_t is an unsigned 32-bit (4-byte) integer. These are often used when we are working with the integers at such a low-level 
#include <stdint.h>


// Define some game-like "flags". Flags are a collections of booleans
// Always use "emum class", which forces you to prefix the enum name (e.g. Flags::VSyncOn) - this stops enum names colliding with other variable names
// The type after the colon indicates what type will be used for the enum values - forcing them to be 4 byte unsigned integers here
enum class GPUSettings : uint32_t
{
	// 0b prefix means the number is in binary
	VSync      = 0b0001, // is 1 in decimal
	Shadows    = 0b0010, // is 2 in decimal
	MotionBlur = 0b0100, // is 4 in decimal
	GodRays    = 0b1000, //... etc.
};
ENUM_FLAG_OPERATORS(GPUSettings) // This "macro" removes a lot of casting that would otherwise be needed - see Utility.h



// Define some collections of flags
const GPUSettings DefaultSettings = GPUSettings::Shadows;
const GPUSettings MaxSettings     = GPUSettings::Shadows | GPUSettings::MotionBlur | GPUSettings::GodRays;



int main()
{
    GPUSettings mySettings = DefaultSettings;

	// Enable this flag (on top of any other flags)
	mySettings |= GPUSettings::VSync;

	if ((mySettings & GPUSettings::Shadows) != 0) // Check for a flag being set
	{
		// Initialise shadows
	}

	if ((mySettings & (GPUSettings::GodRays | GPUSettings::MotionBlur)) != 0) // Check for either of these flags being set
	{
		// Prepare post-processing
	}

	// Output flags to prove they are all held in a single integer
	std::cout << static_cast<uint32_t>(mySettings) << std::endl << std::endl;
}
