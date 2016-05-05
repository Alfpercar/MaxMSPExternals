#ifndef INCLUDED_CHANGELISTENERBINDER_HXX
#define INCLUDED_CHANGELISTENERBINDER_HXX

#include <cstddef>

#include "juce.h"

// Simple utility class that allows a class to have multiple listener objects which 
// can be bound to different callback functions. This may offer a nicer alternative 
// to deriving a class from ChangeListener and having one big 
// changeListenerCallback(void *source) function which switches on the source argument.
//
// Example:
// class A
// {
// public:
//     ChangeBroadcaster updateFooSignal;
//
// public:
//     void someFunction() { updateFooSignal.sendChangeMessage(this); }
// };
//
// class B
// {
// public:
//     ChangeListenerBinder<B> updateFooSlot;
//
// public:
//     B(A &a)
//     {
//         updateFooSlot.bind(this, &B::updateFoo);
//         connect(a.updateFooSignal, updateFooSlot);
//     }
//
//     ~B()
//     {
//         disconnect(a.updateFooSignal, updateFooSlot);
//     }
//
// private:
//     void updateFoo(void *)
//     {
//         // ...
//     }
// };
template<typename CallbackClassT>
class ChangeListenerBinder : public ChangeListener
{
public:
	typedef void (CallbackClassT::*CallbackFunPtr)(void *objectThatHasChanged);

public:
	ChangeListenerBinder()
	{
		callbackClassInstance_ = NULL;
		callbackFunPtr_ = NULL;
		broadcaster_ = NULL;
	}

	~ChangeListenerBinder()
	{
		unbindAndDisconnect();
	}

	// binds or re-binds a callback function to this change listener
	void bind(CallbackClassT *callbackClassInstance, CallbackFunPtr callbackFunPtr)
	{
		callbackClassInstance_ = callbackClassInstance;
		callbackFunPtr_ = callbackFunPtr;
	}

	// unbinds the callback function from this listener (for further changes no callback 
	// is called); not needed for proper object destruction
	void unbind()
	{
		callbackClassInstance_ = NULL;
		callbackFunPtr_ = NULL;
	}

	// bind to member function and connect to broadcaster
	void bindAndConnect(ChangeBroadcaster &broadcaster, CallbackClassT *callbackClassInstance, CallbackFunPtr callbackFunPtr)
	{
		bind(callbackClassInstance, callbackFunPtr);
		broadcaster.addChangeListener(this); // internally locks
		broadcaster_ = &broadcaster;
	}

	// unbind to member function and disconnect from broadcaster
	void unbindAndDisconnect()
	{
		if (broadcaster_ == NULL)
			return;

		broadcaster_->removeChangeListener(this); // internally locks
		unbind();
	}

	// overridden ChangeListener method
	void changeListenerCallback(void *objectThatHasChanged)
	{
		if (callbackClassInstance_ != NULL && callbackFunPtr_ != NULL)
		{
			(callbackClassInstance_->*callbackFunPtr_)(objectThatHasChanged);
		}
	}

private:
	CallbackClassT *callbackClassInstance_;
	CallbackFunPtr callbackFunPtr_;
	ChangeBroadcaster *broadcaster_;
};

#endif // INCLUDED_CHANGELISTENERBINDER_HXX