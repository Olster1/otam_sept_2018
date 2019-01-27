#define EASY_UI_BUTTON_COLOR COLOR_GREY

static inline bool EasyUI_UpdateCheckBox(bool active, Texture *tex, AppKeyStates *keyStates, V3 pos, V3 dim, Matrix4 metresToPixels, V2 resolution, float dt, LerpV4 *cLerp) {
	RenderInfo renderInfo = calculateRenderInfo(pos, dim, v3(0, 0, 0), metresToPixels);
	bool result = false;
	Rect2f outputDim = rect2fCenterDimV2(renderInfo.transformPos.xy, renderInfo.transformDim.xy);
	V4 uiColor = cLerp->value;
	if(active) {
		float lerpPeriod = 0.3f;
		if(!updateLerpV4(cLerp, dt, LINEAR)) {
		    if(!easyLerp_isAtDefault(cLerp)) {
		        setLerpInfoV4_s(cLerp, COLOR_WHITE, 0.01, &cLerp->value);
		    }
		}
		
		
		if(inBounds(keyStates->mouseP_yUp, outputDim, BOUNDS_RECT)) {
		    setLerpInfoV4_s(cLerp, EASY_UI_BUTTON_COLOR, 0.2f, &cLerp->value);
		    if(wasPressed(keyStates->gameButtons, BUTTON_LEFT_MOUSE)) {
		        result = true;
		    }
		}
	}
	
	renderTextureCentreDim(tex, renderInfo.pos, renderInfo.dim.xy, uiColor, 0, mat4(), mat4(), Mat4Mult(OrthoMatrixToScreen(resolution.x, resolution.y), renderInfo.pvm)); 

	return result;
}

static inline bool EasyUI_UpdateButton(bool active, bool *holding, Texture *tex, Font *font, char *title, AppKeyStates *keyStates, float fontSize, V3 pos, Matrix4 metresToPixels, Matrix4 pixelsToMeters, V2 resolution, float dt, LerpV4 *cLerp, float resolutionDiffScale) {
	Rect2f margin = rect2f(0, 0, resolution.x, resolution.y);
	
	RenderInfo renderInfo = calculateRenderInfo(pos, v3(1, 1, 1), v3(0, 0, 0), metresToPixels);

	float xAt = renderInfo.transformPos.x;
	float yAt = renderInfo.transformPos.y;
	float zAt = pos.z;

	Rect2f outputDim = getBoundsRectf(title, xAt, yAt, margin, font, fontSize, resolution, resolutionDiffScale);

	V2 buttonDim = v2_scale(2.0f, getDim(outputDim));
	buttonDim.x *= 2.0f;
	Rect2f interactDim = rect2fCenterDimV2(getCenter(outputDim), buttonDim);
	bool result = false;
	V4 uiColor = cLerp->value;
	if(active) {
	
		float lerpPeriod = 0.3f;
		if(!updateLerpV4(cLerp, dt, LINEAR)) {
		    if(!easyLerp_isAtDefault(cLerp)) {
		        setLerpInfoV4_s(cLerp, COLOR_WHITE, 0.01, &cLerp->value);
		    }
		}
		
		
		if(inBounds(keyStates->mouseP, interactDim, BOUNDS_RECT)) {
		    setLerpInfoV4_s(cLerp, v4(0.8f, 0.8f, 0.8f, 1), 0.2f, &cLerp->value);
		    if(wasPressed(keyStates->gameButtons, BUTTON_LEFT_MOUSE)) {
		    	*holding = true;
		    }
		}

		if(inBounds(keyStates->mouseP, interactDim, BOUNDS_RECT)) {
			if(wasReleased(keyStates->gameButtons, BUTTON_LEFT_MOUSE)) {
				*holding = false;
				result = true;
			}
		}
	}

	if(wasReleased(keyStates->gameButtons, BUTTON_LEFT_MOUSE)) {
		*holding = false;
	}

	V2 dim = getDim(outputDim);
	renderTextureCentreDim(tex, v3(xAt + 0.5f*dim.x, yAt - 0.5f*dim.y, zAt), buttonDim, uiColor, 0, mat4TopLeftToBottomLeft(resolution.y), OrthoMatrixToScreen_BottomLeft(resolution.x, resolution.y), mat4()); 
	outputText(font, xAt, yAt - 5, zAt + 0.1f, resolution, title, margin, COLOR_BLACK, fontSize, true, resolutionDiffScale);

	return result;
}