#include <powercores/utilities.hpp>
#include <atomic>

namespace powercores {

long long getThreadId() {
	//This relies on "magic statics", the threadsafe initialization of statics, ensured by the compiler.
	thread_local long long id = 0; //0 means no id.
	static std::atomic<long long> globallyUniqueId{1};
	if(id == 0) id = globallyUniqueId.fetch_add(1, std::memory_order_relaxed);
	return id;
}

}