#pragma once

#include <unordered_map>
#include <list>
#include <memory>
#include <czmq.h>
#include "ris/riobserver.h"
#include "ris/regionlrureader.h"
#include "ris/responsechain.hpp"
#include "snapshot/snapshotable.h"
#include "snapshot/snapshotbuilder.h"
#include "zmqx/zlooptimer.h"

class RITrackerTable : public ResponseChainHandler<IRIObserver>,public ISnapshotable {
public:
	RITrackerTable(zloop_t* loop);
	virtual ~RITrackerTable();

public:
	int start();
	void stop();

	// access method
	size_t region_size() const;
	size_t service_size() const;
	size_t payload_size() const;

	std::list<std::shared_ptr<Region>> regions() const;
	std::list<std::string> services() const;

	std::shared_ptr<Region> getRegion(const ri_uuid_t& id)  const;

	// get region carry this payload
	std::pair<bool,EndPoint> routePayload(const ri_uuid_t& id)  const;

	// get region provide this service,this is round-robin
	std::pair<bool,EndPoint> robinRouteService(const std::string& svc);

	int merge(const std::shared_ptr<IRegionLRUReader>& reader);
public:
	// method from IRIObserver
	virtual void onRegion(const Region& reg);
	virtual void onRmRegion(const ri_uuid_t& reg);
	virtual void onService(const ri_uuid_t& reg,uint32_t version,const Service& svc);
	virtual void onRmService(const ri_uuid_t& reg,uint32_t version,const std::string& svc);
	virtual void onPayload(const ri_uuid_t& reg,uint32_t version,const Payload& pl);
	virtual void onRmPayload(const ri_uuid_t& reg,uint32_t version,const ri_uuid_t& pl);

	// method from ISnapshotable
	virtual snapshot_package_t buildSnapshot();

private:
	struct RegionService {
		std::shared_ptr<Region>						region;
		Service										service;
	};
	struct RegionPayload {
		std::shared_ptr<Region>						region;
		Payload										payload;
	};

	typedef std::list<std::shared_ptr<Region>>				region_container_t;
	typedef typename region_container_t::iterator			region_iterator_t;
	typedef std::unordered_map<ri_uuid_t,region_iterator_t>	region_index_t;

	typedef std::list<RegionService>				service_container_t;
	typedef typename service_container_t::iterator	service_iterator_t;
	typedef std::unordered_map<std::string,std::list<service_iterator_t>>	service_index_t;

	typedef std::list<RegionPayload>				payload_container_t;
	typedef typename payload_container_t::iterator	payload_iterator_t;
	typedef std::unordered_map<ri_uuid_t,payload_iterator_t>					payload_index_t;

private:
	void updateRegionVersion(std::shared_ptr<Region>& region,uint32_t version);
	void updateRegionVersion(const ri_uuid_t& region,uint32_t version);
	std::list<service_iterator_t>::iterator findRegionService(std::list<service_iterator_t>& l,const ri_uuid_t& region);

	void doRmRegion(const ri_uuid_t& region);
	void doRmService(const ri_uuid_t& region,const std::string& svc);
	void doRmPayload(const ri_uuid_t& region,const ri_uuid_t& pl);
	int onCheckTimer();

private:
	zloop_t*										m_loop;
	ZLoopTimer										m_timer;
	region_container_t								m_regions;
	region_index_t									m_regions_index;
	service_container_t								m_services;
	service_index_t									m_services_index;
	payload_container_t								m_payloads;
	payload_index_t									m_payloads_index;
};

