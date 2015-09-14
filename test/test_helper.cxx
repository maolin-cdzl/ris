#include <iostream>
#include <uuid/uuid.h>
#include <gtest/gtest.h>
#include "test_helper.h"

std::string newUUID() {
    uuid_t uuid;
    uuid_generate_random ( uuid );
    char s[37];
    uuid_unparse( uuid, s );
    return s;
}

//class LoopTimeoutStopper
LoopTimeoutStopper::LoopTimeoutStopper(zloop_t* loop,long timeout) :
	m_loop(loop),
	m_tid(-1)
{
	m_tid = zloop_timer(loop,timeout,1,&LoopTimeoutStopper::onTimer,this);
}

LoopTimeoutStopper::~LoopTimeoutStopper() {
	cancel();
}

void LoopTimeoutStopper::cancel() {
	if( m_tid != -1 ) {
		zloop_timer_end(m_loop,m_tid);
		m_tid = -1;
	}
}

int LoopTimeoutStopper::onTimer(zloop_t* loop,int timeid,void* arg) {
	(void)loop;
	(void)timeid;
	LoopTimeoutStopper* self = (LoopTimeoutStopper*)arg;
	zloop_timer_end(loop,self->m_tid);
	self->m_tid = -1;
	return -1;
}

// class LoopCompleteStopper

LoopCompleteStopper::LoopCompleteStopper(zloop_t* loop,long period) :
	m_loop(loop),
	m_result(-1),
	m_running(true)
{
	m_tid = zloop_timer(loop,period,0,&LoopCompleteStopper::onTimer,this);
}

LoopCompleteStopper::~LoopCompleteStopper() {
	cancel();
}

void LoopCompleteStopper::complete(int err) {
	m_result = err;
	m_running = false;
}

int LoopCompleteStopper::onTimer(zloop_t* loop,int timeid,void* arg) {
	(void)loop;
	(void)timeid;
	LoopCompleteStopper* self = (LoopCompleteStopper*)arg;

	if( self->m_running ) {
		return 0;
	} else {
		zloop_timer_end(loop,self->m_tid);
		self->m_tid = -1;
		return -1;
	}
}

void LoopCompleteStopper::cancel() {
	if( m_tid != -1 ) {
		zloop_timer_end(m_loop,m_tid);
		m_tid = -1;
		m_running = false;
	}
}

// class ReadableHelper
ReadableHelper::ReadableHelper(zloop_t* loop) :
	m_loop(loop),
	m_sock(nullptr),
	m_msg(nullptr)
{
}

ReadableHelper::~ReadableHelper() {
	if( m_msg ) {
		zmsg_destroy(&m_msg);
	}
	unregister();
}

int ReadableHelper::register_read(zsock_t* sock) {
	if( m_sock )
		return -1;

	int result = zloop_reader(m_loop,sock,&ReadableHelper::readAndGo,this);
	if( 0 == result ) {
		m_sock = sock;
	}
	return result;
}

int ReadableHelper::register_read_int(zsock_t* sock) {
	if( m_sock )
		return -1;

	int result = zloop_reader(m_loop,sock,&ReadableHelper::readAndInterrupt,this);
	if( 0 == result ) {
		m_sock = sock;
	}
	return result;
}

int ReadableHelper::unregister() {
	if( m_sock ) {
		zloop_reader_end(m_loop,m_sock);
		m_sock = nullptr;
		return 0;
	} else {
		return -1;
	}
}

int ReadableHelper::readAndGo(zloop_t* loop,zsock_t* reader,void* arg) {
	(void)loop;
	ReadableHelper* self = (ReadableHelper*)arg; 
	if( self->m_msg ) {
		zmsg_destroy(&self->m_msg);
	}

	self->m_msg = zmsg_recv(reader);
	return 0;
}

int ReadableHelper::readAndInterrupt(zloop_t* loop,zsock_t* reader,void* arg) {
	ReadableHelper* self = (ReadableHelper*)arg; 
	if( self->m_msg ) {
		zmsg_destroy(&self->m_msg);
	}

	self->m_msg = zmsg_recv(reader);
	zloop_reader_end(loop,reader);
	self->m_sock = nullptr;
	return -1;
}

// class TaskRunner

TaskRunner::TaskRunner(zloop_t* loop) :
	m_timer(loop),
	m_running(false)
{
}

TaskRunner::~TaskRunner() {
	stop();
}

void TaskRunner::push(const task_t& task) {
	m_tasks.push_back(task);
}

int TaskRunner::start(uint64_t interval) {
	if( m_timer.isActive() ) {
		return -1;
	}
	m_running = true;
	return m_timer.start(interval,0,std::bind<int>(&TaskRunner::onTimer,this));
}

void TaskRunner::stop() {
	m_timer.stop();
	m_tasks.clear();
	m_running = false;
}

