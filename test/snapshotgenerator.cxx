#include <uuid/uuid.h>
#include <string>
#include <memory>
#include <random>
#include <sstream>
#include <chrono>

#include "snapshotgenerator.h"
#include "ris/snapshot.pb.h"

static std::string newUUID() {
    uuid_t uuid;
    uuid_generate_random ( uuid );
    char s[37];
    uuid_unparse( uuid, s );
    return s;
}

// class EmptySnapshotGenerator
snapshot_package_t EmptySnapshotGenerator::operator()() {
	snapshot_package_t package;
	package.push_back( std::make_shared<snapshot::SnapshotBegin>() );
	package.push_back( std::make_shared<snapshot::SnapshotEnd>() );
	return std::move(package);
}

// class DuplicateRegionGenerator
snapshot_package_t DuplicateRegionGenerator::operator()() {
	snapshot_package_t package;
	const std::string uuid = newUUID();
	package.push_back( std::make_shared<snapshot::SnapshotBegin>() );
	for(size_t i=0; i < 2; ++i) {
		auto begin = std::make_shared<snapshot::RegionBegin>();
		begin->set_uuid(uuid);
		begin->set_version(1);
		begin->set_idc("test_idc");
		begin->set_bus_address("tcp://test:8888");
		begin->set_snapshot_address("tcp://test:9999");
		package.push_back(begin);

		for(size_t i=0; i < 10; ++i) {
			std::stringstream ss;
			ss << "service-" << i;

			auto svc = std::make_shared<snapshot::Service>();
			svc->set_name(ss.str());
			svc->set_address("tcp://test:6666");
			package.push_back(svc);
		}

		for(size_t i=0; i < 10; ++i) {
			auto payload = std::make_shared<snapshot::Payload>();
			payload->set_uuid(newUUID());
			package.push_back(payload);
		}

		auto end = std::make_shared<snapshot::RegionEnd>();
		end->set_uuid(uuid);
		package.push_back(end);
	}
	package.push_back( std::make_shared<snapshot::SnapshotEnd>() );
	return std::move(package);
}

// class DuplicatePayloadGenerator
snapshot_package_t DuplicatePayloadGenerator::operator()() {
	snapshot_package_t package;
	const std::string uuid = newUUID();
	package.push_back( std::make_shared<snapshot::SnapshotBegin>() );
	auto begin = std::make_shared<snapshot::RegionBegin>();
	begin->set_uuid(uuid);
	begin->set_version(1);
	begin->set_idc("test_idc");
	begin->set_bus_address("tcp://test:8888");
	begin->set_snapshot_address("tcp://test:9999");
	package.push_back(begin);

	for(size_t i=0; i < 10; ++i) {
		std::stringstream ss;
		ss << "service-" << i;

		auto svc = std::make_shared<snapshot::Service>();
		svc->set_name(ss.str());
		svc->set_address("tcp://test:6666");
		package.push_back(svc);
	}

	for(size_t i=0; i < 10; ++i) {
		auto payload = std::make_shared<snapshot::Payload>();
		payload->set_uuid("Duplicated");
		package.push_back(payload);
	}

	auto end = std::make_shared<snapshot::RegionEnd>();
	end->set_uuid(uuid);
	package.push_back(end);
	package.push_back( std::make_shared<snapshot::SnapshotEnd>() );
	return std::move(package);
}

// class DuplicateServiceGenerator
snapshot_package_t DuplicateServiceGenerator::operator()() {
	snapshot_package_t package;
	const std::string uuid = newUUID();
	package.push_back( std::make_shared<snapshot::SnapshotBegin>() );
	auto begin = std::make_shared<snapshot::RegionBegin>();
	begin->set_uuid(uuid);
	begin->set_version(1);
	begin->set_idc("test_idc");
	begin->set_bus_address("tcp://test:8888");
	begin->set_snapshot_address("tcp://test:9999");
	package.push_back(begin);

	for(size_t i=0; i < 10; ++i) {
		auto svc = std::make_shared<snapshot::Service>();
		svc->set_name("Duplicate");
		svc->set_address("tcp://test:6666");
		package.push_back(svc);
	}

	for(size_t i=0; i < 10; ++i) {
		auto payload = std::make_shared<snapshot::Payload>();
		payload->set_uuid(newUUID());
		package.push_back(payload);
	}

	auto end = std::make_shared<snapshot::RegionEnd>();
	end->set_uuid(uuid);
	package.push_back(end);
	package.push_back( std::make_shared<snapshot::SnapshotEnd>() );
	return std::move(package);
}

