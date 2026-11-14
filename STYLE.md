# CannonCruise — Code Style Guide

> Phase 5.4 · Documents the existing conventions of the source code.
> New code follows this guide; where it conflicts with old code, this guide prevails.

## 1. Basic rules

- **Standard:** C++14 (fixed in `CMakeLists.txt`).
- **Indentation:** Use spaces, **no tabs**. 4 spaces preferred
  (framework code uses 4, some game classes use 2 — as long as in-file consistency
  is not broken, the existing indentation is kept).
- **Source files:** UTF-8, LF termination.
- **Line endings:** Same as the rest of the file (existing files are LF).

## 2. File header

Every `.cpp`/`.h` file opens with the standard block:

```cpp
// ============================================================================
// CannonCruise - <Section> (<file name>)
// Original path: D:\Projects\CannonCruisePC\code\...
// Rewritten by reverse engineering with Ghidra and CC.asm analysis.
//
// Binary Functions:
//   FUN_0040b090 — CEventHandler vtable setup
// ============================================================================
```

## 3. Naming

| Item | Rule | Example |
|---|---|---|
| Class | `C` prefix, PascalCase | `CShip`, `CPhysicsWorld` |
| Namespace (framework) | `RWS` | `RWS::CEventHandler` |
| Member variable | `m_` prefix + PascalCase; `p` for pointers | `m_pClump`, `m_bInWorld` |
| Static member | `sm_` prefix | `sm_pInstance` |
| Global/static local | `s_` prefix | `s_RegisteredEvents` |
| Parameter | `p` prefix (pointer) + camelCase | `pClump`, `deltaTime` |
| Local variable | camelCase | `numVertices` |
| Constant / macro | `rw`/`iMsg` prefix, mixed ALL_CAPS | `iMsgRunningTick`, `rwID_CLUMP` |
| Enum | `E` prefix + PascalCase | `ECannonballType` |
| Function | PascalCase (C APIs `RwXxxYyy`) | `HandleEvents`, `RpClumpRender` |

## 4. Style

- **Braces:** Opening brace at the end of the definition line (K&R) —
  the dominant style in game classes:

  ```cpp
  void CShip::Initialize(RpClump *pClump) {
      assert(m_pClump == 0 && "Ship already has a clump");
  }
  ```

  Allman can be seen in framework files; new code uses K&R.
- **Pointer/reference star:** Adjacent to the variable name (`RpClump *pClump`,
  `const CMsg &msg`) — this placement is common in the current code.
- **Blank line:** Section separator between functions
  (`// ---------...`); explanatory Turkish/English comments are free-form.
- **`//` comments** for bodies, **`/** */`** for the API in header files.

## 5. Documentation (Doxygen)

In header files, every class/member is documented with `@brief`:

```cpp
/**
 * @brief Advances the physics simulation by deltaTime seconds.
 * @param deltaTime  Time step (the original game uses a fixed 1/60 s).
 */
void Update(RwReal deltaTime);
```

Generation: `doxygen Doxyfile` → `docs/html/`.

## 6. Reverse engineering evidence

- Binary function addresses (`FUN_0040b090`) and ASM references
  (`00419e72 CALL ...`) are **preserved** in code comments — never deleted.
- Original assert string quotes stay in English:
  `"pRigidBody != NULL Failed"`.
- Unknown behavior is marked with `// TODO(Phase N+): ...`.

## 7. Pointers and null values

- Null pointer: `0` (C++98 compatible; although `nullptr` exists in C++14,
  the current code uses `0`).
- `RwBool` returns: `TRUE` / `FALSE` macros.
- Opaque Havok/RW types: `void*` + a type comment in the header.

## 8. Build / verification

After adding new code:

```bash
cd code
cmake --build build -j8          # static library + link verification
./build/CannonCruise             # "link verification OK" expected
```

- The build must be error-free; undefined symbols are caught at link time with `-force_load`.
- Tests: the independent tests under `tests/` (see README).

## 9. Prohibitions

- Tab character.
- `#ifdef _WIN32` clutter in source code — none unless platform separation is required.
- Deleting comments that contain binary evidence.
- Adding `#include <real-sdk-header>` before the real SDK version is verified
  (local stub headers must not be shadowed).
