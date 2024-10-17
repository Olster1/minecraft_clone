#define CGLTF_IMPLEMENTATION
#include "./libs/cgltf.h"

/*
structs of interest: 

typedef struct cgltf_mesh {
	char* name;
	cgltf_primitive* primitives;
	cgltf_size primitives_count;
	cgltf_float* weights;
	cgltf_size weights_count;
	char** target_names;
	cgltf_size target_names_count;
	cgltf_extras extras;
	cgltf_size extensions_count;
	cgltf_extension* extensions;
} cgltf_mesh;


typedef enum cgltf_primitive_type
{
	cgltf_primitive_type_invalid,
	cgltf_primitive_type_points,
	cgltf_primitive_type_lines,
	cgltf_primitive_type_line_loop,
	cgltf_primitive_type_line_strip,
	cgltf_primitive_type_triangles,
	cgltf_primitive_type_triangle_strip,
	cgltf_primitive_type_triangle_fan,
	cgltf_primitive_type_max_enum
} cgltf_primitive_type;


typedef struct cgltf_primitive {
	cgltf_primitive_type type;
	cgltf_accessor* indices;  //IMPORTANT: The indicies array
	cgltf_material* material;
	cgltf_attribute* attributes;  //IMPORTANT: The attribute arrays
	cgltf_size attributes_count;
	cgltf_morph_target* targets;
	cgltf_size targets_count;
	cgltf_extras extras;
	cgltf_bool has_draco_mesh_compression;
	cgltf_draco_mesh_compression draco_mesh_compression;
	cgltf_material_mapping* mappings;
	cgltf_size mappings_count;
	cgltf_size extensions_count;
	cgltf_extension* extensions;
} cgltf_primitive;

typedef enum cgltf_attribute_type
{
	cgltf_attribute_type_invalid,
	cgltf_attribute_type_position,
	cgltf_attribute_type_normal,
	cgltf_attribute_type_tangent,
	cgltf_attribute_type_texcoord,
	cgltf_attribute_type_color,
	cgltf_attribute_type_joints, //IMPORTANT: For Skeletal Animation
	cgltf_attribute_type_weights, //IMPORTANT: For Skeletal Animation
	cgltf_attribute_type_custom,
	cgltf_attribute_type_max_enum
} cgltf_attribute_type;

typedef struct cgltf_attribute  //IMPORTANT: Points to the buffer view
{
	char* name;
	cgltf_attribute_type type;
	cgltf_int index;
	cgltf_accessor* data;
} cgltf_attribute;

typedef enum cgltf_type
{
	cgltf_type_invalid,
	cgltf_type_scalar,
	cgltf_type_vec2,
	cgltf_type_vec3,
	cgltf_type_vec4,
	cgltf_type_mat2,
	cgltf_type_mat3,
	cgltf_type_mat4,
	cgltf_type_max_enum
} cgltf_type;

typedef enum cgltf_component_type
{
	cgltf_component_type_invalid,
	cgltf_component_type_r_8,  BYTE 
	cgltf_component_type_r_8u, UNSIGNED_BYTE 
	cgltf_component_type_r_16,  SHORT 
	cgltf_component_type_r_16u,  UNSIGNED_SHORT 
	cgltf_component_type_r_32u,  UNSIGNED_INT 
	cgltf_component_type_r_32f, FLOAT 
    cgltf_component_type_max_enum
} cgltf_component_type;

typedef struct cgltf_accessor_sparse
{
	cgltf_size count;
	cgltf_buffer_view* indices_buffer_view;
	cgltf_size indices_byte_offset;
	cgltf_component_type indices_component_type;
	cgltf_buffer_view* values_buffer_view;
	cgltf_size values_byte_offset;
} cgltf_accessor_sparse;


typedef struct cgltf_accessor
{
	char* name;
	cgltf_component_type component_type;
	cgltf_bool normalized;
	cgltf_type type;
	cgltf_size offset;
	cgltf_size count;
	cgltf_size stride;
	cgltf_buffer_view* buffer_view;
	cgltf_bool has_min;
	cgltf_float min[16];
	cgltf_bool has_max;
	cgltf_float max[16];
	cgltf_bool is_sparse; //IMPORTANT: This is if we should replace some of the data with this ones
	cgltf_accessor_sparse sparse; //IMPORTANT: This is the data to replace with
	cgltf_extras extras;
	cgltf_size extensions_count;
	cgltf_extension* extensions;
} cgltf_accessor;

typedef enum cgltf_buffer_view_type
{
	cgltf_buffer_view_type_invalid,
	cgltf_buffer_view_type_indices,
	cgltf_buffer_view_type_vertices,
	cgltf_buffer_view_type_max_enum
} cgltf_buffer_view_type;

typedef struct cgltf_buffer_view
{
	char *name;
	cgltf_buffer* buffer;
	cgltf_size offset;
	cgltf_size size;
	cgltf_size stride;  0 == automatically determined by accessor 
	cgltf_buffer_view_type type;
	void* data; // overrides buffer->data if present, filled by extensions 
	cgltf_bool has_meshopt_compression;
	cgltf_meshopt_compression meshopt_compression;
	cgltf_extras extras;
	cgltf_size extensions_count;
	cgltf_extension* extensions;
} cgltf_buffer_view;

typedef struct cgltf_buffer
{
	char* name;
	cgltf_size size;
	char* uri;
	void* data; //: IMPORTANT: loaded by cgltf_load_buffers
	cgltf_data_free_method data_free_method;
	cgltf_extras extras;
	cgltf_size extensions_count;
	cgltf_extension* extensions;
} cgltf_buffer;

*/

