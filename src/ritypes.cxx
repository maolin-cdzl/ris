#include <glog/logging.h>
#include "ris/ritypes.h"

/**
 * class EndPoint
 */

EndPoint::EndPoint() {
}

EndPoint::EndPoint(const std::string& addr) :
	address(addr)
{
}

EndPoint::EndPoint(const std::string& addr,const std::string& id) :
	address(addr),identity(id)
{
}

EndPoint::EndPoint(const ris::EndPoint& ep) :
	address(ep.address()),
	identity(ep.identity())
{
}

void EndPoint::toProtobuf(ris::EndPoint* msg) const {
	CHECK_NOTNULL(msg);
	CHECK(! address.empty());
	msg->set_address(address);
	if( !identity.empty() ) {
		msg->set_identity(identity);
	}
}

/**
 * class Region
 */

Region::Region() :
	version(0),timeval(0)
{
}

Region::Region(const Region& ref) :
	id(ref.id),
	version(ref.version),
	idc(ref.idc),
	bus(ref.bus),
	snapshot(ref.snapshot),
	timeval(ref.timeval)
{
}

Region::Region(const ris::Region& r) :
	id(r.uuid()),
	version(0),
	idc(r.idc()),
	bus(r.bus()),
	snapshot(r.snapshot()),
	timeval(ri_time_now())
{
}

Region& Region::operator = (const Region& ref) {
	if( this != &ref  ) {
		id = ref.id;
		version = ref.version;
		idc = ref.idc;
		bus = ref.bus;
		snapshot = ref.snapshot;
		timeval = ref.timeval;
	}
	return *this;
}

bool Region::operator == (const ri_uuid_t& _id) const {
	return id == _id;
}

bool Region::operator == (const Region& ref) const {
	return id ==  ref.id;
}

bool Region::good() const {
	if( id.empty() || idc.empty() || !bus.all_good() || !snapshot.all_good() || timeval == 0 ) {
		return false;
	} else {
		return true;
	}
}

void Region::toProtobuf(ris::Region* msg) const {
	CHECK_NOTNULL(msg);
	CHECK( (!id.empty()) && (!idc.empty()) );
	msg->set_uuid(id);
	msg->set_idc(idc);
	bus.toProtobuf(msg->mutable_bus());
	snapshot.toProtobuf(msg->mutable_snapshot());
}

void Region::toProtobuf(ris::RegionRuntime* msg) const {
	CHECK_NOTNULL(msg);
	msg->set_uuid(id);
	msg->set_version(version);
}

std::shared_ptr<pub::PubRegion> Region::toPublish() const {
	auto msg = std::make_shared<pub::PubRegion>();
	toProtobuf(msg->mutable_rt());
	toProtobuf(msg->mutable_region());
	return msg;
}

std::shared_ptr<pub::PubRmRegion> Region::toPublishRm(const ri_uuid_t& uuid) {
	CHECK( ! uuid.empty() );
	auto msg = std::make_shared<pub::PubRmRegion>();
	msg->set_uuid(uuid);
	return msg;
}

std::shared_ptr<snapshot::RegionBegin> Region::toSnapshotBegin() const {
	auto msg = std::make_shared<snapshot::RegionBegin>();
	toProtobuf(msg->mutable_rt());
	toProtobuf(msg->mutable_region());
	return msg;
}

std::shared_ptr<snapshot::RegionEnd> Region::toSnapshotEnd() const {
	auto msg = std::make_shared<snapshot::RegionEnd>();
	toProtobuf(msg->mutable_rt());
	return msg;
}



/**
 * class Service 
 */

Service::Service() :
	timeval(0)
{
}

Service::Service(const std::string& n,const std::string& a,const std::string& id) :
	name(n),
	endpoint(a,id),
	timeval(ri_time_now())
{
}

Service::Service(const Service& ref) :
	name(ref.name),
	endpoint(ref.endpoint),
	timeval(ref.timeval)
{
}

Service::Service(const ris::Service& s) :
	name(s.name()),
	endpoint(s.endpoint()),
	timeval(ri_time_now())
{
}

Service& Service::operator = (const Service& ref) {
	if( this != &ref ) {
		name = ref.name;
		endpoint = ref.endpoint;
		timeval = ref.timeval;
	}
	return *this;
}

bool Service::good() const {
	if( name.empty() || !endpoint.all_good() || 0 == timeval ) {
		return false;
	} else {
		return true;
	}
}

