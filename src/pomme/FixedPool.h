#pragma once

namespace Pomme {

template<typename TObj, typename TId, int MAX>
class FixedPool {
	// fixed size array so that pointers to the elements don't move around in memory
	// (unlike how a vector might move around elements on resize)
	TObj pool[MAX];
	std::vector<TId> freeIDs;
	int inUse, inUsePeak;

public:
	FixedPool() {
		inUse = 0;
		inUsePeak = 0;
		freeIDs.reserve(MAX);
		for (int i = MAX - 1; i >= 0; i--)
			freeIDs.push_back(i);
	}

	TObj* Alloc() {
		if (freeIDs.empty())
			throw std::length_error("pool exhausted");
		TId id = freeIDs.back();
		freeIDs.pop_back();
		inUse++;
		if (inUse > inUsePeak)
			inUsePeak = inUse;
		return &pool[id];
	}

	void Dispose(TObj* obj) {
		long id = obj - &pool[0];
		if (id < 0 || id >= MAX)
			throw std::invalid_argument("obj isn't stored in pool");
		inUse--;
		freeIDs.push_back(id);
	}
};

}
