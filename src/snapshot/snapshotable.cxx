#include "czmq.h"
#include "ris/snapshot/snapshotable.h"


/************************
 *
 ***********************/

SnapshotItem::SnapshotItem(const std::string& id) :
	m_msg( zmsg_new() )
{
	zmsg_addstr(m_msg,"$ssit");
	zmsg_addstr(m_msg,id.c_str());
}

SnapshotItem::~SnapshotItem() {
	if( m_msg ) {
		zmsg_destroy(&m_msg);
	}
}

int SnapshotItem::addData(const void* src,size_t size) {
	assert(m_msg);
	return zmsg_addmem(m_msg,src,size);
}

int SnapshotItem::addString(const char* str) {
	assert(m_msg);
	return zmsg_addstr(m_msg,str);
}

int SnapshotItem::send(zsock_t* sock) {
	if( m_msg ) {
		return zmsg_send(&m_msg,sock);
	} else {
		return -1;
	}
}


/************************
 *
 ***********************/

SnapshotPartition::SnapshotPartition(const std::string& id,uint32_t version) :
	m_info(zmsg_new())
{
	zmsg_addstr(m_info,id.c_str());
	char ver[32];
	snprintf(ver,sizeof(ver),"%u",version);
	zmsg_addstr(m_info,ver);
}

SnapshotPartition::~SnapshotPartition() {
	if( m_info ) {
		zmsg_destroy(&m_info);
	}
	m_items.clear();
}

int SnapshotPartition::addData(const void* src,size_t size) {
	assert( m_info );
	return zmsg_addmem(m_info,src,size);
}

int SnapshotPartition::addString(const char* str) {
	assert( m_info );
	return zmsg_addstr(m_info,str);
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

int SnapshotPartition::sendHeader(zsock_t* sock) {
	assert(sock);
	zmsg_t* msg = zmsg_new();

	zmsg_addstr(msg,"$sspth");
	char count[32];
	snprintf(count,sizeof(count),"%u",(uint32_t)m_items.size());
	zmsg_addstr(msg,count);

	zmsg_t* info = zmsg_dup(m_info);
	zmsg_addmsg(msg,&info);

	return zmsg_send(&msg,sock);
}

int SnapshotPartition::sendBorder(zsock_t* sock) {
	assert(sock);
	zmsg_t* msg = zmsg_new();

	zmsg_addstr(msg,"$ssptb");
	zmsg_t* info = zmsg_dup(m_info);
	zmsg_addmsg(msg,&info);

	return zmsg_send(&msg,sock);
}

/************************
 *
 ***********************/
Snapshot::Snapshot() :
	m_info( nullptr )
{
}

Snapshot::~Snapshot() {
	if( m_info ) {
		zmsg_destroy(&m_info);
	}
}


int Snapshot::addData(const void* src,size_t size) {
	if( m_info == nullptr ) {
		m_info = zmsg_new();
	}
	return zmsg_addmem(m_info,src,size);
}

int Snapshot::addString(const char* str) {
	if( m_info == nullptr ) {
		m_info = zmsg_new();
	}
	return zmsg_addstr(m_info,str);
}

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

int Snapshot::sendHeader(zsock_t* sock) {
	assert(sock);
	zmsg_t* msg = zmsg_new();

	zmsg_addstr(msg,"$ssh");
	char count[32];
	snprintf(count,sizeof(count),"%u",(uint32_t)m_partitions.size());
	zmsg_addstr(msg,count);

	if( m_info ) {
		zmsg_t* info = zmsg_dup(m_info);
		zmsg_addmsg(msg,&info);
	}

	return zmsg_send(&msg,sock);
}

int Snapshot::sendBorder(zsock_t* sock) {
	assert(sock);
	zmsg_t* msg = zmsg_new();

	zmsg_addstr(msg,"$ssb");

	if( m_info ) {
		zmsg_t* info = zmsg_dup(m_info);
		zmsg_addmsg(msg,&info);
	}

	return zmsg_send(&msg,sock);
}




