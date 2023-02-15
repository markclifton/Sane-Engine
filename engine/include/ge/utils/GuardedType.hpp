#pragma once

#include <ge/utils/Mutex.hpp>

namespace GE
{
	namespace Utils
	{
		template <typename T>
		class Guarded
		{
		public:
			Guarded() {}
			~Guarded() {}

			Guarded(T initialValue)
			{
				_mutex = std::make_shared<std::mutex>();
				Update(initialValue);
			}

			void Update(T newValue) {
				LOCK(*_mutex);
				_value = newValue;
			}

			T Get() {
				LOCK(*_mutex);
				return _value;
			}

		private:
			T _value;
			std::shared_ptr<std::mutex> _mutex;
		};
	}
}