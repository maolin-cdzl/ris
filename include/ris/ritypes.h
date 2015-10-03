#pragma once

#include <string>
#include <czmq.h>

#include "zmqx/zhelper.h"
#include "ris/ris.pb.h"
#include "ris/pub.pb.h"
#include "ris/snapshot.pb.h"
#include "snapshot/snapshotable.h"

typedef std::string			ri_uuid_t;
typedef uint64_t			ri_time_t;		// msec

#define ri_time_now			time_now

struct EndPoint {
	std::string				address;
	std::string				identity;

	EndPoint();
	EndPoint(const std::string& addr);
	EndPoint(const std::string& addr,const std::string& id);
	EndPoint(const ris::EndPoint& ep);

	void toProtobuf(ris::EndPoint* msg) const;

	inline bool operator == (const EndPoint& other) const {
		return (address == other.address) && (identity == other.identity);
	}

	inline bool good() const {
		return !address.empty();
	}

	inline bool all_good() const {
		return (!address.empty()) && (!identity.empty());
	}
};

class Region {
public:
	ri_uuid_t				id;
	uint32_t				version;
	std::string				idc;
	EndPoint				bus;
	EndPoint				snapshot;
	ri_time_t				timeval;

	Region();
	Region(const Region& ref);
	Region(const ris::Region& r);
	Region& operator = (const Region& ref);

	bool operator == (const ri_uuid_t& _id) const;
	bool operator == (const Region& ref) const;

public:
	bool good() const;

	void toProtobuf(ris::Region* msg) const;
	void toProtobuf(ris::RegionRuntime* msg) const;

	std::shared_ptr<pub::PubRegion> toPublish() const ;
	static std::shared_ptr<pub::PubRmRegion> toPublishRm(const ri_uuid_t& uuid);

	std::shared_ptr<snapshot::RegionBegin> toSnapshotBegin() const;
	std::shared_ptr<snapshot::RegionEnd> toSnapshotEnd() const;
};


class Service {
public:
	std::string				name;
	EndPoint				endpoint;
	ri_time_t				timeval;

	Service();
	Service(const std::string& n,const std::string& a,const std::string& id);
	Service(const Service& ref);
	Service(const ris::Service& s);
	Service& operator = (const Service& ref);
	
public:
	bool good() const;

	void toProtobuf(ris::Service* msg) const;
	std::shared_ptr<ris::Service> toProtobuf() const;

	std::shared_ptr<pub::PubService> toPublish(const Region& region) const;
	std::shared_ptr<pub::PubService> toPublish(const ri_uuid_t& region,uint32_t version) const;
	static std::shared_ptr<pub::PubRmService> toPublishRm(const ri_uuid_t& region,uint32_t version,const std::string& name);
	static std::shared_ptr<pub::PubRmService> toPublishRm(const Region& region,const std::string& name);

	inline std::shared_ptr<ris::Service> toSnapshot() const {
		return toProtobuf();
	}
};

class Payload {
public:
	ri_uuid_t					id;
	ri_time_t				timeval;

	Payload();
	Payload(const ri_uuid_t& _id);
	Payload(const Payload& ref);
	Payload(const ris::Payload& p);

	Payload& operator = (const Payload& ref);
	bool operator == (const ri_uuid_t& _id) const;
	bool operator == (const Payload& ref) const;
public:
	inline bool good() const {
		return ! id.empty();
	}

	void toProtobuf(ris::Payload* msg) const;
	std::shared_ptr<ris::Payload> toProtobuf() const;

	std::shared_ptr<pub::PubPayload> toPublish(const Region& region) const;
	std::shared_ptr<pub::PubPayload> toPublish(const ri_uuid_t& region,uint32_t version) const;
	static std::shared_ptr<pub::PubRmPayload> toPublishRm(const Region& region,const ri_uuid_t& id);
	static std::shared_ptr<pub::PubRmPayload> toPublishRm(const ri_uuid_t& region,uint32_t version,const ri_uuid_t& id);

	inline std::shared_ptr<ris::Payload> toSnapshot() const {
		return toProtobuf();
	}
};


namespace std {
	template<>
	struct hash<EndPoint> {
		std::size_t operator()(const EndPoint& ep) const {
			return std::hash<std::string>()(ep.address + ep.identity);
		}
	};

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