struct Joint {
	int *childIndexes;
};

struct SkeletalModel {
    bool valid;
    ModelBuffer modelBuffer;

	int inverseBindMatrixCount;
	float16 *inverseBindMatrices;

	int jointCount;
	Joint *joints;

	int parentJointIndex;
	
};

void copyBufferToMemory(void *bufferToCopyTo, uint8_t *ptr, cgltf_buffer_view *buffer_view, cgltf_accessor *accessor, bool isShort, int floatCount) {
	uint8_t *from = ptr + buffer_view->offset + accessor->offset;
	uint8_t *to = (uint8_t *)bufferToCopyTo;

	size_t sizeOfElm = (isShort) ? sizeof(u16) : sizeof(float);

	int buffer_view_stride = buffer_view->stride;
	if(buffer_view_stride == 0) {
		buffer_view_stride = accessor->stride;
		if(buffer_view_stride == 0) {
			buffer_view_stride = sizeOfElm * floatCount;
		}
	}
 
	//NOTE: This is to account if the data is interleaved, we extract out into it's own SOA instead of AOS
	assert(sizeOfElm*floatCount <= buffer_view_stride);
	for(int i = 0; i < buffer_view->size; i += buffer_view_stride) {
		memcpy(to, from + i, sizeOfElm*floatCount);

		to += sizeOfElm*floatCount;
	}
}

