#include "ris/ritypes.h"

/**
 * class Region
 */

std::shared_ptr<region::pub::Region> Region::toPublish() const {
	assert( ! id.empty() );
	std::shared_ptr<region::pub::Region> msg(new region::pub::Region());

	toPublishBase(msg->mutable_region());

	if( ! idc.empty() ) {
		msg->set_idc(idc);
	}
	if( ! msg_address.empty() ) {
		msg->set_msg_address(msg_address);
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
	if( ! msg_address.empty() ) {
		msg->set_msg_address(msg_address);
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


std::shared_ptr<snapshot::Service> Service::toSnapshot() const {
	std::shared_ptr<snapshot::Service> msg{ new snapshot::Service() };
	msg->set_name(name);
	msg->set_address(address);

	return msg;
}

std::shared_ptr<region::pub::Service> Service::toPublish(const Region& region) const {
	assert( ! name.empty() );
	assert( ! address.empty() );

	std::shared_ptr<region::pub::Service> msg(new region::pub::Service());
	region.toPublishBase(msg->mutable_region());
	msg->set_name( name );
	msg->set_address( address );
	return msg;
}

std::shared_ptr<region::pub::RmService> Service::toPublishRm(const Region& region,const std::string& name) {
	assert( ! name.empty() );

	std::shared_ptr<region::pub::RmService> msg(new region::pub::RmService());
	region.toPublishBase(msg->mutable_region());
	msg->set_name(name);

	return msg;
}


/**
 * class Payload
 */

std::shared_ptr<region::pub::Payload> Payload::toPublish(const Region& region) const {
	assert( ! id.empty() );
	
	std::shared_ptr<region::pub::Payload> msg(new region::pub::Payload());
	region.toPublishBase(msg->mutable_region());
	msg->set_uuid( id );
	return msg;
}

std::shared_ptr<region::pub::RmPayload> Payload::toPublishRm(const Region& region,const uuid_t& id) {
	assert( ! id.empty() );
	
	std::shared_ptr<region::pub::RmPayload> msg(new region::pub::RmPayload());
	region.toPublishBase(msg->mutable_region());
	msg->set_uuid( id );
	return msg;
}


std::shared_ptr<snapshot::Payload> Payload::toSnapshot() const {
	std::shared_ptr<snapshot::Payload> msg{ new snapshot::Payload() };
	msg->set_uuid(id);
	return msg;
}


