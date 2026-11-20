#ifndef RWS_CCLASSFACTORY_H
#define RWS_CCLASSFACTORY_H

// ============================================================================
// CannonCruise - RenderWare Studio Framework (cclassfactory.h)
// Original path: D:\Projects\CannonCruisePC\code\framework\core\attributehandler\cclassfactory.h
// Reverse-engineered and rewritten by analyzing with Ghidra and CC.asm.
// Binary ref: FUN_00403000 - FUN_004032e0 (RWS::CClassFactory)
// ============================================================================

#include <rwcore.h>
#include <map>
#include <string>

namespace RWS
{
    // Forward declarations
    class CAttributePacket;

    /**
     * @typedef ClassCreationFunc
     * @brief Factory function pointer used to create class instances.
     *
     * Registered via `CClassFactory::Register()`; called when `Create()` is invoked.
     */
    typedef void* (*ClassCreationFunc)(void);

    /**
     * @typedef ClassPacketCreationFunc
     * @brief Function pointer to create a class instance with an attribute packet.
     */
    typedef void* (*ClassPacketCreationFunc)(const CAttributePacket& packet);

    /**
     * @struct ClassData
     * @brief Structure holding the factory data of a registered class.
     */
    struct ClassData
    {
        ClassCreationFunc m_pCreationFunc; ///< Object creation function
        RwUInt32          m_Size;          ///< Class size (bytes)
        ClassPacketCreationFunc m_pPacketCreationFunc; ///< Creation from an attribute packet (optional)
    };

    /**
     * @class CClassFactory
     * @brief RenderWare Studio object factory — creates objects by class name.
     *
     * When `CAttributeHandler::ProcessPacket()` receives an object creation packet,
     * `CClassFactory::Create(pClassName)` is called to create the appropriate class instance.
     * All game classes register with this factory via the `RWS_REGISTER_CLASS` macro.
     *
     * **Usage:**
     * ```cpp
     * // Inside the class definition:
     * RWS_DECLARE_CLASS(CShip)
     * RWS_MAKENEWCLASS(CShip)
     *
     * // In the CPP file:
     * RWS_REGISTER_CLASS(CShip)
     * ```
     *
     * **Binary references:**
     * - Create: `FUN_00403000`
     * - Register: `FUN_00403080`
     * - ClassIsRegistered: `FUN_004030f0`
     */
    class CClassFactory
    {
    public:
        enum
        {
            m_AlignmentSize = 64 ///< Object memory alignment (bytes)
        };

        /// @return Pointer to the singleton CClassFactory instance
        static CClassFactory* Instance();
        /// @brief Deletes the CClassFactory instance
        static void Delete();
        /// @brief Shuts down the factory and clears all registered classes
        static void Shutdown();

        /**
         * @brief Creates a new object by class name.
         * @param pClassName Name of the class to create (e.g. "CShip").
         * @return void* — Pointer to the created object (must be cast to the type).
         */
        static void* Create(const char* pClassName);
        /// Creates from the object's attribute packet when the class has a
        /// packet factory; otherwise falls back to Create(pClassName).
        static void* Create(const char* pClassName, const CAttributePacket& packet);
        /// Adds/replaces the packet factory of a class (registers it if new).
        static void RegisterPacketFactory(const char* pClassName,
                                          ClassPacketCreationFunc pCreateFunc,
                                          RwUInt32 size = 0);

        /**
         * @brief Checks whether the given class name is registered with the factory.
         * @param pClassName Class name to check.
         * @return TRUE if the class is registered.
         */
        static RwBool ClassIsRegistered(const char* pClassName);

        /**
         * @brief Registers a new class with the factory.
         * @param pClassName   Class name to register.
         * @param pCreateFunc  Object creation function.
         * @param size         Class size (bytes, optional).
         * @param flags        Registration flags (optional).
         * @return void* — Pointer to the registration record (usually a non-NULL sentinel).
         */
        static void* Register(const char* pClassName, ClassCreationFunc pCreateFunc, RwUInt32 size = 0, RwUInt32 flags = 0);

        /**
         * @brief Removes the given class's registration from the factory.
         * @param pClassName Class name whose registration should be removed.
         */
        static void UnRegister(const char* pClassName);

        /// @brief Removes all registered classes from the factory
        static void UnRegisterAll();

    private:
        CClassFactory();
        ~CClassFactory();

        static CClassFactory* sm_pInstance; ///< Singleton instance pointer

