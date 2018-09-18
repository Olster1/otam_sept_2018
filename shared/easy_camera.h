//this function makes a world -> view matrix for a camera looking at a position
Matrix4 easy3d_lookAt(V3 cameraPos, V3 targetPos, V3 upVec) {
	Matrix4 result = mat4();
	V3 direction = v3_minus(cameraPos, targetPos); //do this since we are in right handed coordinate system

	V3 rightVec = normalizeV3(v3_crossProduct(upVec, direction));
	V3 yAxis = normalizeV3(v3_crossProduct(direction, rightVec));
	
	result = mat4_xyzAxis(rightVec, yAxis, normalizeV3(direction));

	result = mat4_transpose(result);

	result = Matrix4_translate(result, v3_negate(cameraPos));

	return result;
}

