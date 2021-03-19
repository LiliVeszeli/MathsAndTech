// Other example code to highlight certain points


//-----------------

// Class with ill-thought out constructors
class Foo
{
public:
	// Strange use of default constructor - initialising two values here, but one below. 
	// When writing real life classes, consider what to initialise and where to do the initialisation
	Foo()
	{
		a = 1;
		b = 2.5f;
	}

	// Poor choice of constructor code leaves b uninitialised - is that OK (it might be, but usually isn't)
	Foo(int initA)
	{
		a = initA;
	}

	// And what about this kind of construction?
	Foo(float initB) : b(initB)
	{
		// Nothing here, this time the variable a is uninitialised
	}

private:
	int   a;
	float b;
	bool  c = false;
};



//-----------------


// Same class with better construction
class Foo2
{
	// No need to write anything in the default constructor, everything is declared below
	Foo2() = default;
	

	Foo2(int initA)
	{
		a = initA;
		// Other members take the values below
	}

	Foo2(bool initK) : k(initK) // Correct way to initiase a const (can't do it in the code body)
	{
		// Other members take the values below
	}

private:
	const bool k = false; // Const member variable added to show how to use these

	int   a = 1;
	float b = 2.5f;
	bool  c = false;
};