        std::map<std::string, ClassData*> m_RegisteredClasses; ///< Name → ClassData mapping table
    };

    /**
     * @class CAutoRegisterClass
     * @brief Helper that automatically registers the class with CClassFactory during static initialization.
     *
     * The `RWS_REGISTER_CLASS` macro creates a static instance of this class,
     * ensuring the class registers with the factory when the `.cpp` file is loaded.
     */
    class CAutoRegisterClass
    {
    public:
        /**
         * @brief Registers the class with CClassFactory.
         * @param pClassName  Class name to register.
         * @param pCreateFunc Object creation function.
         * @param size        Class size (optional).
         * @param flags       Registration flags (default: 1).
         */
        CAutoRegisterClass(const char* pClassName, ClassCreationFunc pCreateFunc, RwUInt32 size = 0, RwUInt32 flags = 1)
        {
            CClassFactory::Register(pClassName, pCreateFunc, size, flags);
        }
    };

    /**
     * @brief Empty factory function — create=NULL pattern for 1-arg registrations.
     * MSVC's traditional preprocessor mangles lambda syntax inside macros
     * (C2059: '['), so a fixed function is used instead of a lambda.
     */
    inline void* NullCreationFunc() { return 0; }

    /// Static registration of a class's packet factory (className::MakeNewClass,
    /// see RWS_MAKENEWCLASS).
    class CAutoRegisterPacketClass
    {
    public:
        CAutoRegisterPacketClass(const char* pClassName,
                                 ClassPacketCreationFunc pCreateFunc,
                                 RwUInt32 size)
        {
            CClassFactory::RegisterPacketFactory(pClassName, pCreateFunc, size);
        }
    };

    /// Registers className so the spawner builds it from its attribute packet.
    /// Requires RWS_MAKENEWCLASS(className) in the class.
    #define RWS_REGISTER_PACKET_CLASS(className)                               \
        static RWS::CAutoRegisterPacketClass s_autoRegisterPacket_##className( \
            #className, &className::MakeNewClass, sizeof(className))

    /**
     * @def RWS_REGISTER_CLASS(className[, factory[, size]])
     * @brief Macro that registers the class with CClassFactory via static initialization.
     *
     * Usage (codegen):
     * ```cpp
     * RWS_REGISTER_CLASS(CShip)  // name registration — create is supplied later by BehaviorLoader
     * RWS_REGISTER_CLASS(CShip, MakeCShip, sizeof(CShip))
     * ```
     * The 3-arg form actually registers the factory function (calls Create);
     * the 1-arg form is the binary name-registration pattern (create NULL).
     *
     * NOTE: MSVC lambda syntax breaks inside macros (C2059/C2512).
     * Use a static factory function or NullCreationFunc instead of a lambda.
     */
    #define RWS_GET_MACRO(_1, _2, _3, NAME, ...) NAME
    #define RWS_REGISTER_CLASS1(className)                                   \
        static RWS::CAutoRegisterClass s_autoRegister_##className(           \
            #className,                                                      \
            &RWS::NullCreationFunc,                                          \
            sizeof(className))
    #define RWS_REGISTER_CLASS2(className, factory)                          \
        static RWS::CAutoRegisterClass s_autoRegister_##className(           \
            #className, factory, sizeof(className))
    #define RWS_REGISTER_CLASS3(className, factory, size)                    \
        static RWS::CAutoRegisterClass s_autoRegister_##className(           \
            #className, factory, (RwUInt32)(size))
    #define RWS_REGISTER_CLASS(...)                                           \
        RWS_GET_MACRO(__VA_ARGS__, RWS_REGISTER_CLASS3,                      \
                      RWS_REGISTER_CLASS2, RWS_REGISTER_CLASS1)(__VA_ARGS__)

    /**
     * @def RWS_MAKENEWCLASS(className)
     * @brief Defines a static factory method that creates an object from an attribute packet.
     */
    #define RWS_MAKENEWCLASS(className) \
        static void* MakeNewClass(const RWS::CAttributePacket& packet) { return new className(packet); }

    /**
     * @def RWS_DECLARE_CLASS(className)
     * @brief Defines the `GetClassName()` method that returns the class name.
     */
    #define RWS_DECLARE_CLASS(className) \
        static const char* GetClassName() { return #className; }

    /**
     * @def RWS_CLASS_INDEX(className)
     * @brief Selects the class block in the attribute packet (by class name).
     */
    #define RWS_CLASS_INDEX(className) #className
}

#endif // RWS_CCLASSFACTORY_H

