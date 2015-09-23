#include <memory>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <glog/logging.h>
#include <czmq.h>

#include "ribroker/ribrokeractor.h"

zloop_t* g_loop = nullptr;

class RisEnv : public testing::Environment {
public:
	// Override this to define how to set up the environment.
	virtual void SetUp() {
		zsys_init();
		g_loop = zloop_new();
		broker = std::make_shared<RIBrokerActor>();
		broker->start("tcp://127.0.0.1:2015","tcp://127.0.0.1:2016");
	}
	// Override this to define how to tear down the environment.
	virtual void TearDown() {
		zloop_destroy(&g_loop);
		broker.reset();
		zsys_shutdown();
	}
private:
	std::shared_ptr<RIBrokerActor> broker;
};

int main(int argc,char* argv[]) {
	google::InitGoogleLogging(argv[0]);
	FLAGS_alsologtostderr = false;
	FLAGS_stderrthreshold = google::GLOG_FATAL;
	::testing::AddGlobalTestEnvironment(new RisEnv());
	testing::InitGoogleMock(&argc,argv);

	int result = RUN_ALL_TESTS();

	return result;
}

