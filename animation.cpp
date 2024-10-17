// struct Joint {
// 	int *childIndexes;
// };

// struct SkeletalModel {
//     bool valid;
//     ModelBuffer modelBuffer;

// 	int inverseBindMatrixCount;
// 	float16 *inverseBindMatrices;

// 	int jointCount;
// 	Joint *joints;
	
// };

void recursivelyWalkJointChildren(Joint *j, float16 *skinningMatrix, SkeletalModel *model) {
	assert(j);
	if(j) {
		for(int i = 0; i < getArrayLength(j->childIndexes); ++i) {
			//TODO: Replace with animation Transform
			float16 thisJointT = float16_identity();
			skinningMatrix[i] = float16_multiply(skinningMatrix[i], thisJointT);
			Joint *childJoint = &model->joints[i];
			recursivelyWalkJointChildren(childJoint, skinningMatrix, model);
		}
	}
}

void buildSkinningMatrix(SkeletalModel *model) {
	float16 *skinningMatrix = (float16 *)pushArray(&globalPerFrameArena, model->jointCount, float16);

	for(int i = 0; i < model->jointCount; ++i) {
		skinningMatrix[i] = float16_identity();
	}

	assert(model->parentJointIndex >= 0 && model->parentJointIndex < model->jointCount);

	Joint *parentJoint = &model->joints[model->parentJointIndex];
	recursivelyWalkJointChildren(parentJoint, skinningMatrix, model);

	assert(model->jointCount == model->inverseBindMatrixCount);
	//NOTE: Multiply in the inverse bind matrix
	for(int i = 0; i < model->jointCount; ++i) {
		skinningMatrix[i] = float16_multiply(skinningMatrix[i], model->inverseBindMatrices[i]);
	}

	updateSkinningTexture(&model->modelBuffer, skinningMatrix, model->jointCount);
	
}