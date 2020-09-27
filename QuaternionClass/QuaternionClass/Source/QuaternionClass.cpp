/**************************************************************************************************
	Module:       QuaternionClass.cpp
	Author:       Laurent Noel
	Date created: 01/10/06

	Contains main and testing functions for this console-based project

	Copyright 2006, University of Central Lancashire and Laurent Noel

	Change history:
		V1.0    Created 01/10/06 - LN
**************************************************************************************************/

#include "Defines.h"
#include "Error.h"

#include "CMatrix4x4.h"
#include "CQuaternion.h"
#include "MathIO.h"
using namespace gen; 


// Main function is a test harness for math classes
int main()
{
	GEN_SENTRY;

	cout << "Quaternion Testing" << endl
	     << "------------------" << endl << endl;

	/*CQuaternion q1( 1, 0, 0, 0 );
	CQuaternion q2( 1, 0, 0, 0 );
	CQuaternion q = q1 + q2;
	cout << q1 << " + " << q2 << " = " << q << endl;*/

	//ex 1
	//CMatrix4x4 m1 =
	//	MatrixRotation(CVector3(ToRadians(50.0f), ToRadians(30.0f), 0.0f));
	//CQuaternion q1(m1);
	//CMatrix4x4 m2(q1);
	//cout << m1 << endl << m2 << endl;

	//ex2
	//CMatrix4x4 m1 =
	//	MatrixRotation(CVector3(ToRadians(50.0f), ToRadians(30.0f), 0.0f));
	//CQuaternion q1(m1);

	//CMatrix4x4 mulMatrix  = m1* m1;
	//CQuaternion mulQuat = q1* q1;

	//CMatrix4x4 m2(mulQuat);

	//cout << mulMatrix << endl << m2 << endl;


	//ex5
	CMatrix4x4 m1 =
		MatrixRotation(CVector3(ToRadians(50.0f), ToRadians(30.0f), 0.0f));

	CQuaternion q1(m1);

	CVector3 v(1, 2, 3);
	
	
	CVector3 rotVecQuat = q1.Rotate(v);

	CVector3 rotVecMat = m1.TransformVector(v); //rotates vector by matrix

	
	cout << rotVecQuat << endl << rotVecMat << endl;



	cout << endl;
	system( "pause" );
	return 0;

	GEN_ENDSENTRY;
}
