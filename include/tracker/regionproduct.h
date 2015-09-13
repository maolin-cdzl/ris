#pragma once

#include <list>
#include <unordered_map>

#include "ris/riobserver.h"
#include "ris/regionlrureader.h"
#include "snapshot/snapshotbuilder.h"

class RegionProduct : public ISnapshotBuilder, public IRIObserver , public IRegionLRUReader {
public:
	RegionProduct();
	virtual ~RegionProduct();

public:
	// from IRegionLRUReader
	virtual const Region& getRegion() const;
	virtual const std::list<Service>& getServicesOrderByLRU() const;
	virtual const std::list<Payload>& getPayloadsOrderByLRU() const;

protected:
	// from ISnapshotBuilder
	virtual int addRegion(const Region& region);
	virtual int addService(const ri_uuid_t& region,const Service& svc);
	virtual int addPayload(const ri_uuid_t& region,const Payload& pl);
	virtual void onCompleted(int err);

protected:
	// from IRIObserver
	virtual void onRegion(const Region& region);
	virtual void onRmRegion(const ri_uuid_t& region);
	virtual void onService(const ri_uuid_t& region,uint32_t version,const Service& svc);
	virtual void onRmService(const ri_uuid_t& region,uint32_t version,const std::string& svc);
	virtual void onPayload(const ri_uuid_t& region,uint32_t version,const Payload& pl);
	virtual void onRmPayload(const ri_uuid_t& region,uint32_t version,const ri_uuid_t& pl);
private:
	Region					m_region;
	std::list<Service>		m_services;
	std::list<Payload>		m_payloads;

	std::unordered_map<std::string,std::list<Service>::iterator>	m_services_index;
	std::unordered_map<ri_uuid_t,std::list<Payload>::iterator>	m_payloads_index;
};