// class UnmatchedRegionGenerator
snapshot_package_t UnmatchedRegionGenerator::operator()() {
	snapshot_package_t package;
	package.push_back( std::make_shared<snapshot::SnapshotBegin>() );
	auto begin = std::make_shared<snapshot::RegionBegin>();
	begin->set_uuid( newUUID() );
	begin->set_version(1);
	begin->set_idc("test_idc");
	begin->set_bus_address("tcp://test:8888");
	begin->set_snapshot_address("tcp://test:9999");
	package.push_back(begin);

	for(size_t i=0; i < 10; ++i) {
		std::stringstream ss;
		ss << "service-" << i;

		auto svc = std::make_shared<snapshot::Service>();
		svc->set_name(ss.str());
		svc->set_address("tcp://test:6666");
		package.push_back(svc);
	}

	for(size_t i=0; i < 10; ++i) {
		auto payload = std::make_shared<snapshot::Payload>();
		payload->set_uuid(newUUID());
		package.push_back(payload);
	}

	auto end = std::make_shared<snapshot::RegionEnd>();
	end->set_uuid( newUUID() );
	package.push_back(end);
	package.push_back( std::make_shared<snapshot::SnapshotEnd>() );
	return std::move(package);
}

// class UncompletedRegionGenerator
snapshot_package_t UncompletedRegionGenerator::operator()() {
	snapshot_package_t package;
	package.push_back( std::make_shared<snapshot::SnapshotBegin>() );
	auto begin = std::make_shared<snapshot::RegionBegin>();
	begin->set_uuid( newUUID() );
	begin->set_version(1);
	begin->set_idc("test_idc");
	begin->set_bus_address("tcp://test:8888");
	begin->set_snapshot_address("tcp://test:9999");
	package.push_back(begin);

	for(size_t i=0; i < 10; ++i) {
		std::stringstream ss;
		ss << "service-" << i;

		auto svc = std::make_shared<snapshot::Service>();
		svc->set_name(ss.str());
		svc->set_address("tcp://test:6666");
		package.push_back(svc);
	}

	for(size_t i=0; i < 10; ++i) {
		auto payload = std::make_shared<snapshot::Payload>();
		payload->set_uuid(newUUID());
		package.push_back(payload);
	}

	package.push_back( std::make_shared<snapshot::SnapshotEnd>() );
	return std::move(package);
}


// class SnapshotGenerator
SnapshotGenerator::SnapshotGenerator(size_t region_max,size_t payload_max,size_t service_max) :
	m_region_max(region_max),
	m_payload_max(payload_max),
	m_service_max(service_max),
	m_region_size(0),
	m_payload_size(0),
	m_service_size(0),
	m_generator(std::chrono::system_clock::now().time_since_epoch().count())
{
}


snapshot_package_t SnapshotGenerator::operator()() {
	snapshot_package_t package;
	package.push_back( std::make_shared<snapshot::SnapshotBegin>() );
	std::uniform_int_distribution<size_t> region_random(1,m_region_max);
	m_region_size = region_random(m_generator);
	for(size_t i=0; i < m_region_size; ++i) {
		generate_region(package);
	}
	package.push_back( std::make_shared<snapshot::SnapshotEnd>() );
	return std::move(package);
}
	
void SnapshotGenerator::generate_region(snapshot_package_t& package) {
	std::uniform_int_distribution<size_t> payload_random(1,m_payload_max);
	std::uniform_int_distribution<size_t> service_random(1,m_service_max);
	std::uniform_int_distribution<uint32_t> version_random(0,0xFFFFFFF);

	const size_t payload_size = payload_random(m_generator);
	const size_t service_size = service_random(m_generator);
	m_payload_size += payload_size;
	m_service_size += service_size;

	const std::string uuid = newUUID();
	auto begin = std::make_shared<snapshot::RegionBegin>();
	begin->set_uuid(uuid);
	begin->set_version(version_random(m_generator));
	begin->set_idc("test_idc");
	begin->set_bus_address("tcp://test:8888");
	begin->set_snapshot_address("tcp://test:9999");
	package.push_back(begin);

	for(size_t i=0; i < service_size; ++i) {
		std::stringstream ss;
		ss << "service-" << i;

		auto svc = std::make_shared<snapshot::Service>();
		svc->set_name(ss.str());
		svc->set_address("tcp://test:6666");
		package.push_back(svc);
	}

	for(size_t i=0; i < payload_size; ++i) {
		auto payload = std::make_shared<snapshot::Payload>();
		payload->set_uuid(newUUID());
		package.push_back(payload);
	}

	auto end = std::make_shared<snapshot::RegionEnd>();
	end->set_uuid(uuid);
	package.push_back(end);
}

