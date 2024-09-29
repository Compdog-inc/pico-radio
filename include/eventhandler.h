#ifndef _EVENT_HANDLER_H_
#define _EVENT_HANDLER_H_

#include <stdint.h>
#include <stdlib.h>

constexpr int EVENT_HANDLER_MAX_EVENT_COUNT = 5;

template <typename T>
struct EventHandler
{
    /// @brief Creates a new event handler
    EventHandler()
    {
        this->count = 0;
        this->size = EVENT_HANDLER_MAX_EVENT_COUNT;
    }

    /// @brief Adds an event to the event handler
    /// @param event The event to add
    /// @return Returns the index of the new event
    int Add(T event)
    {
        if (count < size) // if there is space left
        {
            array[count] = event;
            return count++; // return index and add count
        }

        return -1;
    }

    /// @brief Returns the current number of events in the event handler
    int Count()
    {
        return count;
    }

    /// @brief Finds an event in the event handler
    /// @param event The event to match with
    /// @return Returns the index of the event or -1 if it does not exist
    int Find(T event)
    {
        for (int i = 0; i < count; i++)
        {
            if (array[i] == event)
                return i;
        }
        return -1;
    }

    /// @brief Removes an event from the event handler
    /// @param event The event to remove
    /// @return Returns the event if it was sucessfully removed or null if it failed
    T Remove(T event)
    {
        int index = Find(event);
        if (index < 0)
        {
            return nullptr;
        }

        return RemoveAt(index);
    }

    /// @brief Removes an event from the event handler at an index
    /// @param index The index at which the event to remove is located
    /// @return Returns the event if it was successfully removed or null if it failed
    T RemoveAt(int index)
    {
        if (count > 0)
        {
            T event = array[index];
            count--;
            // shift all events after index by one
            for (int i = index; i < count; i++)
            {
                array[i] = array[i + 1];
            }
            return event;
        }
        return nullptr;
    }

    /// @brief Returns an event at an index
    T &Get(int index)
    {
        return array[index];
    }

private:
    int count;
    int size;
    T array[EVENT_HANDLER_MAX_EVENT_COUNT];
};

#endif