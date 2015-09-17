#pragma once

#include <string>
#include <czmq.h>

#include "zmqx/zhelper.h"
#include "ris/pub.pb.h"
#include "ris/snapshot.pb.h"
#include "snapshot/snapshotable.h"

typedef std::string			ri_uuid_t;
typedef uint64_t			ri_time_t;		// msec

#define ri_time_now			time_now

class Region {
public:
	ri_uuid_t					id;
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

	bool operator == (const ri_uuid_t& _id) const;
	bool operator == (const Region& ref) const;
public:
	std::shared_ptr<pub::Region> toPublish() const ;
	static std::shared_ptr<pub::RmRegion> toPublishRm(const ri_uuid_t& uuid);

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
	std::shared_ptr<pub::Service> toPublish(uint32_t version) const;
	static std::shared_ptr<pub::RmService> toPublishRm(uint32_t version,const std::string& name);

	std::shared_ptr<snapshot::Service> toSnapshot() const;
};

class Payload {
public:
	ri_uuid_t					id;
	ri_time_t				timeval;

	inline Payload() :
		timeval(0)
	{
	}
	inline Payload(const ri_uuid_t& _id,ri_time_t tv=0) :
		id(_id),timeval(tv)
	{
	}

	Payload(const Payload& ref);
	Payload& operator = (const Payload& ref);
	bool operator == (const ri_uuid_t& _id) const;
	bool operator == (const Payload& ref) const;
public:
	std::shared_ptr<pub::Payload> toPublish(uint32_t version) const;
	static std::shared_ptr<pub::RmPayload> toPublishRm(uint32_t version,const ri_uuid_t& id);


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

