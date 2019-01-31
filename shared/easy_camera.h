typedef struct {
	V3 pos;
	Quaternion orientation;
} EasyCamera;

static inline void easy3d_initCamera(EasyCamera *cam, V3 pos) {
	cam->orientation = identityQuaternion();
	cam->pos = pos;
}

static inline Matrix4 easy3d_getViewToWorld(EasyCamera *camera) {
	Matrix4 result = mat4();
	result = quaternionToMatrix(camera->orientation);
	Matrix4 cameraTrans = Matrix4_translate(mat4(), v3_negate(camera->pos));
	return result;
}

static inline Matrix4 easy3d_getWorldToView(EasyCamera *camera) {
	Matrix4 result = mat4();
	result = quaternionToMatrix(camera->orientation);
	result = mat4_transpose(result);
	Matrix4 cameraTrans = Matrix4_translate(mat4(), v3_negate(camera->pos));
	result = Mat4Mult(result, cameraTrans);

	return result;
}

//this function makes a world -> view matrix for a camera looking at a position
Matrix4 easy3d_lookAt(V3 cameraPos, V3 targetPos, V3 upVec) {
	Matrix4 result = mat4();
	V3 direction = v3_minus(targetPos, cameraPos); //assuming we are in left handed coordinate system
	direction = normalizeV3(direction);
	// printf("DIRECTION: %f%f%f\n", direction.x, direction.y, direction.z);

	V3 rightVec = normalizeV3(v3_crossProduct(upVec, direction));
	if(getLengthV3(rightVec) == 0) {
		rightVec = v3(1, 0, 0);
	}
	V3 yAxis = normalizeV3(v3_crossProduct(direction, rightVec));
	assert(getLengthV3(yAxis) != 0);
	
	result = mat4_xyzAxis(rightVec, yAxis, direction);

	// printf("%f %f %f \n", result.a.x, result.a.y, result.a.z);
	// printf("%f %f %f \n", result.b.x, result.b.y, result.b.z);
	// printf("%f %f %f \n", result.c.x, result.c.y, result.c.z);
	// printf("%s\n", "----------------");

	result = mat4_transpose(result);

	Matrix4 cameraTrans = Matrix4_translate(mat4(), v3_negate(cameraPos));

	result = Mat4Mult(result, cameraTrans);

	return result;
}

