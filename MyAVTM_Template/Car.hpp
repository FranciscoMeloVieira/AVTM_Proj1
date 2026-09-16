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

	float maxSpeed = 6.0f; 
	float acceleration = 0.30f; 
	float brakeDeceleration = 0.50f;
	float deceleration = 0.20f;

	float maxTurnRate = 20.0f; 

    void addChildren();
};