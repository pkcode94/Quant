#pragma once

#include <set>
#include <stdexcept>

// Generic ID allocator with lock/unlock semantics.
//
// - acquire()  : finds the lowest free ID, marks it as locked, returns it.
// - commit(id) : moves a locked ID into the permanent used set.
// - release(id): frees a used or locked ID entirely.
// - unlock(id) : releases a lock without committing (cancellation).
//
// IDs start at 1.  Callers are responsible for external synchronisation.

class IdGenerator
{
public:
    IdGenerator() = default;

    // Seed the generator with IDs already in use (e.g. loaded from disk).
    void seed(const std::set<int>& existingIds)
    {
        m_used = existingIds;
    }

    void seedSingle(int id)
    {
        if (id > 0) m_used.insert(id);
    }

    // Find the lowest available ID, lock it, and return it.
    int acquire()
    {
        int id = 1;
        while (m_used.count(id) || m_locked.count(id))
            ++id;
        m_locked.insert(id);
        return id;
    }

    // Commit a previously locked ID into permanent use.
    void commit(int id)
    {
        m_locked.erase(id);
        m_used.insert(id);
    }

    // Release an ID (whether used or locked) so it can be reused.
    void release(int id)
    {
        m_used.erase(id);
        m_locked.erase(id);
    }

    // Cancel a lock without committing.
    void unlock(int id)
    {
        m_locked.erase(id);
    }

    bool isAvailable(int id) const
    {
        return !m_used.count(id) && !m_locked.count(id);
    }

    bool isUsed(int id)    const { return m_used.count(id) > 0; }
    bool isLocked(int id)  const { return m_locked.count(id) > 0; }

    const std::set<int>& usedIds()   const { return m_used; }
    const std::set<int>& lockedIds() const { return m_locked; }

private:
    std::set<int> m_used;
    std::set<int> m_locked;
};
