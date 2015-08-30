#pragma once

#include <string>
#include <czmq.h>

#include "zmqx/zhelper.h"
#include "ris/snapshot/snapshotable.h"
#include "ris/regionpub.pb.h"
#include "ris/snapshot.pb.h"

typedef std::string			uuid_t;
typedef uint64_t			ri_time_t;		// msec

#define ri_time_now			time_now

class Region {
public:
	uuid_t					id;
	uint32_t				version;
	std::string				idc;
	std::string				bus_address;
	std::string				snapshot_address;
	ri_time_t				timeval;

	inline Region() :
		version(0),timeval(0)
	{
	}

	Region(const Region& ref);
	Region& operator = (const Region& ref);

	bool operator == (const uuid_t& _id);
	bool operator == (const Region& ref);
public:
	std::shared_ptr<region::pub::Region> toPublish() const ;
	void toPublishBase(region::pub::RegionBase* region) const;
	static std::shared_ptr<region::pub::RmRegion> toPublishRm(const uuid_t& uuid);

	std::shared_ptr<snapshot::RegionBegin> toSnapshotBegin() const;
	std::shared_ptr<snapshot::RegionEnd> toSnapshotEnd() const;
};


class Service {
public:
	std::string				name;
	std::string				address;
	ri_time_t				timeval;

	inline Service() :
		timeval(0)
	{
	}
	inline Service(const std::string& n,const std::string& a,ri_time_t tv=0) :
		name(n),address(a),timeval(tv)
	{
	}

	Service(const Service& ref);
	Service& operator = (const Service& ref);
public:
	std::shared_ptr<region::pub::Service> toPublish(const uuid_t& region,uint32_t version) const;
	static std::shared_ptr<region::pub::RmService> toPublishRm(const uuid_t& region,uint32_t version,const std::string& name);

	std::shared_ptr<snapshot::Service> toSnapshot() const;
};

class Payload {
public:
	uuid_t					id;
	ri_time_t				timeval;

	inline Payload() :
		timeval(0)
	{
	}
	inline Payload(const uuid_t& _id,ri_time_t tv=0) :
		id(_id),timeval(tv)
	{
	}

	Payload(const Payload& ref);
	Payload& operator = (const Payload& ref);
	bool operator == (const uuid_t& _id);
	bool operator == (const Payload& ref);
public:
	std::shared_ptr<region::pub::Payload> toPublish(const uuid_t& region,uint32_t version) const;
	static std::shared_ptr<region::pub::RmPayload> toPublishRm(const uuid_t& region,uint32_t version,const uuid_t& id);


	std::shared_ptr<snapshot::Payload> toSnapshot() const;
};


namespace std {
	template<>
	struct hash<Region> {
		std::size_t operator()(const Region& region) const {
			return std::hash<std::string>()(region.id);
		}
	};

	template<>
	struct hash<Payload> {
		std::size_t operator()(const Payload& payload) const {
			return std::hash<std::string>()(payload.id);
		}
	};
}

