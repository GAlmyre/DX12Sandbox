#pragma once

#include <DirectXMath.h>
#include "pch.h"

using namespace DirectX;

class Actor
{
public:
	Actor();

	XMFLOAT3 GetPosition() const
	{
		return Position;
	}
	XMFLOAT3 GetRotation() const
	{
		return Rotation;
	}
	XMFLOAT3 GetScale() const
	{
		return Scale;
	}
	XMFLOAT4X4 GetWorldMatrix()
	{
		return WorldMatrix;
	}
	XMFLOAT3 GetForwardVector() const
	{
		XMFLOAT3 DefaultForward = XMFLOAT3(0.0f, 0.0f, 1.0f);
		XMVECTOR ForwardVector = XMVector3Transform(XMLoadFloat3(&DefaultForward), XMMatrixRotationRollPitchYaw(Rotation.x, Rotation.y, Rotation.z));
		XMFLOAT3 Result;
		XMStoreFloat3(&Result, ForwardVector);

		return Result;

	}
	XMFLOAT3 GetRightVector() const
	{
		XMFLOAT3 DefaultRight = XMFLOAT3(1.0f, 0.0f, 0.0f);
		XMVECTOR RightVector = XMVector3Transform(XMLoadFloat3(&DefaultRight), XMMatrixRotationRollPitchYaw(Rotation.x, Rotation.y, Rotation.z));
		XMFLOAT3 Result;
		XMStoreFloat3(&Result, RightVector);

		return Result;
	}
	XMFLOAT3 GetUpVector() const
	{
		XMFLOAT3 DefaultUp = XMFLOAT3(0.0f, 1.0f, 0.0f);
		XMVECTOR UpVector = XMVector3Transform(XMLoadFloat3(&DefaultUp), XMMatrixRotationRollPitchYaw(Rotation.x, Rotation.y, Rotation.z));
		XMFLOAT3 Result;
		XMStoreFloat3(&Result, UpVector);

		return Result;
	}


	void SetPosition(const XMFLOAT3& InPosition)
	{
		Position = InPosition;
		RecomputeMatrices();
	}
	void SetRotation(const XMFLOAT3& InRotation)
	{
		Rotation = InRotation;
		RecomputeMatrices();
	}
	void SetScale(const XMFLOAT3& InScale)
	{
		Scale = InScale;
		RecomputeMatrices();
	}

protected:

	virtual void RecomputeMatrices();

	// The world Matrix holding the transform of the Actor
	XMFLOAT4X4 WorldMatrix = XMFLOAT4X4();

	// Actor's Transform
	XMFLOAT3 Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMFLOAT3 Rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMFLOAT3 Scale = XMFLOAT3(1.0f, 1.0f, 1.0f);
};