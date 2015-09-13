#pragma once

#include <list>
#include "ris/ritypes.h"


class IRegionLRUReader {
public:
	virtual ~IRegionLRUReader() { }

	virtual const Region& getRegion() const = 0;

	virtual const std::list<Service>& getServicesOrderByLRU() const = 0;

	virtual const std::list<Payload>& getPayloadsOrderByLRU() const = 0;
};

