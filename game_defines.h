#define GRAVITY_POWER 28
#define JUMP_POWER 6
#define MAX_SHAKE_TIMER 0.4f
#define EPSILON_VALUE 0.2f
#define WALK_SPEED 2
#define CIRCLE_RADIUS_MAX 10
#define SHOW_CIRCLE_DELAY 2
#define STAMINA_DRAIN_SPEED 0.1f
#define STAMINA_RECHARGE_SPEED 0.1f
#define CLOUD_EVELVATION 100 //NOTE: This is 192 in actual minecraft - Clouds always float westward between layer 192 and 196
#define WATER_ELEVATION 40 //NOTE: This is 62 in actual minecraft
#define BLOCK_SIZE 1 
#define AO_BIT_NOT_VISIBLE 61
#define AO_BIT_CREATING 62
#define AO_BIT_INVALID 63
#define SUB_SOIL_DEPTH 5
#define DISTANCE_CAN_PLACE_BLOCK 6 //NOTE: Minecraft has a reach distance of 5 in survival and 6 in creative
#define CAMERA_OFFSET make_float3(0, 0.7f, 0)
#define ITEM_PER_SLOT_COUNT 64
#define ITEM_HOT_SPOTS 8


//NOTE: DEBUGGING CONSTANTS
#define PERLIN_SIZE 128
#define MAX_BONES_PER_MODEL 1000