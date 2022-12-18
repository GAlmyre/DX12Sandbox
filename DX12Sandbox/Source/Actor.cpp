#include "Actor.h"
#include "pch.h"

using namespace DirectX;

Actor::Actor()
{
	XMStoreFloat4x4(&WorldMatrix, XMMatrixIdentity());
}

void Actor::RecomputeMatrices()
{
	// Compute Rotation
	XMMATRIX RotationXMatrix = XMMatrixRotationX(XMConvertToRadians(Rotation.x));
	XMMATRIX RotationYMatrix = XMMatrixRotationY(XMConvertToRadians(Rotation.y));
	XMMATRIX RotationZMatrix = XMMatrixRotationZ(XMConvertToRadians(Rotation.z));
	XMMATRIX RotationMatrix = RotationXMatrix * RotationYMatrix * RotationZMatrix;

	// Compute Translation
	XMFLOAT4 PositionF4 = XMFLOAT4(Position.x, Position.y, Position.z, 1.0f);
	XMMATRIX TranslationMatrix = XMMatrixTranslationFromVector(XMLoadFloat4(&PositionF4));
	XMMATRIX TmpWorldMatrix = RotationMatrix * TranslationMatrix;
	XMStoreFloat4x4(&WorldMatrix, TmpWorldMatrix);
}
