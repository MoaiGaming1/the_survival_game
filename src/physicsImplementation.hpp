#pragma once

#include "Jolt/Jolt.h"
#include "Jolt/RegisterTypes.h"
#include "Jolt/Core/Factory.h"
#include "Jolt/Physics/Collision/ObjectLayer.h"
#include "Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include "Jolt/Core/TempAllocator.h"
#include "Jolt/Core/JobSystemThreadPool.h"
#include "Jolt/Physics/Collision/Shape/BoxShape.h"
#include "Jolt/Physics/Body/BodyCreationSettings.h"
#include "Jolt/Physics/Collision/Shape/HeightFieldShape.h"

#include <iostream>

#include "constants.hpp"


namespace physics {
	namespace Layers {
		static constexpr JPH::ObjectLayer NON_MOVING = 0;
		static constexpr JPH::ObjectLayer MOVING = 1;
		static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
	}

	namespace BroadPhaseLayers {
		static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
		static constexpr JPH::BroadPhaseLayer MOVING(1);
		static constexpr JPH::uint NUM_LAYERS = 2;
	}

	class ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter {
	public:
		bool ShouldCollide(JPH::ObjectLayer object1, JPH::ObjectLayer object2) const override {
			switch (object1) {
				case Layers::NON_MOVING: return object2 == Layers::MOVING;
				case Layers::MOVING: return true;
				default: return false;
			}
		}
	};

	class BPLayerInterfaceImpl : public JPH::BroadPhaseLayerInterface {
	public:
		BPLayerInterfaceImpl() {
			mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
			mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
		}

		JPH::uint GetNumBroadPhaseLayers() const override { return BroadPhaseLayers::NUM_LAYERS; }

		JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override {
			return mObjectToBroadPhase[layer];
		}

	private:
		JPH::BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
	};

	class ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter {
	public:
		bool ShouldCollide(JPH::ObjectLayer layer1, JPH::BroadPhaseLayer layer2) const override {
			switch (layer1) {
				case Layers::NON_MOVING: return layer2 == BroadPhaseLayers::MOVING;
				case Layers::MOVING: return true;
				default: return false;
			}
		}
	};

	JPH::PhysicsSystem physicsSystem;
	JPH::TempAllocatorImpl* tempAllocator = nullptr;
	JPH::JobSystemThreadPool* jobSystem = nullptr;

	BPLayerInterfaceImpl broadPhaseLayerInterface;
	ObjectVsBroadPhaseLayerFilterImpl objVsBpFilter;
	ObjectLayerPairFilterImpl objVsObjFilter;

	void init() {
		JPH::RegisterDefaultAllocator();
		JPH::Factory::sInstance = new JPH::Factory();
		JPH::RegisterTypes();

		tempAllocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024);
		jobSystem = new JPH::JobSystemThreadPool(
			JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers,
			std::thread::hardware_concurrency() - 1
		);

		const JPH::uint maxBodies = 1024;
		const JPH::uint numBodyMutexes = 0;
		const JPH::uint maxBodyPairs = 1024;
		const JPH::uint maxContactConstraints = 1024;

		physicsSystem.Init(maxBodies, numBodyMutexes, maxBodyPairs, maxContactConstraints, broadPhaseLayerInterface, objVsBpFilter, objVsObjFilter);

		std::cout << "physics init\n";
	}

	uint stepCount = 0;

	JPH::Vec3 joltVec3(glm::vec3 v) {
		return JPH::Vec3(v.x, v.y, v.z);
	}

	glm::quat glmQuat(glm::vec3 r) {
		return glm::quat(glm::radians(r));
	}

	JPH::Quat joltQuat(glm::quat q) {
		return JPH::Quat(q.x, q.y, q.z, q.w);
	}

	JPH::Quat joltQuat(glm::vec3 r) {
		glm::quat q = glmQuat(r);
		return JPH::Quat(q.x, q.y, q.z, q.w);
	}

	void update(float totalTime) {
		uint expectedStepCount = totalTime * PHYSICS_RATE;
		if (expectedStepCount > stepCount) {
			uint steps = std::min(expectedStepCount - stepCount, (uint)PHYSICS_MAX_STEP_COUNT);
			for (uint i = 0; i < steps; i++) {
				physicsSystem.Update(1.0f / ((float)PHYSICS_RATE), 1, tempAllocator, jobSystem);
			}
			stepCount = expectedStepCount;
		}
	}

	void shutdown() {
		delete jobSystem;
		delete tempAllocator;
		JPH::UnregisterTypes();
		delete JPH::Factory::sInstance;
		JPH::Factory::sInstance = nullptr;
	}
}
