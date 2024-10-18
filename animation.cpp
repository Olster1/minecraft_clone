// struct Joint {
// 	int *childIndexes;
// };

// struct KeyFrame {
// 	float time;
// 	float4 transform;
// };

// enum BoneAnimationType {
// 	BONE_ANIMATION_ROTATION,
// 	BONE_ANIMATION_TRANSLATION,
// 	BONE_ANIMATION_SCALE,
// };

// struct BoneAnimation {
// 	int keyFrameCount;
// 	KeyFrame *keyFrames;
	
// 	int boneIndex;
// 	BoneAnimationType type;
// };

// struct Animation3d {
// 	char *name; //NOTE: Lives on Heap

// 	int boneCount;
// 	BoneAnimation *boneAnimations;
// };

// struct SkeletalModel {
//     bool valid;
//     ModelBuffer modelBuffer;

// 	int inverseBindMatrixCount;
// 	float16 *inverseBindMatrices;

// 	int jointCount;
// 	Joint *joints;

// 	int animationCount;
// 	Animation3d *animations;

// 	int parentJointIndex;
	
// };


void updateAnimationState(GameState *gameState, AnimationState *animationState) {
	animationState->timeAt += gameState->dt;

	Animation3d *a = animationState->animation.animation;

	while(animationState->timeAt > a->maxTime) {
		if(animationState->animation.next) {
			animationState->animation.animation = animationState->animation.next;
			animationState->animation.next = 0;
			animationState->timeAt = 0;	
		} else {
			animationState->timeAt -= a->maxTime;
		}
	}
}

void recursivelyWalkJointChildren(Joint *j, float16 *skinningMatrix, SkeletalModel *model, float16 *perJointTransforms) {
	assert(j);
	if(j) {
		for(int i = 0; i < getArrayLength(j->childIndexes); ++i) {
			int boneIndex = j->childIndexes[i];
			float16 thisJointT = perJointTransforms[boneIndex];
			skinningMatrix[boneIndex] = float16_multiply(skinningMatrix[boneIndex], thisJointT);
			Joint *childJoint = &model->joints[boneIndex];
			recursivelyWalkJointChildren(childJoint, skinningMatrix, model, perJointTransforms);
		}
	}
}

void buildSkinningMatrix(GameState *gameState, SkeletalModel *model, AnimationState *animationState) {
	updateAnimationState(gameState, animationState);

	float16 *skinningMatrix = (float16 *)pushArray(&globalPerFrameArena, model->jointCount, float16);
	SQT *perJointTransform = (SQT *)pushArray(&globalPerFrameArena, model->jointCount, SQT);
	float16 *perJointTransforms = (float16 *)pushArray(&globalPerFrameArena, model->jointCount, float16);

	for(int i = 0; i < model->jointCount; ++i) {
		perJointTransforms[i] = skinningMatrix[i] = float16_identity();
		perJointTransform[i] = SQT_identity();
	}

	Animation3d *animationOn = animationState->animation.animation;

	assert(animationOn);

	for(int i = 0; i < animationOn->boneCount; ++i) {
		BoneAnimation *bone = &animationOn->boneAnimations[i];

		SQT *T = &perJointTransform[bone->boneIndex];

		float4 a;
		float4 b;
		float t = 0;

		for(int k = 0; k < bone->keyFrameCount - 1; ++k) {
			KeyFrame *frame = &bone->keyFrames[k];
			KeyFrame *frame2 = &bone->keyFrames[k + 1];

			if(animationState->timeAt >= frame->time && animationState->timeAt <= frame2->time) {
				t = (animationState->timeAt - frame->time) / (frame2->time - frame->time);
				a = frame->transform;
				b = frame2->transform;
				break;
			}
		}

		assert(t >= 0 && t <= 1.0f);
		
		a = lerp_float4(a, b, t);

		if(bone->type == BONE_ANIMATION_ROTATION) {
			T->rotation = easyMath_normalizeQuaternion(quaternion_mult(easyMath_normalizeQuaternion(quaternion(a.x, a.y, a.z, a.w)), quaternion(T->rotation.x, T->rotation.y, T->rotation.z, T->rotation.w))).vector4;
		} else if(bone->type == BONE_ANIMATION_TRANSLATION) {
			T->translate = plus_float3(a.xyz, T->translate);
		} else if(bone->type == BONE_ANIMATION_SCALE) {
			T->scale = float3_hadamard(a.xyz, T->scale);
		}
	}

	for(int i = 0; i < model->jointCount; ++i) {
		SQT sqt = perJointTransform[i];
		perJointTransforms[i] = sqt_to_float16(quaternion(sqt.rotation.x, sqt.rotation.y, sqt.rotation.z, sqt.rotation.w), sqt.scale, sqt.translate);
	}

	assert(model->parentJointIndex >= 0 && model->parentJointIndex < model->jointCount);

	Joint *parentJoint = &model->joints[model->parentJointIndex];
	recursivelyWalkJointChildren(parentJoint, skinningMatrix, model, perJointTransforms);

	assert(model->jointCount == model->inverseBindMatrixCount);
	//NOTE: Multiply in the inverse bind matrix
	for(int i = 0; i < model->jointCount; ++i) {
		skinningMatrix[i] = float16_multiply(skinningMatrix[i], model->inverseBindMatrices[i]);
	}

	updateSkinningTexture(&model->modelBuffer, skinningMatrix, model->jointCount);
	
}