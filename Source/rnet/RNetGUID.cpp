#include <rnet/RNetGUID.h>
#include <string>

#if defined(_MSC_VER) && _MSC_VER > 0
#define PRINTF_64_BIT_MODIFIER "I64"
#else
#define PRINTF_64_BIT_MODIFIER "ll"
#endif

namespace rnet
{
	static constexpr size_t StringSize = 64;

	const char* GUID::ToString(void) const
	{
		static unsigned char strIndex = 0;
		static char str[8][StringSize];

		unsigned char lastStrIndex = strIndex;
		strIndex++;
		ToString(str[lastStrIndex & 7]);
		return (char*)str[lastStrIndex & 7];
	}

	// Return the GUID as a string
	// dest must be large enough to hold the output
	// THREADSAFE
	void GUID::ToString(char *dest) const
	{
		if (g == UNASSIGNED_RAKNET_GUID)
		{
			char tmp[] = "UNASSIGNED_RAKNET_GUID";
			memcpy(dest, tmp, 23);
		}
		else
		{
#if defined(_MSC_VER) && _MSC_VER > 0
			sprintf_s(dest, StringSize, "%" PRINTF_64_BIT_MODIFIER "u", (long long unsigned int) g);
#else
			sprintf(dest, /*StringSize,*/ "%" PRINTF_64_BIT_MODIFIER "u", (long long unsigned int) g);
#endif
		}
	}

	bool GUID::FromString(const char *source)
	{
		if (source == 0)
			return false;
#if   defined(WIN32)
		g = _strtoui64(source, NULL, 10);
#else
		// Changed from g=strtoull(source,0,10); for android
		g = strtoull(source, (char **)NULL, 10);
#endif
		return true;

	}
}