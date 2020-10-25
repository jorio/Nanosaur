#pragma once

#include <vector>

namespace Pomme
{

	template<typename TObj, typename TId, int MAX>
	class GrowablePool
	{
		std::vector<TObj> pool;
		std::vector<bool> isInUse;
		std::vector<TId> freeIDs;
		TId inUse, inUsePeak;

	public:
		GrowablePool()
		{
			inUse = 0;
			inUsePeak = 0;
		}

		TId Alloc()
		{
			if (IsFull()) throw std::length_error("too many items allocated");

			inUse++;
			if (inUse > inUsePeak)
			{
				inUsePeak = inUse;
			}

			if (!freeIDs.empty())
			{
				auto id = freeIDs.back();
				freeIDs.pop_back();
				isInUse[id] = true;
				return id;
			}
			else
			{
				pool.emplace_back();
				isInUse.push_back(true);
				return TId(pool.size() - 1);
			}
		}

		void Dispose(TId id)
		{
			if (!IsAllocated(id)) throw std::invalid_argument("id isn't allocated");
			inUse--;
			freeIDs.push_back(id);
			isInUse[id] = false;
			Compact();
		}

		void Compact()
		{
			while (!freeIDs.empty() && freeIDs.back() == (TId) pool.size() - 1)
			{
				freeIDs.pop_back();
				pool.pop_back();
				isInUse.pop_back();
			}
		}

		TObj& operator[](TId id)
		{
			if (!IsAllocated(id)) throw std::invalid_argument("id isn't allocated");
			return pool[id];
		}

		const TObj& operator[](TId id) const
		{
			if (!IsAllocated(id)) throw std::invalid_argument("id isn't allocated");
			return pool[id];
		}

		bool IsFull() const
		{
			return inUse >= MAX;
		}

		bool IsAllocated(TId id) const
		{
			return id >= 0 && (unsigned) id < pool.size() && isInUse[id];
		}
	};

}