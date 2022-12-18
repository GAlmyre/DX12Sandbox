#pragma once
#include "Actor.h"

class Camera :
    public Actor
{
public:

	Camera();

	void MoveForward(float InputValue);
	void MoveRight(float InputValue);
	void MoveUp(float InputValue);

	DirectX::XMFLOAT4X4 ProjectionMatrix; // this will store our projection matrix
	DirectX::XMFLOAT4X4 ViewMatrix; // this will store our view matrix

	DirectX::XMFLOAT4 CameraPosition; // this is our cameras position vector
	DirectX::XMFLOAT4 CameraTarget; // a vector describing the point in space our camera is looking at
	DirectX::XMFLOAT4 CameraUp; // the worlds up vector

	float AspectRatio = 1280.0f/720.0f;

	float FOV = 45.0f;

	float Near = 0.01f;

	float Far = 1000.0f;

	float CameraSpeed = 0.05f;

	void RecomputeMatrices() override;
};

