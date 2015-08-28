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
	std::string				msg_address;
	std::string				snapshot_address;
	ri_time_t				timeval;

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

	Service() = default;
	inline Service(const std::string& n,const std::string& a,ri_time_t tv=0) :
		name(n),address(a),timeval(tv)
	{
	}
public:
	std::shared_ptr<region::pub::Service> toPublish(const Region& region) const;
	static std::shared_ptr<region::pub::RmService> toPublishRm(const Region& region,const std::string& name);

	std::shared_ptr<snapshot::Service> toSnapshot() const;
};

class Payload {
public:
	uuid_t					id;
	ri_time_t				timeval;

	Payload() = default;
	inline Payload(const uuid_t& _id,ri_time_t tv=0) :
		id(_id),timeval(tv)
	{
	}
public:
	std::shared_ptr<region::pub::Payload> toPublish(const Region& region) const;
	static std::shared_ptr<region::pub::RmPayload> toPublishRm(const Region& region,const uuid_t& id);


	std::shared_ptr<snapshot::Payload> toSnapshot() const;
};


