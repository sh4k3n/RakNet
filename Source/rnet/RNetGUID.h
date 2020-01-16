#pragma once

#include <stdint.h>

namespace rnet
{
	using SystemIndex = uint16_t;

	/// Peer global unique identifier identifier.
	struct GUID
	{
		static constexpr SystemIndex InvalidSystemIndex = ~static_cast<SystemIndex>(0);

		static constexpr uint64_t UNASSIGNED_RAKNET_GUID = ~static_cast<uint64_t>(0);

		GUID() : g(UNASSIGNED_RAKNET_GUID), systemIndex(InvalidSystemIndex) {}

		GUID(const GUID& other) : g(other.g), systemIndex(other.systemIndex) {}

		explicit GUID(uint64_t _g) : g(_g), systemIndex(InvalidSystemIndex) {}

		GUID& operator=(const GUID& other)
		{
			g = other.g;
			systemIndex = other.systemIndex;
			return *this;
		}

		constexpr bool operator==(const GUID& other) const { return g == other.g; }
		constexpr bool operator!=(const GUID& other) const { return g != other.g; }
		constexpr bool operator>(const GUID& other) const { return g > other.g; }
		constexpr bool operator<(const GUID& other) const { return g < other.g; }

		// TODO: Get rid of this
		const char* ToString(void) const;

		void ToString(char *dest) const;

		bool FromString(const char *source);

		static unsigned long ToUint32(const GUID &g)
		{
			return ((unsigned long)(g.g >> 32)) ^ ((unsigned long)(g.g & 0xFFFFFFFF));
		}

		static int size() { return (int) sizeof(uint64_t); }

		uint64_t g;
		SystemIndex systemIndex; // Internal index for fast lookup
	};

	using UserGUID = rnet::GUID;
}
