#pragma once

#include <cstdint>
#include <random>

namespace Pillar {

	struct UUIDComponent
	{
		uint64_t UUID;

		UUIDComponent()
			: UUID(GenerateUUID()) {}
		UUIDComponent(uint64_t uuid)
			: UUID(uuid) {}
		UUIDComponent(const UUIDComponent&) = default;

		operator uint64_t() const { return UUID; }

	private:
		static uint64_t GenerateUUID()
		{
			static std::random_device rd;
			static std::mt19937_64 gen(rd());
			static std::uniform_int_distribution<uint64_t> dis;
			return dis(gen);
		}
	};

} // namespace Pillar
