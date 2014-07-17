/**This file is part of Lambdatask, released under the terms of the Unlicense.
See LICENSE in the root of the Lambdatask repository for details.*/
#include <mutex>
#include <memory>
#include <utility>

namespace lambdatask {
/**\brief A monitor, implementing the monitor pattern.

Specifically, this has a `shared_ptr`-like interface: if `T` is a type, then `Monitor<t> foo;` is a monitor over a `T`.

Access to the monitor `foo` is done as follows: `foo->bar()`.  Only one thread may be in the monitor at a time.  If another thread were to call `foo->qux()` while `foo->bar()` is still executing, it will be blocked until `foo->bar()` finishes.

This class does not support `operator*`, merely `operator->`.  Furthermore, monitors are not copyable.

The monitor owns the lifetime of an instance of `T` inside it.  It is not possible to make the monitor take ownership of another instance of `T`: the purpose of monitors is to moderate access to a shared resource, not act as a smart pointer.
Furthermore, the lifetime of the contained `T` ends when the monitor's lifetime ends; the internal instance of `T` is freed by the destructor.  What this translates to in English is that you can only construct a monitor and an object of the contained type at the same time and that you shouldn't do "tricky" things like also access said object through a raw pointer.

The constraints on `T` are as follows:

- `T` must be a class type.  Do not use pointers or classes that overload `operator->`.  Neither of these will work as expected.

- If Monitor<t> is default-constructed, then `T` must also be default-constructible.
*/
template <class T>
class Monitor {
	public:
	Monitor() {
		instance = new T();
	}

	~Monitor() {
		auto guard = std::lock_guard<std::mutex>(lock);  //It should not be possible for a monitor to be concurrently destructed and accessed, but let's provide some safety anyway.
		if(moved) return;
		delete instance;
	}

	Monitor(Monitor<T> &other) = delete; //no copying.
	Monitor(Monitor<T> &&other) {
		auto guard = std::lock_guard<std::mutex>(other);//we want to have exclusive access.  This may be unnecessary, but adds an extra layer of safety.
		instance = other.instance;
		other.moved = true;
	}

	Monitor& operator=(const Monitor&) = delete; //no copying.

	//a helper type that represents a locked monitor.
	class LockedMonitor {
		public:
		//we need to make it impossible to store this anywhere no matter what.
		//Also, impossible to make one outside a monitor.
		//disable copying.
		LockedMonitor(const LockedMonitor& other) = delete;
		LockedMonitor& operator=(const LockedMonitor& other) = delete;
		//and disable moving.
		LockedMonitor(LockedMonitor&& other) = delete;	
		LockedMonitor& operator=(const LockedMonitor&& other) = delete;
		//also not default constructible.
		LockedMonitor() = delete;

		//the only way for the constructed instance to be constructed is to hold the lock.
		T* operator->() {
			return managed_ptr;
		}

		private:
		//it's enough just to construct these: the lock_guard will hold the lock for us.
		LockedMonitor(T* m, std::mutex& lock): managed_ptr(m), guard(lock) { }
		std::unique_lock<std::mutex>  guard;
		T* managed_ptr;
		friend Monitor<T>;
	};

	LockedMonitor&& operator->() {
		return std::move(LockedMonitor(instance, lock));
	}

	private:
	T* instance;
	std::mutex lock;
	bool moved  = false;
};

}