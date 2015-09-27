#include <algorithm>
#include <glog/logging.h>
#include "buscli/busmsgreply.h"


BusMsgReply::BusMsgReply(uint64_t msg_id,const std::set<std::string>& payloads) :
	m_msg_id(msg_id),
	m_expects(payloads),
	m_count(0)
{
}

BusMsgReply::~BusMsgReply() {
	while( !m_replys.empty() ) {
		zmsg_destroy(&(m_replys.front().msg));
		m_replys.pop_front();
	}
}

zmsg_t* BusMsgReply::reply(const std::string& payload) const {
	if( m_fails.find(payload) != m_fails.end() ) {
		return nullptr;
	}
	for(auto it=m_replys.begin(); it != m_replys.end(); ++it) {
		if( it->payloads.find(payload) != it->payloads.end() ) {
			return it->msg;
		}
	}
	return nullptr;
}

void BusMsgReply::addReply(const bus::ReplyMessage& reply,zmsg_t** p_msg) {
	CHECK_NOTNULL(p_msg);
	if( nullptr == *p_msg || reply.msg_id() != m_msg_id )
		return;
	
	BusMsgReplyItem item;
	for(auto it=reply.payloads().begin(); it != reply.payloads().end(); ++it) {
		if( m_expects.find(*it) != m_expects.end() ) {
			item.payloads.insert(*it);
		}
	}

	if( !item.payloads.empty() ) {
		m_count += item.payloads.size();
		item.msg = *p_msg;
		*p_msg = nullptr;
		m_replys.push_back(std::move(item));
	}
}

void BusMsgReply::addFailure(const bus::Failure& fails) {
	if( fails.msg_id() != m_msg_id )
		return;
	for(auto it=fails.payloads().begin(); it != fails.payloads().end(); ++it) {
		if( m_expects.find(*it) != m_expects.end() ) {
			m_fails.insert(*it);
			++m_count;
		}
	}
}

