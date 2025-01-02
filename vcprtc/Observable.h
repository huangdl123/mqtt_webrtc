/**
 * This file is part of janus_client project.
 * Author:    Jackie Ou
 * Created:   2020-10-01
 **/

#pragma once

#include <memory>
#include <functional>
#include <algorithm>
#include <vector>

namespace vcprtc {

using namespace std;

class Observable
{
public:
    template<class T> static void addObserver(std::vector<weak_ptr<T>>& observers, weak_ptr<T> obs)
    {
        if (!obs.lock()) return;

        bool found = false;
        for (int i = 0; i < observers.size(); i++)
        {
            if (observers[i].lock() == obs.lock())
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            observers.push_back(obs);
        }
    }

    template<class T> static void removeObserver(std::vector<weak_ptr<T>>& observers, weak_ptr<T> obs)
    {
	    for (int i = 0; i < observers.size(); i++)
	    {
		    auto obs0 = observers[i];
		    if (observers[i].lock() == obs.lock())
		    {
                observers[i].reset();
		    }
	    }
        removeInvalidObserver(observers);
    }

    template<class T> static void removeInvalidObserver(std::vector<std::weak_ptr<T>>& observers)
    {
        observers.erase(std::remove_if(observers.begin(), observers.end(), [](const std::weak_ptr<T>& observer) {
                                            return observer.expired();
                                        }), observers.end());
    }

    template<class T> static void notifyObserver4Change(std::vector<std::weak_ptr<T>>& observers, std::function<void(std::shared_ptr<T>& ot)> func)
    {
        removeInvalidObserver(observers);
        auto copiedObservers = observers;
        for(auto it = copiedObservers.begin(); it != copiedObservers.end(); ++it)
        {
            auto observer = (*it).lock();
            if (observer)
            {
                func(observer);
            }
        }
    }
};

}
