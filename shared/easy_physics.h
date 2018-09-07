typedef struct {
	bool collided;
	V2 point;
	V2 normal;
	float distance;
} RayCastInfo;

//TODO: Think more about when the line is parrelel to the edge. 

void easy_phys_updatePosAndVel(V3 *pos, V3 *dP, V3 dPP, float dtValue, float dragFactor) {
	*pos = v3_plus(*pos, v3_plus(v3_scale(sqr(dtValue), dPP),  v3_scale(dtValue, *dP)));
	*dP = v3_plus(*dP, v3_minus(v3_scale(dtValue, dPP), v3_scale(dragFactor, *dP)));
}

float getDtValue(float idealFrameTime, int loopIndex, float dt, float remainder) {
	float dtValue = idealFrameTime;
	if((dtValue * (loopIndex + 1)) > dt) {
	    dtValue = remainder;
	}
	return dtValue;
}

//assumes the shape is clockwise
RayCastInfo easy_phys_castRay(V2 startP, V2 ray, V2 *points, int count) {
	isNanErrorV2(startP);
	RayCastInfo result = {};
	if(!(ray.x == 0 && ray.y == 0)) {
		float min_tAt = 0;
		bool isSet = false;
		for(int i = 0; i < count; ++i) {
			V2 pA = points[i];
			isNanErrorV2(pA);
			int bIndex = i + 1;
			if(bIndex == count) {
				bIndex = 0;
			}

			V2 pB = points[bIndex];
			isNanErrorV2(pB);
			V2 endP = v2_plus(startP, ray);
			isNanErrorV2(endP);

			V2 ba = v2_minus(pB, pA);
			isNanErrorV2(ba);

			V2 sa = v2_minus(startP, pA);
			isNanErrorV2(sa);
			V2 ea = v2_minus(endP, pA);
			isNanErrorV2(ea);
			float sideLength = getLength(ba);
			V2 normalBA = normalize_(ba, sideLength);
			isNanErrorV2(normalBA);
			V2 perpBA = perp(normalBA);
			isNanErrorV2(perpBA);
			////
			float endValueX = dotV2(perpBA, ea);
			isNanErrorf(endValueX);
			float startValueX = dotV2(perpBA, sa); 
			isNanErrorf(startValueX);

			float endValueY = dotV2(normalBA, ea);
			isNanErrorf(endValueY);
			float startValueY = dotV2(normalBA, sa); 
			isNanErrorf(startValueY);

			float tAt = inverse_lerp(startValueX, 0, endValueX);
			if(startValueX == endValueX) { //This is when the line is parrellel to the side. 
				if(startValueX != 0) {
					tAt = -1;
				}
			}
			isNanErrorf(tAt);

			if(tAt >= 0 && tAt < 1) {
				float yAt = lerp(startValueY, tAt, endValueY);
				isNanErrorf(yAt);
				if(yAt >= 0 && yAt < sideLength) {
					if(tAt < min_tAt || !isSet) {
						assert(signOf(startValueX) != signOf(endValueX) || (startValueX == 0 && endValueX == 0) || (startValueX == 0.0f || endValueX == 0.0f));
						float xAt = lerp(startValueX, tAt, endValueX); 
						assert(floatEqual_withError(xAt, 0));
						isNanErrorf(tAt);
						isNanErrorf(startValueX);
						isNanErrorf(endValueX);
						isNanErrorf(xAt);
						result.collided = true;
						result.normal = perp(normalBA);
						result.point = v2_plus(v2(xAt, yAt), pA);
						result.distance = getLength(v2_minus(result.point, startP));
						min_tAt = tAt;
					}
				}
			}
		}
	}
	return result;
}

// V2 AP = v2_minus(hitPoint, ent->pos.xy);
// isNanErrorV2(AP);
// V2 BP = v2_minus(hitPoint, testEnt->pos.xy);
// isNanErrorV2(BP);
    
// V2 Velocity_A = v2_plus(ent->dP.xy, v2_scale(ent->dA, perp(AP)));
// isNanErrorV2(Velocity_A);
// V2 Velocity_B = v2_plus(testEnt->dP.xy, v2_scale(testEnt->dA, perp(BP)));
// isNanErrorV2(Velocity_B);

// V2 RelVelocity = v2_minus(Velocity_A, Velocity_B);
// isNanErrorV2(RelVelocity);

// float R = 1.0f; //NOTE(oliver): CoefficientOfRestitution

// float Inv_BodyA_Mass = ent->inverseWeight;
// float Inv_BodyB_Mass = testEnt->inverseWeight;
        
// float Inv_BodyA_I = ent->inverse_I;
// float Inv_BodyB_I = testEnt->inverse_I;
// V2 N = castInfo.normal;
        
// float J_Numerator = dotV2(v2_scale(-(1.0f + R), RelVelocity), N);
// float J_Denominator = dotV2(N, N)*(Inv_BodyA_Mass + Inv_BodyB_Mass) +
//     (sqr(dotV2(perp(AP), N)) * Inv_BodyA_I) + (sqr(dotV2(perp(BP), N)) * Inv_BodyB_I);

// float J = J_Numerator / J_Denominator;
// isNanf(J);

// V2 Impulse = v2_scale(J, N);
// isNanErrorV2(Impulse);
    
// ent->dP.xy = v2_plus(ent->dP.xy, v2_scale(Inv_BodyA_Mass, Impulse));                
// isNanErrorV2(ent->dP.xy);
// testEnt->dP.xy = v2_minus(testEnt->dP.xy, v2_scale(Inv_BodyB_Mass, Impulse));
// isNanErrorV2(testEnt->dP.xy);
//ent->dA = ent->dA + (dotV2(perp(AP), Impulse)*Inv_BodyA_I);
//testEnt->dA = testEnt->dA - (dotV2(perp(BP), Impulse)*Inv_BodyB_I);