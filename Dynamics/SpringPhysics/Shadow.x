xof 0303txt 0032

Material Shadow {
	1.0;1.0;1.0;1.000000;;
	0.000000;
	0.000000;0.000000;0.000000;;
	0.000000;0.000000;0.000000;;
	TextureFilename {
		"Shadow_tlxmul.png";
	}
}
Mesh single_mesh_object {
	4;		// 4 vertices
	-0.500000;-0.000000;0.500000;,
	-0.500000;0.000000;-0.500000;,
	0.500000;-0.000000;0.500000;,
	0.500000;0.000000;-0.500000;;

	2;		// 2 faces
	3;0,2,3;,
	3;0,3,1;;

	MeshMaterialList {
		1;1;0;;
		{Shadow}
	}

	MeshNormals {
		1; // 1 normals
		0.000000;1.000000;0.000000;;

		2;		// 2 faces
		3;0,0,0;,
		3;0,0,0;;
	}  // End of Normals

	MeshTextureCoords {
		4; // 4 texture coords
		0.000000;1.000000;,
		0.000000;0.000000;,
		1.000000;1.000000;,
		1.000000;0.000000;;
	}  // End of texture coords
} // End of Mesh
