#pragma once
#include <vector>

template <typename T>
class ExtenderInterface
{
public:
	virtual void AllocateBlocks() = 0;
	virtual void DeAllocateBlocks() = 0;
	virtual void OnConstructor(T *object) = 0;
	virtual void OnDestructor(T *object) = 0;
	virtual void Store() = 0;
	virtual void Restore() = 0;
};

template <typename T>
class ExtendersHandler
{
protected:
	struct static_data
	{
		std::vector <ExtenderInterface<T> *> extenders;
		bool injected;
		
		static_data()
		{
			injected = false;
		}
	};

	static inline static_data& get_data()
	{
		static static_data data;
		return data;
	}

	static void Allocate()
	{
		static_data& data = get_data();
		for(auto i = data.extenders.begin(); i != data.extenders.end(); ++i)
			(*i)->AllocateBlocks();
	}
		
	static void DeAllocate()
	{
		static_data& data = get_data();
		for(auto i = data.extenders.begin(); i != data.extenders.end(); ++i)
			(*i)->DeAllocateBlocks();
	}

	static void Constructor(void *object)
	{
		static_data& data = get_data();
		for(auto i = data.extenders.begin(); i != data.extenders.end(); ++i)
			(*i)->OnConstructor((T *)object);
	}

	static void Destructor(void *object)
	{
		static_data& data = get_data();
		for(auto i = data.extenders.begin(); i != data.extenders.end(); ++i)
			(*i)->OnDestructor((T *)object);
	}
	
	static void StoreReplay()
	{
		static_data& data = get_data();
		for(auto i = data.extenders.begin(); i != data.extenders.end(); ++i)
			(*i)->Store();
	}
	
	static void RestoreReplay()
	{
		static_data& data = get_data();
		for(auto i = data.extenders.begin(); i != data.extenders.end(); ++i)
			(*i)->Restore();
	}
};