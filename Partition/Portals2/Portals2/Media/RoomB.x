xof 0303txt 0032
//
// DirectX file: C:\Documents and Settings\Laurent Noel\Desktop\Uclan\Development\CO3303\Portals\Media Source.LN\RoomB.x
//
// Converted by the PolyTrans geometry converter from Okino Computer Graphics, Inc.
// Date/time of export: 01/18/2007 00:17:56
//
// Bounding box of geometry = (-2.5,0.05,3.2) to (-0.5,2.55,4.7).

Header {
	1; // Major version
	0; // Minor version
	1; // Flags
}

Material xof_default {
	0.400000;0.400000;0.400000;1.000000;;
	32.000000;
	0.700000;0.700000;0.700000;;
	0.000000;0.000000;0.000000;;
}

Material Floor {
	1.0;1.0;1.0;1.000000;;
	64.000000;
	0.80000;0.80000;0.80000;;
	0.000000;0.000000;0.000000;;
	TextureFilename {
		"wood2.jpg";
	}
}

Material RoomBPaint {
	0.727200;0.608459;0.466135;1.000000;;
	256.000000;
	0.500000;0.500000;0.500000;;
	0.000000;0.000000;0.000000;;
}

// Top-most frame encompassing the 'World'
Frame Frame_World {
	FrameTransformMatrix {
		1.000000, 0.0, 0.0, 0.0, 
		0.0, 1.000000, 0.0, 0.0, 
		0.0, 0.0, 1.000000, 0.0, 
		0.0, 0.0, 0.0, 1.000000;;
	}

Frame Frame_RoomB {
	FrameTransformMatrix {
		1.000000, 0.0, 0.0, 0.0, 
		0.0, 1.000000, 0.0, 0.0, 
		0.0, 0.0, 1.000000, 0.0, 
		0.0, 0.0, 0.0, 1.000000;;
	}

// Original object name = "RoomB"
Mesh RoomB {
	32;		// 32 vertices
	-2.000000;0.050000;-4.700000;,
	-1.000000;0.050000;-4.700000;,
	-2.000000;2.050000;-4.700000;,
	-1.000000;2.050000;-4.700000;,
	-2.500000;0.050000;-4.700000;,
	-2.500000;0.050000;-4.700000;,
	-2.500000;0.050000;-4.700000;,
	-0.500000;0.050000;-4.700000;,
	-0.500000;0.050000;-4.700000;,
	-0.500000;0.050000;-4.700000;,
	-2.500000;2.550000;-4.700000;,
	-2.500000;2.550000;-4.700000;,
	-2.500000;2.550000;-4.700000;,
	-0.500000;2.550000;-4.700000;,
	-0.500000;2.550000;-4.700000;,
	-0.500000;2.550000;-4.700000;,
	-2.500000;2.550000;-3.200000;,
	-2.500000;2.550000;-3.200000;,
	-2.500000;2.550000;-3.200000;,
	-0.500000;2.550000;-3.200000;,
	-0.500000;2.550000;-3.200000;,
	-0.500000;2.550000;-3.200000;,
	-2.500000;0.050000;-3.200000;,
	-2.500000;0.050000;-3.200000;,
	-2.500000;0.050000;-3.200000;,
	-0.500000;0.050000;-3.200000;,
	-0.500000;0.050000;-3.200000;,
	-0.500000;0.050000;-3.200000;,
	-2.000000;0.050000;-3.200000;,
	-1.000000;0.050000;-3.200000;,
	-2.000000;2.050000;-3.200000;,
	-1.000000;2.050000;-3.200000;;

	16;		// 16 faces
	3;2,6,0;,
	3;2,12,6;,
	3;2,15,12;,
	3;3,15,2;,
	3;3,9,15;,
	3;1,9,3;,
	4;14,21,18,11;,
	3;20,30,17;,
	3;20,31,30;,
	3;30,24,17;,
	3;28,24,30;,
	3;27,31,20;,
	3;27,29,31;,
	4;26,19,13,8;,
	4;5,10,16,23;,
	4;7,4,22,25;;

	MeshMaterialList {
		2; // Number of unique materials
		16; // Number of faces
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1;;
		{RoomBPaint}
		{Floor}
	} // End of MeshMaterialList

	MeshNormals {
		6; // 6 normals
		-1.000000;0.000000;0.000000;,
		0.000000;-1.000000;0.000000;,
		0.000000;0.000000;1.000000;,
		0.000000;0.000000;-1.000000;,
		0.000000;1.000000;0.000000;,
		1.000000;0.000000;0.000000;;

		16;		// 16 faces
		3;2,2,2;,
		3;2,2,2;,
		3;2,2,2;,
		3;2,2,2;,
		3;2,2,2;,
		3;2,2,2;,
		4;1,1,1,1;,
		3;3,3,3;,
		3;3,3,3;,
		3;3,3,3;,
		3;3,3,3;,
		3;3,3,3;,
		3;3,3,3;,
		4;0,0,0,0;,
		4;5,5,5,5;,
		4;4,4,4,4;;
	}  // End of Normals

	MeshTextureCoords {
		32; // 32 texture coords
		0.000000;1.000000;,
		1.000000;1.000000;,
		0.000000;0.000000;,
		1.000000;0.000000;,
		-0.500000;1.250000;,
		1.000000;1.000000;,
		0.000000;1.000000;,
		1.500000;1.250000;,
		0.000000;1.000000;,
		1.000000;1.000000;,
		1.000000;0.000000;,
		0.000000;1.000000;,
		0.000000;0.000000;,
		0.000000;0.000000;,
		1.000000;1.000000;,
		1.000000;0.000000;,
		0.000000;0.000000;,
		0.000000;1.000000;,
		0.000000;0.000000;,
		1.000000;0.000000;,
		1.000000;1.000000;,
		1.000000;0.000000;,
		-0.500000;-0.250000;,
		0.000000;1.000000;,
		0.000000;0.000000;,
		1.500000;-0.250000;,
		1.000000;1.000000;,
		1.000000;0.000000;,
		0.000000;1.000000;,
		1.000000;1.000000;,
		0.000000;0.000000;,
		1.000000;0.000000;;
	}  // End of texture coords
} // End of Mesh
} // End of frame for 'RoomB'
} // End of "World" frame