int TaskRunner::onTimer() {
	if( m_tasks.empty() ) {
		stop();
		return -1;
	}
	auto task = m_tasks.front();
	m_tasks.pop_front();
	task();
	return 0;
}

// class SnapshotClientRepeater

SnapshotClientRepeater::SnapshotClientRepeater(zloop_t* loop) :
	m_loop(loop),
	m_client(std::make_shared<SnapshotClient>(loop)),
	m_limits(0),
	m_count(0),
	m_success_count(0)
{
}

int SnapshotClientRepeater::start(size_t count,const std::shared_ptr<ISnapshotBuilder>& builder,const std::string& address) {
	m_limits = count;
	m_count = 0;
	m_success_count = 0;
	m_builder = builder;
	m_address = address;
	m_stopper = std::make_shared<LoopCompleteStopper>(m_loop,1000);

	return m_client->start(std::bind(&SnapshotClientRepeater::onComplete,this,std::placeholders::_1),m_builder,m_address);
}

void SnapshotClientRepeater::onComplete(int err) {
	++m_count;
	if( 0 == err ) {
		++m_success_count;
	}
	if( m_count < m_limits ) {
		m_client->start(std::bind(&SnapshotClientRepeater::onComplete,this,std::placeholders::_1),m_builder,m_address);
	} else {
		m_stopper->complete(0);
	}
}


//class SnapshotClientParallelRepeater

SnapshotClientParallelRepeater::SnapshotClientParallelRepeater(zloop_t* loop) :
	m_loop(loop),
	m_limits(0),
	m_count(0),
	m_success_count(0)
{
}

int SnapshotClientParallelRepeater::start(size_t count,const std::shared_ptr<ISnapshotBuilder>& builder,const std::string& address) {
	m_limits = count;
	m_count = 0;
	m_success_count = 0;
	m_builder = builder;
	m_address = address;
	m_clients.clear();

	for(size_t i = 0; i < count; ++i) {
		auto client = std::make_shared<SnapshotClient>(m_loop);
		if( -1 == client->start(std::bind(&SnapshotClientParallelRepeater::onComplete,this,std::placeholders::_1),m_builder,m_address) ) {
			return -1;
		} else {
			m_clients.push_back(client);
		}
	}
	m_stopper = std::make_shared<LoopCompleteStopper>(m_loop,1000);
	return 0;
}

void SnapshotClientParallelRepeater::onComplete(int err) {
	++m_count;
	if( 0 == err ) {
		++m_success_count;
	}
	if( m_count >= m_limits ) {
		m_stopper->complete(0);
	}
}

// class InvokeCardinality

InvokeCardinality::InvokeCardinality(const std::function<size_t()>& min,const std::function<size_t()>& max) :
	m_fn_min(min),
	m_fn_max(max)
{
}

InvokeCardinality::~InvokeCardinality() {
}


int InvokeCardinality::ConservativeLowerBound() const {
	return 0; 
}

int InvokeCardinality::ConservativeUpperBound() const {
	return INT_MAX;
}

// Returns true iff call_count calls will satisfy this cardinality.
bool InvokeCardinality::IsSatisfiedByCallCount(int call_count) const {
	return (m_fn_min() <= (size_t)call_count && (size_t)call_count <= m_fn_max());
}


// Returns true iff call_count calls will saturate this cardinality.
bool InvokeCardinality::IsSaturatedByCallCount(int call_count) const {
	return (size_t)call_count > m_fn_max();
}

// Describes self to an ostream.
void InvokeCardinality::DescribeTo(::std::ostream* os) const {
	const size_t min = m_fn_min();
	const size_t max = m_fn_max();

	if (min == 0) {
		if (max == 0) {
			*os << "never called";
		} else if (max == INT_MAX) {
			*os << "called any number of times";
		} else {
			*os << "called at most " << max;
		}
	} else if (min == max) {
		*os << "called " << min;
	} else if (max == INT_MAX) {
		*os << "called at least " << min;
	} else {
		// 0 < min_ < max_ < INT_MAX
		*os << "called between " << min << " and " << max << " times";
	}
}

size_t InvokeCardinality::Zero() {
	return 0;
}

size_t InvokeCardinality::Unlimited() {
	return INT_MAX;
}

testing::Cardinality InvokeCardinality::makeAtLeast(const std::function<size_t()>& min) {
	return testing::MakeCardinality(new InvokeCardinality(min,std::bind(&InvokeCardinality::Unlimited)));
}

testing::Cardinality InvokeCardinality::makeAtMost(const std::function<size_t()>& max) {
	return testing::MakeCardinality(new InvokeCardinality(std::bind(&InvokeCardinality::Zero),max));
}

testing::Cardinality InvokeCardinality::makeBetween(const std::function<size_t()>& min,const std::function<size_t()>& max) {
	return testing::MakeCardinality(new InvokeCardinality(min,max));
}

