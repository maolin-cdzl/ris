#include "ris/ritypes.h"

/**
 * class Region
 */

Region::Region(const Region& ref) :
	id(ref.id),
	version(ref.version),
	idc(ref.idc),
	bus_address(ref.bus_address),
	snapshot_address(ref.snapshot_address),
	timeval(ref.timeval)
{
}

Region& Region::operator = (const Region& ref) {
	if( this != &ref  ) {
		id = ref.id;
		version = ref.version;
		idc = ref.idc;
		bus_address = ref.bus_address;
		snapshot_address = ref.snapshot_address;
		timeval = ref.timeval;
	}
	return *this;
}

bool Region::operator == (const uuid_t& _id) const {
	return id == _id;
}

bool Region::operator == (const Region& ref) const {
	return id ==  ref.id;
}

std::shared_ptr<region::pub::Region> Region::toPublish() const {
	assert( ! id.empty() );
	std::shared_ptr<region::pub::Region> msg(new region::pub::Region());

	toPublishBase(msg->mutable_region());

	if( ! idc.empty() ) {
		msg->set_idc(idc);
	}
	if( ! bus_address.empty() ) {
		msg->set_bus_address(bus_address);
	}
	if( ! snapshot_address.empty() ) {
		msg->set_snapshot_address(snapshot_address);
	}
	return msg;
}

void Region::toPublishBase(region::pub::RegionBase* region) const {
	assert( ! id.empty() );
	region->set_uuid( id );
	region->set_version( version );
}

std::shared_ptr<region::pub::RmRegion> Region::toPublishRm(const uuid_t& uuid) {
	assert( ! uuid.empty() );
	std::shared_ptr<region::pub::RmRegion> msg(new region::pub::RmRegion());
	msg->set_uuid(uuid);
	return msg;
}

std::shared_ptr<snapshot::RegionBegin> Region::toSnapshotBegin() const {
	std::shared_ptr<snapshot::RegionBegin> msg(new snapshot::RegionBegin());
	msg->set_uuid(id);
	msg->set_version(version);
	if( ! idc.empty() ) {
		msg->set_idc(idc);
	}
	if( ! bus_address.empty() ) {
		msg->set_bus_address(bus_address);
	}
	if( ! snapshot_address.empty() ) {
		msg->set_snapshot_address(snapshot_address);
	}
	return msg;
}

std::shared_ptr<snapshot::RegionEnd> Region::toSnapshotEnd() const {
	std::shared_ptr<snapshot::RegionEnd> msg(new snapshot::RegionEnd());
	msg->set_uuid(id);
	return msg;
}



/**
 * class Service 
 */

Service::Service(const Service& ref) :
	name(ref.name),
	address(ref.address),
	timeval(ref.timeval)
{
}

Service& Service::operator = (const Service& ref) {
	if( this != &ref ) {
		name = ref.name;
		address = ref.address;
		timeval = ref.timeval;
	}
	return *this;
}

std::shared_ptr<snapshot::Service> Service::toSnapshot() const {
	std::shared_ptr<snapshot::Service> msg{ new snapshot::Service() };
	msg->set_name(name);
	msg->set_address(address);

	return msg;
}

std::shared_ptr<region::pub::Service> Service::toPublish(const uuid_t& region,uint32_t version) const {
	assert( ! name.empty() );
	assert( ! address.empty() );

	std::shared_ptr<region::pub::Service> msg(new region::pub::Service());
	msg->mutable_region()->set_uuid(region);
	msg->mutable_region()->set_version(version);
	msg->set_name( name );
	msg->set_address( address );
	return msg;
}

std::shared_ptr<region::pub::RmService> Service::toPublishRm(const uuid_t& region,uint32_t version,const std::string& name) {
	assert( ! name.empty() );

	std::shared_ptr<region::pub::RmService> msg(new region::pub::RmService());
	msg->mutable_region()->set_uuid(region);
	msg->mutable_region()->set_version(version);
	msg->set_name(name);

	return msg;
}


/**
 * class Payload
 */

Payload::Payload(const Payload& ref) :
	id(ref.id),
	timeval(ref.timeval)
{
}

Payload& Payload::operator = (const Payload& ref) {
	if( this != &ref ) {
		id = ref.id;
		timeval = ref.timeval;
	}
	return *this;
}

bool Payload::operator == (const uuid_t& _id) const {
	return id == _id;
}

bool Payload::operator == (const Payload& ref) const {
	return id == ref.id;
}

std::shared_ptr<region::pub::Payload> Payload::toPublish(const uuid_t& region,uint32_t version) const {
	assert( ! id.empty() );
	
	std::shared_ptr<region::pub::Payload> msg(new region::pub::Payload());
	msg->mutable_region()->set_uuid(region);
	msg->mutable_region()->set_version(version);
	msg->set_uuid( id );
	return msg;
}

std::shared_ptr<region::pub::RmPayload> Payload::toPublishRm(const uuid_t& region,uint32_t version,const uuid_t& id) {
	assert( ! id.empty() );
	
	std::shared_ptr<region::pub::RmPayload> msg(new region::pub::RmPayload());
	msg->mutable_region()->set_uuid(region);
	msg->mutable_region()->set_version(version);
	msg->set_uuid( id );
	return msg;
}


std::shared_ptr<snapshot::Payload> Payload::toSnapshot() const {
	std::shared_ptr<snapshot::Payload> msg{ new snapshot::Payload() };
	msg->set_uuid(id);
	return msg;
}


