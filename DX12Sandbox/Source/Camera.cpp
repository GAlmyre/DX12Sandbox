#include "pch.h"
#include "Camera.h"

Camera::Camera()
{
	Position = DirectX::XMFLOAT3(0.0f, 0.0f, -4.0f);
	CameraTarget = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	CameraUp = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f);

	// Perspective Camera
	DirectX::XMMATRIX PerspectiveMatrix = DirectX::XMMatrixPerspectiveFovLH(FOV * (3.14f / 180.0f), AspectRatio, Near, Far);
	XMStoreFloat4x4(&ProjectionMatrix, PerspectiveMatrix);
	RecomputeMatrices();
}

void Camera::RecomputeMatrices()
{
	Actor::RecomputeMatrices();
	XMFLOAT4 PositionF4 = XMFLOAT4(Position.x, Position.y, Position.z, 0.0f);
	DirectX::XMVECTOR CamPos = XMLoadFloat4(&PositionF4);
	DirectX::XMVECTOR CamUp = XMLoadFloat4(&CameraUp);
	XMFLOAT3 ComputedForward = GetForwardVector();
	XMFLOAT4 CameraForward = XMFLOAT4(ComputedForward.x, ComputedForward.y, ComputedForward.z, 0.0f);
	DirectX::XMVECTOR CamForward = XMLoadFloat4(&CameraForward);

	DirectX::XMMATRIX LookAtMatrix = DirectX::XMMatrixLookToLH(CamPos, CamForward, CamUp);
	XMStoreFloat4x4(&ViewMatrix, LookAtMatrix);
}

void Camera::MoveForward(float InputValue)
{
	XMFLOAT3 ForwardVector = GetForwardVector();
	XMVECTOR NewPos = XMLoadFloat3(&Position) + XMLoadFloat3(&ForwardVector) * InputValue * CameraSpeed;
	XMFLOAT3 UpdatedPosition;
	XMStoreFloat3(&UpdatedPosition, NewPos);
	SetPosition(UpdatedPosition);
}
void Camera::MoveRight(float InputValue)
{
	XMFLOAT3 RightVector = GetRightVector();
	XMVECTOR NewPos = XMLoadFloat3(&Position) + XMLoadFloat3(&RightVector) * InputValue * CameraSpeed;
	XMFLOAT3 UpdatedPosition;
	XMStoreFloat3(&UpdatedPosition, NewPos);
	SetPosition(UpdatedPosition);
}

void Camera::MoveUp(float InputValue)
{
	XMFLOAT3 UpVector = GetUpVector();
	XMVECTOR NewPos = XMLoadFloat3(&Position) + XMLoadFloat3(&UpVector) * InputValue * CameraSpeed;
	XMFLOAT3 UpdatedPosition;
	XMStoreFloat3(&UpdatedPosition, NewPos);
	SetPosition(UpdatedPosition);
}