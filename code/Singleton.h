#ifndef SINGLETON_H
#define SINGLETON_H

// ============================================================================
// CannonCruise - Engine Singleton Template (Singleton.h)
// Original path: D:\Projects\CannonCruisePC\code\Singleton.h
// Reverse-engineered via Ghidra and CC.asm analysis.
// ============================================================================

#include <cassert>

/**
 * @class CSingleton
 * @brief CRTP template class implementing the singleton design pattern.
 *
 * It is the standard Singleton implementation of the RenderWare Studio framework.
 * Manager classes such as CAudioManager, CSceneManager, CSpeakManager
 * derive from this template to guarantee a single instance.
 *
 * **Usage example:**
 * ```cpp
 * class CAudioManager : public CSingleton<CAudioManager> { ... };
 *
 * // Access:
 * CAudioManager::GetSingleton()->PlaySound("hit.wav");
 * ```
 *
 * **Garantiler:**
 * - Ctor: If a second instance is created, an assert is triggered.
 * - Dtor: If destruction is attempted on an instance that was already destroyed, an assert is triggered.
 * - `GetSingleton()`: If no instance exists, an assert is triggered.
 * - `GetSingletonPtr()`: Returns nullptr if no instance exists (no assert).
 *
 * @tparam T The class type to be made a singleton (CRTP pattern).
 */
template <typename T>
class CSingleton
{
public:
    /**
     * @brief Constructor — sets the static instance pointer to `this`.
     * @pre No instance of this type must have been created yet.
     */
    CSingleton()
    {
        assert(!ms_pSingleton && "!ms_pSingleton Failed");
        ms_pSingleton = static_cast<T*>(this);
    }

    /**
     * @brief Destructor — resets the static instance pointer.
     * @pre ms_pSingleton must be valid.
     */
    virtual ~CSingleton()
    {
        assert(ms_pSingleton && "ms_pSingleton Failed");
        ms_pSingleton = 0;
    }

    /**
     * @brief Returns a reference to the current singleton instance.
     * @return T* — Pointer to the singleton instance.
     * @pre The instance must have been created previously.
     */
    static T* GetSingleton()
    {
        assert(ms_pSingleton != 0 && "ms_pSingleton != NULL Failed");
        return ms_pSingleton;
    }

    /**
     * @brief Returns a pointer to the current singleton instance; does not assert.
     * @return T* — Pointer to the singleton instance, or nullptr (if no instance).
     */
    static T* GetSingletonPtr()
    {
        return ms_pSingleton;
    }

protected:
    static T* ms_pSingleton; ///< Pointer to the singleton instance (one per class)

private:
    CSingleton(const CSingleton&);
    CSingleton& operator=(const CSingleton&);
};

template <typename T>
T* CSingleton<T>::ms_pSingleton = 0;

#endif // SINGLETON_H
