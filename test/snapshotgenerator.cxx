#include <string>
#include <memory>
#include <random>
#include <sstream>
#include <chrono>

#include "snapshotgenerator.h"
#include "test_helper.h"
#include "ris/snapshot.pb.h"

// class EmptySnapshotGenerator
snapshot_package_t EmptySnapshotGenerator::build() {
	snapshot_package_t package;
	package.push_back( std::make_shared<snapshot::SnapshotBegin>() );
	package.push_back( std::make_shared<snapshot::SnapshotEnd>() );
	return std::move(package);
}

size_t EmptySnapshotGenerator::region_size() const {
	return 0;
}

size_t EmptySnapshotGenerator::payload_size() const {
	return 0;
}

size_t EmptySnapshotGenerator::service_size() const {
	return 0;
}

// class DuplicateRegionGenerator
snapshot_package_t DuplicateRegionGenerator::build() {
	snapshot_package_t package;
	const std::string uuid = newUUID();
	package.push_back( std::make_shared<snapshot::SnapshotBegin>() );
	for(size_t i=0; i < 2; ++i) {
		auto begin = std::make_shared<snapshot::RegionBegin>();
		begin->mutable_rt()->set_uuid(uuid);
		begin->mutable_rt()->set_version(1);
		begin->mutable_region()->set_uuid(uuid);
		begin->mutable_region()->set_idc("test_idc");
		begin->mutable_region()->mutable_bus()->set_address("tcp://test:8888");
		begin->mutable_region()->mutable_bus()->set_identity(uuid + "-bus");
		begin->mutable_region()->mutable_snapshot()->set_address("tcp://test:9999");
		begin->mutable_region()->mutable_snapshot()->set_identity(uuid + "-snapshot");
		package.push_back(begin);

		for(size_t i=0; i < 10; ++i) {
			std::stringstream ss;
			ss << "service-" << i;

			auto svc = std::make_shared<ris::Service>();
			svc->set_name(ss.str());
			svc->mutable_endpoint()->set_address("tcp://test:6666");
			svc->mutable_endpoint()->set_identity(uuid + svc->name());
			package.push_back(svc);
		}

		for(size_t i=0; i < 10; ++i) {
			auto payload = std::make_shared<ris::Payload>();
			payload->set_uuid(newUUID());
			package.push_back(payload);
		}

		auto end = std::make_shared<snapshot::RegionEnd>();
		end->mutable_rt()->set_uuid(uuid);
		end->mutable_rt()->set_version(1);
		package.push_back(end);
	}
	package.push_back( std::make_shared<snapshot::SnapshotEnd>() );
	return std::move(package);
}

size_t DuplicateRegionGenerator::region_size() const {
	return 1;
}

size_t DuplicateRegionGenerator::payload_size() const {
	return 10;
}

size_t DuplicateRegionGenerator::service_size() const {
	return 10;
}

// class DuplicatePayloadGenerator
snapshot_package_t DuplicatePayloadGenerator::build() {
	snapshot_package_t package;
	const std::string uuid = newUUID();
	package.push_back( std::make_shared<snapshot::SnapshotBegin>() );
	auto begin = std::make_shared<snapshot::RegionBegin>();
	begin->mutable_rt()->set_uuid(uuid);
	begin->mutable_rt()->set_version(1);
	begin->mutable_region()->set_uuid(uuid);
	begin->mutable_region()->set_idc("test_idc");
	begin->mutable_region()->mutable_bus()->set_address("tcp://test:8888");
	begin->mutable_region()->mutable_bus()->set_identity(uuid + "-bus");
	begin->mutable_region()->mutable_snapshot()->set_address("tcp://test:9999");
	begin->mutable_region()->mutable_snapshot()->set_identity(uuid + "-snapshot");
	package.push_back(begin);

	for(size_t i=0; i < 10; ++i) {
		std::stringstream ss;
		ss << "service-" << i;

		auto svc = std::make_shared<ris::Service>();
		svc->set_name(ss.str());
		svc->mutable_endpoint()->set_address("tcp://test:6666");
		svc->mutable_endpoint()->set_identity("tcp://test:6666");
		package.push_back(svc);
	}

	for(size_t i=0; i < 10; ++i) {
		auto payload = std::make_shared<ris::Payload>();
		payload->set_uuid("Duplicated");
		package.push_back(payload);
	}

	auto end = std::make_shared<snapshot::RegionEnd>();
	end->mutable_rt()->set_uuid(uuid);
	end->mutable_rt()->set_version(1);

	package.push_back(end);
	package.push_back( std::make_shared<snapshot::SnapshotEnd>() );
	return std::move(package);
}

size_t DuplicatePayloadGenerator::region_size() const {
	return 1;
}

size_t DuplicatePayloadGenerator::payload_size() const {
	return 10;
}