SkeletalModel loadGLTF(char *fileName) {
    SkeletalModel model = {};
    model.valid = false;

    cgltf_options options = {};
    cgltf_data* data = NULL;

	int vertexCount = 0;
	VertexWithJoints *vertexes = 0;

	int indexCount = 0;
	unsigned int *indiciesToSend = 0;

    //NOTE: Parse the file
    cgltf_result result = cgltf_parse_file(&options, fileName, &data);
    if (result == cgltf_result_success) {
        //NOTE: Load the buffers - .bin files or imbeded in the file
        result = cgltf_load_buffers(&options, data, fileName);

        if (result == cgltf_result_success) {
            result = cgltf_validate(data);

            if (result == cgltf_result_success) {

                //NOTE: Each file will have a list of meshes 
                //      We're interested in the primitives of this (i.e. triangles, triangle strips, lines) AND the indices
                //  
				float *poss = 0;
				float *uvs = 0;
				float *normals = 0;
				u16 *joints = 0;
				float *weights = 0;
				unsigned short *indicies = 0;

				int normalCount = 0;
				int uvCount = 0;
				int jointCount = 0;
				int weightsCount = 0;
				

				{
					cgltf_accessor *indiciesAccessor = data->meshes[0].primitives[0].indices;
					//NOTE: Get the index data 
					if(indiciesAccessor) {
						cgltf_buffer_view *buffer_view = indiciesAccessor->buffer_view;
						cgltf_buffer* buffer = buffer_view->buffer;
						uint8_t *ptr = (uint8_t *)buffer->data;

						assert(indiciesAccessor->component_type == cgltf_component_type_r_16u);
						// assert(buffer_view->type == cgltf_buffer_view_type_indices);

						indicies = (unsigned short *)malloc(buffer_view->size);

						uint8_t *p = ptr + buffer_view->offset;
						memcpy(indicies, p, buffer_view->size);

						indexCount = indiciesAccessor->count;
					} else {
						indexCount = 0;
						indicies = 0;

					}
				}

				assert(data->skins_count < 2);
				if(data->skins_count > 0) {
					cgltf_skin skin = data->skins[0];
					cgltf_accessor *inverse_bind_matrices = skin.inverse_bind_matrices;
					assert(inverse_bind_matrices->count == skin.joints_count);
					cgltf_buffer_view *buffer_view = inverse_bind_matrices->buffer_view;
					cgltf_buffer* buffer = buffer_view->buffer;
					uint8_t *ptr = (uint8_t *)buffer->data;
					
					model.inverseBindMatrices = (float16 *)malloc(sizeof(float16)*inverse_bind_matrices->count);
					copyBufferToMemory(model.inverseBindMatrices, ptr, buffer_view, inverse_bind_matrices, false, 16);
					model.inverseBindMatrixCount = inverse_bind_matrices->count;

					assert(skin.joints_count < MAX_BONES_PER_MODEL);

					model.joints = (Joint *)malloc(sizeof(Joint)*skin.joints_count);
					model.jointCount = skin.joints_count;

					assert(model.jointCount == model.inverseBindMatrixCount);

					for(int i = 0; i < skin.joints_count; ++i) {
						cgltf_node *joint = skin.joints[i];

						if(joint) {
							model.joints[i].childIndexes = initResizeArray(int);
							//NOTE: Find the parent index
							for(int k = 0; k < joint->children_count; ++k) {
								cgltf_node *child = joint->children[k];
								for(int j = 0; j < skin.joints_count; ++j) {
									cgltf_node *jointChild = skin.joints[j];

									if(child == jointChild) {
										pushArrayItem(&model.joints[i].childIndexes, k, int);
										break;
									}
								}
							}
						}
					}
				}
				
				for(int i = 0; i < data->meshes[0].primitives[0].attributes_count; ++i) {
					
					cgltf_attribute attrib = data->meshes[0].primitives[0].attributes[i];
					cgltf_accessor *accessor = attrib.data;
					cgltf_buffer_view *buffer_view = accessor->buffer_view;
					cgltf_buffer* buffer = buffer_view->buffer;
					uint8_t *ptr = (uint8_t *)buffer->data;

					void *bufferToCopyTo = 0;
					int floatCount = 3;

					bool isShort = false;

					
					
					if(attrib.type == cgltf_attribute_type_position) {
						//NOTE: Make sure it has floats
						assert(accessor->component_type == cgltf_component_type_r_32f);
						assert(accessor->type == cgltf_type_vec3);
						bufferToCopyTo = poss = (float *)malloc(buffer_view->size);
						vertexCount = accessor->count;
					}
					if(attrib.type == cgltf_attribute_type_normal) {
						assert(accessor->component_type == cgltf_component_type_r_32f);
						assert(accessor->type == cgltf_type_vec3);
						bufferToCopyTo = normals = (float *)malloc(buffer_view->size);
						normalCount = accessor->count;
					}
					if(attrib.type == cgltf_attribute_type_joints) {
						assert(accessor->component_type == cgltf_component_type_r_16u);
						assert(accessor->type == cgltf_type_vec4);
						bufferToCopyTo = joints = (u16 *)malloc(buffer_view->size);
						jointCount = accessor->count;
						floatCount = 4;
						isShort = true;
					}

					if(attrib.type == cgltf_attribute_type_weights) {
						assert(accessor->component_type == cgltf_component_type_r_32f);
						assert(accessor->type == cgltf_type_vec4);
						bufferToCopyTo = weights = (float *)malloc(buffer_view->size);
						weightsCount = accessor->count;
						floatCount = 4;
					}

					if(attrib.type == cgltf_attribute_type_texcoord) {
						assert(accessor->component_type == cgltf_component_type_r_32f);
						assert(accessor->type == cgltf_type_vec2);
						bufferToCopyTo = uvs = (float *)malloc(buffer_view->size);
						uvCount = accessor->count;
						floatCount = 2;
					}
					
					//NOTE: Is an actual attribute we want to retrieve
					if(bufferToCopyTo) {
						size_t sizeOfElm = (isShort) ? sizeof(u16) : sizeof(float);
						copyBufferToMemory(bufferToCopyTo, ptr, buffer_view, accessor, isShort, floatCount);
						
						//NOTE: This is if an attribute is sparse 
						if(accessor->is_sparse) {
							
							cgltf_accessor_sparse sparse = accessor->sparse;

							if(sparse.count > 0) {

								//NOTE: Get the indicies out
								cgltf_buffer_view* indices_buffer_view = sparse.indices_buffer_view;
								assert(indices_buffer_view);
								
								cgltf_buffer* indicies_buffer = indices_buffer_view->buffer;
								uint8_t *indicies_ptr = (uint8_t *)((uint8_t *)indicies_buffer->data) + indices_buffer_view->offset;
								assert(cgltf_component_type_r_16u == sparse.indices_component_type);

								//NOTE: Now get the data out 
								cgltf_buffer_view* values_buffer_view = sparse.values_buffer_view;
								
								cgltf_buffer* values_buffer = values_buffer_view->buffer;
								uint8_t *values_ptr = (uint8_t *)((uint8_t *)values_buffer->data) + values_buffer_view->offset;

								size_t indices_stride = sizeof(unsigned short);
								size_t value_stride = sizeOfElm*floatCount;
								
								for(int i = 0; i < sparse.count; ++i) {
									int index = (int)(*(indicies_ptr + i*indices_stride));

									void *to = ((uint8_t *)bufferToCopyTo) + sizeOfElm*floatCount*index;
									void *from = values_ptr + i*value_stride;

									memcpy(to, from, floatCount*sizeOfElm);
								}
							}
						}
					}
				}

				assert(jointCount == vertexCount);
				assert(weightsCount == vertexCount);
				assert(weightsCount == vertexCount);
				assert(uvCount == vertexCount);

				vertexes = (VertexWithJoints *)malloc(sizeof(VertexWithJoints)*vertexCount);

				if(vertexes) {
					//NOTE: Create the vertex data now
					for(int i = 0; i < vertexCount; ++i) {
						VertexWithJoints *v = &vertexes[i];

						v->pos = make_float3((float)poss[i*3 + 0], (float)poss[i*3 + 1], (float)poss[i*3 + 2]);

						if(normals) {
							v->normal = make_float3((float)normals[i*3 + 0], (float)normals[i*3 + 1], (float)normals[i*3 + 2]);
						} 
						if(uvs) {
							v->texUV = make_float2((float)uvs[i*2 + 0], (float)uvs[i*2 + 1]);
						}

						if(joints) {
							v->jointIndexes = make_float4((float)joints[i*4 + 0], (float)joints[i*4 + 1], (float)joints[i*4 + 2], (float)joints[i*4 + 3]);
							assert((int)v->jointIndexes.x < jointCount);
							assert((int)v->jointIndexes.y < jointCount);
							assert((int)v->jointIndexes.z < jointCount);
							assert((int)v->jointIndexes.w < jointCount);
						}

						if(weights) {
							v->jointWeights = make_float4((int)weights[i*4 + 0], (int)weights[i*4 + 1], (int)weights[i*4 + 2], (int)weights[i*4 + 3]);
							float totalJointWeight = (v->jointWeights.x + v->jointWeights.y + v->jointWeights.z + v->jointWeights.w);

							//NOTE: Make sure weights add up to zero if the vertex is weighted
							assert(totalJointWeight == 1.0f || totalJointWeight == 0.0f);
						}
					}
				}

				if(indicies) {
					//NOTE: Upscale the indices buffer from a unsigned SHORT to a unsigned INT
					indiciesToSend = (unsigned int *)malloc(sizeof(unsigned int)*indexCount);

					//NOTE: Create the vertex data now
					int reverseWindingIndex = indexCount - 1;
					//NOTE: The winding is reverse of our renderer - gltf uses a CCW winding order for the front face.
					for(int i = 0; i < indexCount; ++i) {
						unsigned short v = indicies[reverseWindingIndex];
						indiciesToSend[i] = (unsigned int)v;
						reverseWindingIndex--;
					}
				}

				free(poss);
				free(uvs);
				free(normals);
				free(indicies);
				free(joints);
				free(weights);


            }
        } 
                
        cgltf_free(data);
    } else {
		printf("didn't Parsed files successfully\n");
	}

    if(result == cgltf_result_success) {
        model.valid = true;
    } 

	assert(model.valid);	

	if(!indiciesToSend) {
		indexCount = vertexCount;
		indiciesToSend = (unsigned int *)malloc(sizeof(unsigned int)*indexCount);

		for(int i = 0; i < vertexCount; ++i) {
			indiciesToSend[i] = i;
		}
	}

	if(vertexes && indiciesToSend) {
		model.modelBuffer = generateVertexBuffer(vertexes, vertexCount, indiciesToSend, indexCount, ATTRIB_INSTANCE_TYPE_MODEL_MATRIX_SKELETAL);
	}

	free(vertexes);
	free(indiciesToSend);

	return model;
}

