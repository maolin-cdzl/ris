#include "ris/snapshot/snapshot.h"

/************************
 * class SnapshotValues
 ***********************/

SnapshotValues::~SnapshotValues() {
}

void SnapshotValues::addValue(const std::string& name,const std::string& val) {
	assert( ! name.empty() );
	assert( ! val.empty() );
	m_values.push_back(std::make_pair(name,val));
}

int SnapshotValues::package(zmsg_t* msg) {
	assert(msg);
	for(auto it=m_values.begin(); it != m_values.end(); ++it) {
		if( -1 == zmsg_addstr(msg,it->first.c_str()) )
			return -1;
		if( -1 == zmsg_addstr(msg,it->second.c_str()) )
			return -1;
	}
	return 0;
}

int SnapshotValues::parseValues(zmsg_t* msg) {
	assert( msg );
	if( zmsg_size(msg) % 2 != 0 )
		return -1;

	do {
		char* name = zmsg_popstr(msg);
		if( nullptr == name ) {
			break;
		}
		char* val = zmsg_popstr(msg);
		if( nullptr == val ) {
			free(name);
			return -1;
		}
		m_values.push_back(std::make_pair(std::string(name),std::string(val)));
		free(name);
		free(val);
	} while(true);

	return 0;
}

/************************
 * class SnapshotItem
 ***********************/

SnapshotItem::SnapshotItem(const std::string& id) :
	m_id(id)
{
}

int SnapshotItem::send(zsock_t* sock) {
	zmsg_t* msg = zmsg_new();
	zmsg_addstr(msg,"ssit");
	zmsg_addstr(msg,m_id.c_str());

	package(msg);
	
	return zmsg_send(&msg,sock);
}

bool SnapshotItem::is(zmsg_t* msg) {
	assert( msg );
	zframe_t* fr = zmsg_first(msg);
	if( fr ) {
		return zframe_streq(fr,"$ssit");
	} else {
		return false;
	}
}

std::shared_ptr<SnapshotItem> SnapshotItem::parse(zmsg_t* msg) {
	assert( msg );
	std::shared_ptr<SnapshotItem> item;
	char* magic = nullptr;
	char* id = nullptr;

	do {
		magic = zmsg_popstr(msg);
		if( nullptr == magic )
			break;
		if( 0 != strcmp(magic,"$ssit") )
			break;
		id = zmsg_popstr(msg);
		if( nullptr == id )
			break;
		item = std::shared_ptr<SnapshotItem>(new SnapshotItem(id));
		if( -1 == item->parseValues(msg) ) {
			item.reset();
		}
	} while( 0 );

	if( magic )
		free(magic);
	if( id )
		free(id);
	return item;
}

/************************
 * class SnapshotPartition
 ***********************/

SnapshotPartition::SnapshotPartition(const std::string& id,uint32_t version) :
	m_id(id),
	m_version(version)
{
}


void SnapshotPartition::addItem(const std::shared_ptr<SnapshotItem>& item) {
	assert(item);
	m_items.push_back(item);
}

std::shared_ptr<SnapshotItem> SnapshotPartition::popItem() {
	if( ! m_items.empty() ) {
		auto it = m_items.front();
		m_items.pop_front();
		return it;
	} else {
		return nullptr;
	}
}

int SnapshotPartition::send(zsock_t* sock) {
	assert(sock);
	zmsg_t* msg = zmsg_new();

	zmsg_addstr(msg,"$sspt");
	zmsg_addstr(msg,m_id.c_str());
	char ver[32];
	snprintf(ver,sizeof(ver),"%u",m_version);
	zmsg_addstr(msg,ver);

	char count[32];
	snprintf(count,sizeof(count),"%u",(uint32_t)m_items.size());
	zmsg_addstr(msg,count);

	package(msg);

	return zmsg_send(&msg,sock);
}

int SnapshotPartition::sendBorder(zsock_t* sock) {
	assert(sock);
	zmsg_t* msg = zmsg_new();

	zmsg_addstr(msg,"$ssbopt");
	zmsg_addstr(msg,m_id.c_str());
	char ver[32];
	snprintf(ver,sizeof(ver),"%u",m_version);
	zmsg_addstr(msg,ver);

	return zmsg_send(&msg,sock);
}