size_t DuplicatePayloadGenerator::service_size() const {
	return 10;
}

// class DuplicateServiceGenerator
snapshot_package_t DuplicateServiceGenerator::build() {
	snapshot_package_t package;
	const std::string uuid = newUUID();
	package.push_back( std::make_shared<snapshot::SnapshotBegin>() );
	auto begin = std::make_shared<snapshot::RegionBegin>();
	begin->mutable_rt()->set_uuid(uuid);
	begin->mutable_rt()->set_version(1);
	begin->mutable_region()->set_uuid(uuid);
	begin->mutable_region()->set_idc("test_idc");
	begin->mutable_region()->mutable_bus()->set_address("tcp://test:8888");
	begin->mutable_region()->mutable_bus()->set_identity(uuid + "-bus");
	begin->mutable_region()->mutable_snapshot()->set_address("tcp://test:9999");
	begin->mutable_region()->mutable_snapshot()->set_identity(uuid + "-snapshot");
	package.push_back(begin);

	for(size_t i=0; i < 10; ++i) {
		auto svc = std::make_shared<ris::Service>();
		svc->set_name("Duplicate");
		svc->mutable_endpoint()->set_address("tcp://test:6666");
		svc->mutable_endpoint()->set_identity("tcp://test:6666");
		package.push_back(svc);
	}

	for(size_t i=0; i < 10; ++i) {
		auto payload = std::make_shared<ris::Payload>();
		payload->set_uuid(newUUID());
		package.push_back(payload);
	}

	auto end = std::make_shared<snapshot::RegionEnd>();
	end->mutable_rt()->set_uuid(uuid);
	end->mutable_rt()->set_version(1);
	package.push_back(end);
	package.push_back( std::make_shared<snapshot::SnapshotEnd>() );
	return std::move(package);
}

size_t DuplicateServiceGenerator::region_size() const {
	return 1;
}

size_t DuplicateServiceGenerator::payload_size() const {
	return 10;
}

size_t DuplicateServiceGenerator::service_size() const {
	return 10;
}

// class UnmatchedRegionGenerator
snapshot_package_t UnmatchedRegionGenerator::build() {
	snapshot_package_t package;
	package.push_back( std::make_shared<snapshot::SnapshotBegin>() );
	auto begin = std::make_shared<snapshot::RegionBegin>();

	begin->mutable_rt()->set_uuid(newUUID());
	begin->mutable_rt()->set_version(1);
	begin->mutable_region()->set_uuid(begin->rt().uuid());
	begin->mutable_region()->set_idc("test_idc");
	begin->mutable_region()->mutable_bus()->set_address("tcp://test:8888");
	begin->mutable_region()->mutable_bus()->set_identity(begin->region().uuid() + "-bus");
	begin->mutable_region()->mutable_snapshot()->set_address("tcp://test:9999");
	begin->mutable_region()->mutable_snapshot()->set_identity( begin->region().uuid() + "-snapshot");
	package.push_back(begin);

	for(size_t i=0; i < 10; ++i) {
		std::stringstream ss;
		ss << "service-" << i;

		auto svc = std::make_shared<ris::Service>();
		svc->set_name(ss.str());
		svc->mutable_endpoint()->set_address("tcp://test:6666");
		svc->mutable_endpoint()->set_identity("tcp://test:6666");
		package.push_back(svc);
	}

	for(size_t i=0; i < 10; ++i) {
		auto payload = std::make_shared<ris::Payload>();
		payload->set_uuid(newUUID());
		package.push_back(payload);
	}

	auto end = std::make_shared<snapshot::RegionEnd>();
	end->mutable_rt()->set_uuid( newUUID() );
	end->mutable_rt()->set_version( 1 );
	package.push_back(end);
	package.push_back( std::make_shared<snapshot::SnapshotEnd>() );
	return std::move(package);
}

size_t UnmatchedRegionGenerator::region_size() const {
	return 1;
}

size_t UnmatchedRegionGenerator::payload_size() const {
	return 10;
}

size_t UnmatchedRegionGenerator::service_size() const {
	return 10;
}


