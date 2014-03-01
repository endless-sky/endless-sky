/* Callback.h
Michael Zahniser, 28 Feb 2014

Object that calls OnCallback(int value) on whatever object it is given,
regardless of what type of object it is.
*/

#ifndef CALLBACK_H_
#define CALLBACK_H_

#include <memory>



class Callback {
public:
	Callback() = default;
	
	template<class T>
	Callback(T &object);
	
	void operator()(int value = 0);
	
	
public:
	class Function {
	public:
		virtual void operator()(int value) = 0;
	};
	
	template <class T>
	class TFunction : public Function {
	public:
		TFunction(T &object);
		
		virtual void operator()(int value);
		
	private:
		T &object;
	};
	
	
private:
	std::shared_ptr<Function> fun;
};



// Construct a callback from any object.
template<class T>
Callback::Callback(T &object)
	: fun(new TFunction<T>(object))
{
}



inline void Callback::operator()(int value)
{
	if(fun)
		(*fun)(value);
}



template <class T>
Callback::TFunction<T>::TFunction(T &object)
	: object(object)
{
}


	
template <class T>
void Callback::TFunction<T>::operator()(int value)
{
	object.OnCallback(value);
}



#endif
