#include <ScenegraphNode.h>

class CarNode : public ScenegraphNode {


public:
    CarNode(int meshId, int textureId,
        const float* position = nullptr,
        float angle = 0.0f,
        const float* axis = nullptr,
        const float* scale = nullptr);

    CarNode(int meshId, int textureId,
        const float(&transformMatrix)[16]);
    void draw();
    void update(float deltaTime);

    float steering = 0;
    bool isAccelerating = false;
	bool isDecelerating = false;

private:
	int meshID = 0;
	int texMode = 1;

    /* Variables for driving */
	
	float speed = 0.0f; // Current speed of the car

	float maxSpeed = 60.0f; 
	float acceleration = 15.0f; 
	float brakeDeceleration = 60.0f;
	float deceleration = 5.0f;

	float maxTurnRate = 200.0f; 

    void addChildren();
};