// class UncompletedRegionGenerator
snapshot_package_t UncompletedRegionGenerator::build() {
	snapshot_package_t package;
	package.push_back( std::make_shared<snapshot::SnapshotBegin>() );
	auto begin = std::make_shared<snapshot::RegionBegin>();
	const ri_uuid_t uuid = newUUID();
	begin->mutable_rt()->set_uuid(uuid);
	begin->mutable_rt()->set_version(1);
	begin->mutable_region()->set_uuid(uuid);
	begin->mutable_region()->set_idc("test_idc");
	begin->mutable_region()->mutable_bus()->set_address("tcp://test:8888");
	begin->mutable_region()->mutable_bus()->set_identity(uuid + "-bus");
	begin->mutable_region()->mutable_snapshot()->set_address("tcp://test:9999");
	begin->mutable_region()->mutable_snapshot()->set_identity(uuid + "-snapshot");
	package.push_back(begin);

	for(size_t i=0; i < 10; ++i) {
		std::stringstream ss;
		ss << "service-" << i;

		auto svc = std::make_shared<ris::Service>();
		svc->set_name(ss.str());
		svc->mutable_endpoint()->set_address("tcp://test:6666");
		svc->mutable_endpoint()->set_identity("tcp://test:6666");
		package.push_back(svc);
	}

	for(size_t i=0; i < 10; ++i) {
		auto payload = std::make_shared<ris::Payload>();
		payload->set_uuid(newUUID());
		package.push_back(payload);
	}

	package.push_back( std::make_shared<snapshot::SnapshotEnd>() );
	return std::move(package);
}

size_t UncompletedRegionGenerator::region_size() const {
	return 1;
}

size_t UncompletedRegionGenerator::payload_size() const {
	return 10;
}

size_t UncompletedRegionGenerator::service_size() const {
	return 10;
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


snapshot_package_t SnapshotGenerator::build() {
	snapshot_package_t package;
	package.push_back( std::make_shared<snapshot::SnapshotBegin>() );
	std::uniform_int_distribution<size_t> region_random(1,m_region_max);
	size_t region_size = region_random(m_generator);
	m_region_size += region_size;
	for(size_t i=0; i < region_size; ++i) {
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
	begin->mutable_rt()->set_uuid(uuid);
	begin->mutable_rt()->set_version(1);
	begin->mutable_region()->set_uuid(uuid);
	begin->mutable_region()->set_idc("test_idc");
	begin->mutable_region()->mutable_bus()->set_address("tcp://test:8888");
	begin->mutable_region()->mutable_bus()->set_identity(uuid + "-bus");
	begin->mutable_region()->mutable_snapshot()->set_address("tcp://test:9999");
	begin->mutable_region()->mutable_snapshot()->set_identity(uuid + "-snapshot");
	package.push_back(begin);

	for(size_t i=0; i < service_size; ++i) {
		std::stringstream ss;
		ss << "service-" << i;

		auto svc = std::make_shared<ris::Service>();
		svc->set_name(ss.str());
		svc->mutable_endpoint()->set_address("tcp://test:6666");
		svc->mutable_endpoint()->set_identity("tcp://test:6666");
		package.push_back(svc);
	}

	for(size_t i=0; i < payload_size; ++i) {
		auto payload = std::make_shared<ris::Payload>();
		payload->set_uuid(newUUID());
		package.push_back(payload);
	}

	auto end = std::make_shared<snapshot::RegionEnd>();
	end->mutable_rt()->set_uuid(uuid);
	end->mutable_rt()->set_version(1);
	package.push_back(end);
}

size_t SnapshotGenerator::region_size() const {
	return m_region_size;
}

size_t SnapshotGenerator::payload_size() const {
	return m_payload_size;
}

size_t SnapshotGenerator::service_size() const {
	return m_service_size;
}


//

testing::Cardinality RegionNumber(const std::shared_ptr<ISnapshotGeneratorImpl>& impl) {
	return InvokeCardinality::makeBetween(std::bind(&ISnapshotGeneratorImpl::region_size,impl),std::bind(&ISnapshotGeneratorImpl::region_size,impl));
}

testing::Cardinality RegionAtMost(const std::shared_ptr<ISnapshotGeneratorImpl>& impl) {
	return InvokeCardinality::makeAtMost(std::bind(&ISnapshotGeneratorImpl::region_size,impl));
}

testing::Cardinality ServiceNumber(const std::shared_ptr<ISnapshotGeneratorImpl>& impl) {
	return InvokeCardinality::makeBetween(std::bind(&ISnapshotGeneratorImpl::service_size,impl),std::bind(&ISnapshotGeneratorImpl::service_size,impl));
}

testing::Cardinality ServiceAtMost(const std::shared_ptr<ISnapshotGeneratorImpl>& impl) {
	return InvokeCardinality::makeAtMost(std::bind(&ISnapshotGeneratorImpl::service_size,impl));
}

testing::Cardinality PayloadNumber(const std::shared_ptr<ISnapshotGeneratorImpl>& impl) {
	return InvokeCardinality::makeBetween(std::bind(&ISnapshotGeneratorImpl::payload_size,impl),std::bind(&ISnapshotGeneratorImpl::payload_size,impl));
}

testing::Cardinality PayloadAtMost(const std::shared_ptr<ISnapshotGeneratorImpl>& impl) {
	return InvokeCardinality::makeAtMost(std::bind(&ISnapshotGeneratorImpl::payload_size,impl));
}

