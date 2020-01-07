#pragma once

#include <deque>
#include <atomic>
#include <rnet/RNetTime.h>
#include <algorithm>

namespace rnet
{	
	template<TimeMS MaxAge = 1000, TimeMS MinAge = 500>
	class MetricsCounters
	{
	public:
		struct Sample
		{
			TimeMS time;
			uint64_t value;
		};

		void Add(TimeMS now, uint64_t count)
		{
			samples.push_back(Sample{ now, count });
			total += count;
			totalInPeriod += count;
			Update(now);
		}

		void Update(TimeMS now)
		{
			while (TimeSince(now, samples.front().time) > MaxAge)
			{
				totalInPeriod -= samples.front().value;
				samples.pop_front();
				if (samples.empty())
				{
					return;
				}
			}
		}

		uint64_t Total() const { return total; }

		double CalcPerSecond(TimeMS now) 
		{ 
			if (!samples.empty())
			{
				Update(now);
			}
			if (samples.empty())
			{
				return 0;
			}
			
			auto interval = TimeSince(samples.back().time, samples.front().time);
			if (interval < MinAge)
			{
				interval = MinAge;
			}

			return static_cast<double>(totalInPeriod) / (static_cast<double>(interval)/1000);
		}

	private:
		uint64_t total = 0;
		uint64_t totalInPeriod = 0;
		std::deque<Sample> samples;
	};

	enum class PacketType : uint8_t 
	{
		Raw,
		UserReliable,
		UserUnreliable,
		Count
	};

	enum class DataType : uint8_t
	{
		Packets,
		Bytes,
		Count
	};

	enum class DirectionType : uint8_t
	{
		Sent,
		Received,
		Count
	};

	struct Stats
	{
		Stats()
		{
			for (size_t i = 0; i < static_cast<size_t>(PacketType::Count); ++i)
			{
				for (size_t j = 0; j < static_cast<size_t>(DataType::Count); ++j)
				{
					for (size_t k = 0; k < static_cast<size_t>(DirectionType::Count); ++k)
					{
						unitsPerSecond[i][j][k] = 0;
						unitsTotal[i][j][k] = 0;
					}
				}
			}
		}

		double PerSecond(PacketType packetType, DataType dataType, DirectionType directionType) const
		{
			return unitsPerSecond[static_cast<size_t>(packetType)]
				[static_cast<size_t>(dataType)][static_cast<size_t>(directionType)];
		}

		double RawBytesPerSecondSent() const
		{
			return PerSecond(rnet::PacketType::Raw, rnet::DataType::Bytes, rnet::DirectionType::Sent);
		}

		double RawBytesPerSecondReceived() const
		{
			return PerSecond(rnet::PacketType::Raw, rnet::DataType::Bytes, rnet::DirectionType::Received);
		}

		double RawBytesSent() const
		{
			return Total(rnet::PacketType::Raw, rnet::DataType::Bytes, rnet::DirectionType::Sent);
		}

		double UserBytesSent() const
		{
			return Total(rnet::PacketType::UserReliable, rnet::DataType::Bytes, rnet::DirectionType::Sent) +
				Total(rnet::PacketType::UserUnreliable, rnet::DataType::Bytes, rnet::DirectionType::Sent);
		}	

		double RawBytesReceived() const
		{
			return Total(rnet::PacketType::Raw, rnet::DataType::Bytes, rnet::DirectionType::Received);
		}

		double PacketLossPerSecond() const { return 0.0; }

		uint64_t Total(PacketType packetType, DataType dataType, DirectionType directionType) const
		{
			return unitsTotal[static_cast<size_t>(packetType)]
				[static_cast<size_t>(dataType)][static_cast<size_t>(directionType)];
		}

		double unitsPerSecond[static_cast<size_t>(PacketType::Count)]
			[static_cast<size_t>(DataType::Count)][static_cast<size_t>(DirectionType::Count)];
		uint64_t unitsTotal[static_cast<size_t>(PacketType::Count)]
			[static_cast<size_t>(DataType::Count)][static_cast<size_t>(DirectionType::Count)];

		Stats& operator+=(const Stats& other)
		{
			for (size_t i = 0; i < static_cast<size_t>(PacketType::Count); ++i)
			{
				for (size_t j = 0; j < static_cast<size_t>(DataType::Count); ++j)
				{
					for (size_t k = 0; k < static_cast<size_t>(DirectionType::Count); ++k)
					{
						unitsPerSecond[i][j][k] += other.unitsPerSecond[i][j][k];
						unitsTotal[i][j][k] += other.unitsTotal[i][j][k];
					}
				}
			}
			return *this;
		}
	};

	struct DataMetrics
	{
		DataMetrics()
			: refreshSnapshot(false)
		{}

		void OnReceived(TimeMS now, PacketType packetType, size_t byteCount, size_t packetCount = 1)
		{
			Add(now, DirectionType::Received, packetType, byteCount, packetCount);
		}

		void OnSent(TimeMS now, PacketType packetType, size_t byteCount, size_t packetCount = 1)
		{
			Add(now, DirectionType::Sent, packetType, byteCount, packetCount);
		}

		void Snapshot(Stats& stats)
		{
			stats = lastStats;
			refreshSnapshot = true;
		}

		void Update(TimeMS now)
		{
			if (refreshSnapshot)
			{
				refreshSnapshot = false;
				for (size_t i = 0; i < static_cast<size_t>(PacketType::Count); ++i)
				{
					for (size_t j = 0; j < static_cast<size_t>(DataType::Count); ++j)
					{
						for (size_t k = 0; k < static_cast<size_t>(DirectionType::Count); ++k)
						{
							lastStats.unitsPerSecond[i][j][k] = counters[i][j][k].CalcPerSecond(now);
							lastStats.unitsTotal[i][j][k] = counters[i][j][k].Total();
						}
					}
				}
			}
		}
	private:
		void Add(TimeMS now, DirectionType directionType, PacketType packetType, size_t byteCount, size_t packetCount = 1)
		{
			counters[static_cast<size_t>(packetType)][
				static_cast<size_t>(DataType::Bytes)][
				static_cast<size_t>(directionType)].Add(now, byteCount);
			counters[static_cast<size_t>(packetType)][
				static_cast<size_t>(DataType::Packets)][
				static_cast<size_t>(DirectionType::Received)].Add(now, packetCount);
		}

		MetricsCounters<> counters[static_cast<size_t>(PacketType::Count)]
			[static_cast<size_t>(DataType::Count)][static_cast<size_t>(DirectionType::Count)];
		Stats lastStats;
		std::atomic<bool> refreshSnapshot;
	};
}