void Service::toProtobuf(ris::Service* msg) const {
	CHECK_NOTNULL(msg);
	CHECK( !name.empty() );
	msg->set_name(name);
	endpoint.toProtobuf(msg->mutable_endpoint());
}

std::shared_ptr<ris::Service> Service::toProtobuf() const {
	CHECK( !name.empty() );
	auto msg = std::make_shared<ris::Service>();
	msg->set_name(name);
	endpoint.toProtobuf(msg->mutable_endpoint());
	return std::move(msg);
}

std::shared_ptr<pub::PubService> Service::toPublish(const Region& region) const {
	CHECK( ! name.empty() );
	auto msg = std::make_shared<pub::PubService>();
	region.toProtobuf(msg->mutable_rt());
	toProtobuf(msg->mutable_service());
	return msg;
}

std::shared_ptr<pub::PubService> Service::toPublish(const ri_uuid_t& region,uint32_t version) const {
	CHECK( ! name.empty() );
	auto msg = std::make_shared<pub::PubService>();
	msg->mutable_rt()->set_uuid(region);
	msg->mutable_rt()->set_version(version);
	toProtobuf(msg->mutable_service());
	return msg;
}

std::shared_ptr<pub::PubRmService> Service::toPublishRm(const Region& region,const std::string& name) {
	CHECK( ! name.empty() );
	auto msg = std::make_shared<pub::PubRmService>();
	region.toProtobuf(msg->mutable_rt());
	msg->set_name(name);

	return msg;
}

std::shared_ptr<pub::PubRmService> Service::toPublishRm(const ri_uuid_t& region,uint32_t version,const std::string& name) {
	CHECK( ! name.empty() );
	auto msg = std::make_shared<pub::PubRmService>();
	msg->mutable_rt()->set_uuid(region);
	msg->mutable_rt()->set_version(version);
	msg->set_name(name);

	return msg;
}

/**
 * class Payload
 */

Payload::Payload() :
	timeval(0)
{
}

Payload::Payload(const ri_uuid_t& _id) :
	id(_id),
	timeval(ri_time_now())
{
}

Payload::Payload(const Payload& ref) :
	id(ref.id),
	timeval(ref.timeval)
{
}

Payload::Payload(const ris::Payload& p) :
	id(p.uuid()),
	timeval(ri_time_now())
{
}

Payload& Payload::operator = (const Payload& ref) {
	if( this != &ref ) {
		id = ref.id;
		timeval = ref.timeval;
	}
	return *this;
}

bool Payload::operator == (const ri_uuid_t& _id) const {
	return id == _id;
}

bool Payload::operator == (const Payload& ref) const {
	return id == ref.id;
}

void Payload::toProtobuf(ris::Payload* msg) const {
	CHECK_NOTNULL(msg);
	CHECK( !id.empty() );
	msg->set_uuid(id);
}

std::shared_ptr<ris::Payload> Payload::toProtobuf() const {
	CHECK( !id.empty() );
	auto msg = std::make_shared<ris::Payload>();
	msg->set_uuid(id);
	return msg;
}

std::shared_ptr<pub::PubPayload> Payload::toPublish(const Region& region) const {
	auto msg = std::make_shared<pub::PubPayload>();
	region.toProtobuf(msg->mutable_rt());
	toProtobuf(msg->mutable_payload());
	return msg;
}

std::shared_ptr<pub::PubPayload> Payload::toPublish(const ri_uuid_t& region,uint32_t version) const {
	auto msg = std::make_shared<pub::PubPayload>();
	msg->mutable_rt()->set_uuid(region);
	msg->mutable_rt()->set_version(version);
	toProtobuf(msg->mutable_payload());
	return msg;
}

std::shared_ptr<pub::PubRmPayload> Payload::toPublishRm(const Region& region,const ri_uuid_t& id) {
	CHECK( ! id.empty() );
	auto msg = std::make_shared<pub::PubRmPayload>();
	region.toProtobuf(msg->mutable_rt());
	msg->set_uuid( id );
	return msg;
}

std::shared_ptr<pub::PubRmPayload> Payload::toPublishRm(const ri_uuid_t& region,uint32_t version,const ri_uuid_t& id) {
	CHECK( ! id.empty() );
	auto msg = std::make_shared<pub::PubRmPayload>();
	msg->mutable_rt()->set_uuid(region);
	msg->mutable_rt()->set_version(version);
	msg->set_uuid( id );
	return msg;
}