bool SnapshotPartition::is(zmsg_t* msg) {
	assert( msg );
	zframe_t* fr = zmsg_first(msg);
	if( fr ) {
		return zframe_streq(fr,"$sspt");
	} else {
		return false;
	}
}

bool SnapshotPartition::isBorder(zmsg_t* msg) {
	assert( msg );
	zframe_t* fr = zmsg_first(msg);
	if( fr ) {
		return zframe_streq(fr,"$ssbopt");
	} else {
		return false;
	}
}

std::shared_ptr<SnapshotPartition> SnapshotPartition::parse(zmsg_t* msg) {
	assert( msg );
	std::shared_ptr<SnapshotPartition> part;
	char* magic = nullptr;
	char* id = nullptr;
	char* str = nullptr;
	uint32_t val = 0;

	do {
		magic = zmsg_popstr(msg);
		if( nullptr == magic )
			break;
		if( 0 != strcmp(magic,"$sspt") )
			break;
		id = zmsg_popstr(msg);
		if( nullptr == id )
			break;

		str = zmsg_popstr(msg);
		if( nullptr == str )
			break;
		if( sscanf(str,"%u",&val) != 1 )
			break;
		free(str);

		str = zmsg_popstr(msg);		// count of items
		if( nullptr == str )
			break;

		part = std::shared_ptr<SnapshotPartition>(new SnapshotPartition(id,val));
		if( -1 == part->parseValues(msg) ) {
			part.reset();
		}
	} while( 0 );

	if( magic )
		free(magic);
	if( id )
		free(id);
	if( str )
		free(str);
	return part;
}

/************************
 * class Snapshot
 ***********************/

void Snapshot::addPartition(const std::shared_ptr<SnapshotPartition>& part) {
	assert( part );
	m_partitions.push_back(part);
}

std::shared_ptr<SnapshotPartition> Snapshot::popPartition() {
	if( ! m_partitions.empty() ) {
		auto pt = m_partitions.front();
		m_partitions.pop_front();
		return pt;
	} else {
		return nullptr;
	}
}

int Snapshot::send(zsock_t* sock) {
	assert(sock);
	zmsg_t* msg = zmsg_new();

	zmsg_addstr(msg,"$ss");
	char count[32];
	snprintf(count,sizeof(count),"%u",(uint32_t)m_partitions.size());
	zmsg_addstr(msg,count);

	package(msg);

	return zmsg_send(&msg,sock);
}

int Snapshot::sendBorder(zsock_t* sock) {
	assert(sock);
	zmsg_t* msg = zmsg_new();

	zmsg_addstr(msg,"$boss");

	return zmsg_send(&msg,sock);
}

bool Snapshot::is(zmsg_t* msg) {
	assert( msg );
	zframe_t* fr = zmsg_first(msg);
	if( fr ) {
		return zframe_streq(fr,"$ss");
	} else {
		return false;
	}
}

bool Snapshot::isBorder(zmsg_t* msg) {
	assert( msg );
	zframe_t* fr = zmsg_first(msg);
	if( fr ) {
		return zframe_streq(fr,"$boss");
	} else {
		return false;
	}
}

std::shared_ptr<Snapshot> Snapshot::parse(zmsg_t* msg) {
	assert( msg );
	std::shared_ptr<Snapshot> ss;
	char* magic = nullptr;
	char* str = nullptr;

	do {
		magic = zmsg_popstr(msg);
		if( nullptr == magic )
			break;
		if( 0 != strcmp(magic,"$ss") )
			break;

		str = zmsg_popstr(msg); // count of partition
		if( nullptr == str )
			break;

		ss = std::shared_ptr<Snapshot>(new Snapshot());
		if( -1 == ss->parseValues(msg) ) {
			ss.reset();
		}
	} while( 0 );

	if( magic )
		free(magic);
	if( str )
		free(str);
	return ss;
}


