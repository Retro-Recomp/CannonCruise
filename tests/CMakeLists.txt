# SDK-independent unit tests (Phase 6 / roadmap "test coverage").
# Each test compiles only the framework sources under test - no RenderWare,
# Havok or platform SDK is required.
#
# Run from the code/ build tree:
#   cmake -B build -DCMAKE_BUILD_TYPE=Release
#   cmake --build build -j8
#   ctest --test-dir build --output-on-failure

set(CODE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../code)

# Original game data (Streams/, Sound/ ...) is not in git. Point this at a
# folder that has it; tests that need it are disabled (reported as
# "Not Run") when it is missing, e.g. in CI or a fresh worktree.
set(CANNONCRUISE_DATA_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." CACHE PATH
    "Folder containing the original game data (Streams/, Sound/, ...)")
if(EXISTS "${CANNONCRUISE_DATA_ROOT}/Streams/GLOBAL.RWS")
    set(CANNONCRUISE_HAVE_DATA TRUE)
else()
    set(CANNONCRUISE_HAVE_DATA FALSE)
    message(STATUS "Game data not found under ${CANNONCRUISE_DATA_ROOT}; "
                   "asset tests will be disabled")
endif()

set(SDK_INCLUDE_DIRS
    ${CODE_DIR}
    ${CODE_DIR}/framework
    ${CODE_DIR}/framework/core
    ${CODE_DIR}/framework/toolkits
)

# add_sdk_test(<name> <extra sources...>)
function(add_sdk_test name)
    add_executable(${name} ${ARGN})
    target_include_directories(${name} PRIVATE ${SDK_INCLUDE_DIRS})
    add_test(NAME ${name} COMMAND ${name})
endfunction()

add_sdk_test(havok_memory_manager_test
    ${CMAKE_CURRENT_SOURCE_DIR}/havok_memory_manager_test.cpp
    ${CODE_DIR}/HavokMemoryManager.cpp
)

add_sdk_test(ceventhandler_test
    ${CMAKE_CURRENT_SOURCE_DIR}/ceventhandler_test.cpp
    ${CODE_DIR}/framework/core/eventhandler/ceventhandler.cpp
)

add_sdk_test(cclassfactory_test
    ${CMAKE_CURRENT_SOURCE_DIR}/cclassfactory_test.cpp
    ${CODE_DIR}/framework/core/attributehandler/cclassfactory.cpp
)

add_sdk_test(cattributehandler_test
    ${CMAKE_CURRENT_SOURCE_DIR}/cattributehandler_test.cpp
    ${CODE_DIR}/framework/core/attributehandler/cattributehandler.cpp
    ${CODE_DIR}/framework/core/attributehandler/cclassfactory.cpp
)

add_sdk_test(fontsystem_test
    ${CMAKE_CURRENT_SOURCE_DIR}/fontsystem_test.cpp
    ${CODE_DIR}/FontSystem.cpp
    ${CODE_DIR}/rwstubs.cpp
)

add_sdk_test(renderpipeline_test
    ${CMAKE_CURRENT_SOURCE_DIR}/renderpipeline_test.cpp
    ${CODE_DIR}/RenderPipeline.cpp
)

# Phase 4.6 - Bullet physics backend (instead of Havok); selected binary
# parameters are checked. Original world-update cadence remains unresolved.
add_sdk_test(physics_test
    ${CMAKE_CURRENT_SOURCE_DIR}/physics_test.cpp
    ${CODE_DIR}/PhysicsBackend.cpp
)
# Bullet includes are not propagated to targets - manual
target_include_directories(physics_test PRIVATE ${bullet3_SOURCE_DIR}/src)
target_link_libraries(physics_test PRIVATE
    BulletDynamics BulletCollision LinearMath)

# Phase 4.6h - Havok 2.2.1 shim: binary naming (NO hkp prefix) +
# real implementation on the Bullet engine (hkWorld/hkRigidBody/hkPhantom).
# gravity -9.8 / friction 0.6 / restitution 0.3 (sub_55E6D0) + stepDeltaTime.
add_executable(havokshim_test
    ${CMAKE_CURRENT_SOURCE_DIR}/havokshim_test.cpp
    ${CODE_DIR}/havok/havokimpl.cpp
    ${CODE_DIR}/PhysicsBackend.cpp
)
target_include_directories(havokshim_test PRIVATE
    ${SDK_INCLUDE_DIRS}
    ${CODE_DIR}/havok
    ${bullet3_SOURCE_DIR}/src
)
target_link_libraries(havokshim_test PRIVATE
    BulletDynamics BulletCollision LinearMath
)
add_test(NAME havokshim_test COMMAND havokshim_test)

# Phase 4.6 - entity/trigger flow: CPhysicsEntity accessors + AddToWorld
# singleton registration; CCollisionTrigger phantom + PollOverlaps enter/leave.
add_sdk_test(physics_entity_test
    ${CMAKE_CURRENT_SOURCE_DIR}/physics_entity_test.cpp
    ${CODE_DIR}/PhysicsEntity.cpp
    ${CODE_DIR}/PhysicsWorld.cpp
    ${CODE_DIR}/CollisionTrigger.cpp
    ${CODE_DIR}/PhysicsBackend.cpp
    ${CODE_DIR}/framework/core/eventhandler/ceventhandler.cpp
    ${CODE_DIR}/framework/core/attributehandler/cclassfactory.cpp
    ${CODE_DIR}/rwstubs.cpp
)
target_include_directories(physics_entity_test PRIVATE ${bullet3_SOURCE_DIR}/src)
target_link_libraries(physics_entity_test PRIVATE
    BulletDynamics BulletCollision LinearMath)

# Phase 4.6d - librw stream validation: real Streams/*.RWS asset
# scanned chunk by chunk via the RwStream shim (4 gate signals).
# manual instead of add_sdk_test: test takes the asset path as an argument.
add_executable(rwlibrw_test
    ${CMAKE_CURRENT_SOURCE_DIR}/rwlibrw_test.cpp
    ${CODE_DIR}/rw_librw.cpp
)
target_include_directories(rwlibrw_test PRIVATE ${SDK_INCLUDE_DIRS})
target_link_libraries(rwlibrw_test PRIVATE librw::librw)
add_test(NAME rwlibrw_test COMMAND rwlibrw_test
    ${CANNONCRUISE_DATA_ROOT}/Streams/MAINMENU.RWS)

# Phase 4.6e - rwstudio container parse: asset record/manifest inventory +
# embedded txd payload validation (fixed expectations via hexdump forensics).
add_executable(rwstudio_test
    ${CMAKE_CURRENT_SOURCE_DIR}/rwstudio_test.cpp
    ${CODE_DIR}/rw_librw.cpp
    ${CODE_DIR}/RwStudioStream.cpp
)
target_include_directories(rwstudio_test PRIVATE ${SDK_INCLUDE_DIRS})
target_link_libraries(rwstudio_test PRIVATE librw::librw)
add_test(NAME rwstudio_test COMMAND rwstudio_test
    ${CANNONCRUISE_DATA_ROOT}/Streams/MAINMENU.RWS
    ${CANNONCRUISE_DATA_ROOT}/Streams/GLOBAL.RWS
    ${CANNONCRUISE_DATA_ROOT}/Streams/VIKING01.RWS)

# Phase 4.6h - RenderBehaviourWorld property apply: 0x0704 id->offset
# map (FUN_004c3e40) verified with real MAINMENU obj#0 data.
add_executable(renderworld_test
    ${CMAKE_CURRENT_SOURCE_DIR}/renderworld_test.cpp
    ${CODE_DIR}/RenderBehaviourWorld.cpp
    ${CODE_DIR}/rw_librw.cpp
    ${CODE_DIR}/RwStudioStream.cpp
)
target_include_directories(renderworld_test PRIVATE ${SDK_INCLUDE_DIRS})
target_link_libraries(renderworld_test PRIVATE librw::librw)
add_test(NAME renderworld_test COMMAND renderworld_test
    ${CANNONCRUISE_DATA_ROOT}/Streams/MAINMENU.RWS)

# Phase 4.6i - CWbCameraBehavior property apply: 51-case switch
# (sub_4C3450) verified with real MAINMENU obj#1 data.
add_executable(camerastate_test
    ${CMAKE_CURRENT_SOURCE_DIR}/camerastate_test.cpp
    ${CODE_DIR}/CameraState.cpp
    ${CODE_DIR}/rw_librw.cpp
    ${CODE_DIR}/RwStudioStream.cpp
)
target_include_directories(camerastate_test PRIVATE ${SDK_INCLUDE_DIRS})
target_link_libraries(camerastate_test PRIVATE librw::librw)
add_test(NAME camerastate_test COMMAND camerastate_test
    ${CANNONCRUISE_DATA_ROOT}/Streams/MAINMENU.RWS)

# Phase 4.6j - CAudioStreamEntity property apply: 6-case switch
# (sub_458BD0) verified with real MAINMENU obj#2 data.
add_executable(audiostream_test
    ${CMAKE_CURRENT_SOURCE_DIR}/audiostream_test.cpp
    ${CODE_DIR}/AudioStreamEntity.cpp
    ${CODE_DIR}/rw_librw.cpp
    ${CODE_DIR}/RwStudioStream.cpp
)
target_include_directories(audiostream_test PRIVATE ${SDK_INCLUDE_DIRS})
target_link_libraries(audiostream_test PRIVATE librw::librw)
add_test(NAME audiostream_test COMMAND audiostream_test
    ${CANNONCRUISE_DATA_ROOT}/Streams/MAINMENU.RWS)

# Phase 4.6k - BehaviourLoader create-by-name dispatch: class name ->
# CClassFactory -> create -> apply virtual; render/camera/audio end to end.
add_executable(behaviourloader_test
    ${CMAKE_CURRENT_SOURCE_DIR}/behaviourloader_test.cpp
    ${CODE_DIR}/BehaviourLoader.cpp
    ${CODE_DIR}/RenderBehaviourWorld.cpp
    ${CODE_DIR}/CameraState.cpp
    ${CODE_DIR}/AudioStreamEntity.cpp
    ${CODE_DIR}/rw_librw.cpp
    ${CODE_DIR}/RwStudioStream.cpp
    ${CODE_DIR}/framework/core/attributehandler/cclassfactory.cpp
)
target_include_directories(behaviourloader_test PRIVATE ${SDK_INCLUDE_DIRS})
target_link_libraries(behaviourloader_test PRIVATE librw::librw)
add_test(NAME behaviourloader_test COMMAND behaviourloader_test
    ${CANNONCRUISE_DATA_ROOT}/Streams/MAINMENU.RWS)

# Phase 4.6l - Embedded TXD (d3d8 IMAGE) decode: MAINMENU embedded txd
# (16 textures) decoded and name/format/pixel verified.
add_executable(txddecode_test
    ${CMAKE_CURRENT_SOURCE_DIR}/txddecode_test.cpp
    ${CODE_DIR}/TxdDecode.cpp
    ${CODE_DIR}/rw_librw.cpp
    ${CODE_DIR}/RwStudioStream.cpp
)
target_include_directories(txddecode_test PRIVATE ${SDK_INCLUDE_DIRS})
target_link_libraries(txddecode_test PRIVATE librw::librw)
add_test(NAME txddecode_test COMMAND txddecode_test
    ${CANNONCRUISE_DATA_ROOT}/Streams/MAINMENU.RWS)

# Phase 4.6m - Static world collision (.col / STATICWORLDCOLLISION): FUN_0043bfa0
# layout decode + PhysicsShape_CreateTriMesh; three real worlds count/bbox/shape.
add_executable(coltrimesh_test
    ${CMAKE_CURRENT_SOURCE_DIR}/coltrimesh_test.cpp
    ${CODE_DIR}/StaticCollisionData.cpp
    ${CODE_DIR}/PhysicsBackend.cpp
    ${CODE_DIR}/rw_librw.cpp
    ${CODE_DIR}/RwStudioStream.cpp
)
target_include_directories(coltrimesh_test PRIVATE
    ${SDK_INCLUDE_DIRS}
    ${bullet3_SOURCE_DIR}/src
)
target_link_libraries(coltrimesh_test PRIVATE
    librw::librw
    BulletDynamics BulletCollision LinearMath
)
add_test(NAME coltrimesh_test COMMAND coltrimesh_test
    ${CANNONCRUISE_DATA_ROOT}/Streams/VIKING01.RWS
    ${CANNONCRUISE_DATA_ROOT}/Streams/IDUNA03.RWS
    ${CANNONCRUISE_DATA_ROOT}/Streams/BEAVER01.RWS)

# Phase 4.6n - Render platform smoke: librw scene setup + frame loop
# (clear/BeginUpdate/Im2D test frame/EndUpdate) + terminate/re-setup.
# NULL platform headless - device no-op; frame API contract verified.
add_executable(renderplatform_test
    ${CMAKE_CURRENT_SOURCE_DIR}/renderplatform_test.cpp
    ${CODE_DIR}/RenderPlatform.cpp
    ${CODE_DIR}/RenderPipeline.cpp
    ${CODE_DIR}/TxdDecode.cpp
    ${CODE_DIR}/rw_librw.cpp
)
target_include_directories(renderplatform_test PRIVATE ${SDK_INCLUDE_DIRS})
target_link_libraries(renderplatform_test PRIVATE librw::librw)
add_test(NAME renderplatform_test COMMAND renderplatform_test)

# Phase 4.6o - WorldGeometry decode + clump instantiate: [len][libid] prefix
# + STRUCT/GEOMETRY/EXT + ATOMIC x N (embedded geometry) - three real worlds
# inventory counts + VIKING01 clump (embedded streamRead) + corrupt input.
add_executable(worldgeometry_test
    ${CMAKE_CURRENT_SOURCE_DIR}/worldgeometry_test.cpp
    ${CODE_DIR}/WorldGeometry.cpp
    ${CODE_DIR}/RwStudioStream.cpp
    ${CODE_DIR}/rw_librw.cpp
)
target_include_directories(worldgeometry_test PRIVATE ${SDK_INCLUDE_DIRS})
target_link_libraries(worldgeometry_test PRIVATE librw::librw)
add_test(NAME worldgeometry_test COMMAND worldgeometry_test
    ${CANNONCRUISE_DATA_ROOT}/Streams/VIKING01.RWS
    ${CANNONCRUISE_DATA_ROOT}/Streams/IDUNA03.RWS
    ${CANNONCRUISE_DATA_ROOT}/Streams/BEAVER01.RWS)

# Phase 4.6s - WorldGeometry TRAILING chunk decode (after the atomics 0x22/0x1E/
# 0x39; in Phase 4.6o only the type was read). Header fields + body decoding.
add_executable(trailingchunk_test
    ${CMAKE_CURRENT_SOURCE_DIR}/trailingchunk_test.cpp
    ${CODE_DIR}/WorldGeometry.cpp
    ${CODE_DIR}/RwStudioStream.cpp
    ${CODE_DIR}/rw_librw.cpp
)
target_include_directories(trailingchunk_test PRIVATE ${SDK_INCLUDE_DIRS})
target_link_libraries(trailingchunk_test PRIVATE librw::librw)
add_test(NAME trailingchunk_test COMMAND trailingchunk_test
    ${CANNONCRUISE_DATA_ROOT}/Streams/VIKING01.RWS
    ${CANNONCRUISE_DATA_ROOT}/Streams/IDUNA03.RWS
    ${CANNONCRUISE_DATA_ROOT}/Streams/BEAVER01.RWS)

# GLOBAL.RWS asset inventory - embedded txds and external resource references
# (discovery/regression; output inspected from the ctest log).
add_executable(globalassets_test
    ${CMAKE_CURRENT_SOURCE_DIR}/globalassets_test.cpp
    ${CODE_DIR}/RwStudioStream.cpp
    ${CODE_DIR}/rw_librw.cpp
)
target_include_directories(globalassets_test PRIVATE ${SDK_INCLUDE_DIRS})
target_link_libraries(globalassets_test PRIVATE librw::librw)
add_test(NAME globalassets_test COMMAND globalassets_test
    ${CANNONCRUISE_DATA_ROOT}/Streams/GLOBAL.RWS)

# Dumps the GLOBAL embedded txd to PNGs - visual check of the loading screen art
# (output <outdir>/tex_*.png; not a ctest regression, a tool).
add_executable(globaldump_test
    ${CMAKE_CURRENT_SOURCE_DIR}/globaldump_test.cpp
    ${CODE_DIR}/TxdDecode.cpp
    ${CODE_DIR}/RwStudioStream.cpp
    ${CODE_DIR}/rw_librw.cpp
)
target_include_directories(globaldump_test PRIVATE ${SDK_INCLUDE_DIRS})
target_link_libraries(globaldump_test PRIVATE librw::librw)
add_test(NAME globaldump_test COMMAND globaldump_test
    ${CANNONCRUISE_DATA_ROOT}/Streams/GLOBAL.RWS
    ${CMAKE_CURRENT_SOURCE_DIR}/../Scratchpad/globaldump)

# rwID_CLUMP (ship .dff) format probe - decoded with the librw clump reader
# or tests it (discovery tool; not registered with ctest - call externally).
add_executable(shipprobe_test
    ${CMAKE_CURRENT_SOURCE_DIR}/shipprobe_test.cpp
    ${CODE_DIR}/rw_librw.cpp
)
target_include_directories(shipprobe_test PRIVATE ${SDK_INCLUDE_DIRS})
target_link_libraries(shipprobe_test PRIVATE librw::librw)

# .RWS asset payload dumper - rwID_CLUMP / wbID_WORLDGEOMETRY etc. binary
# payloads dumped as <out>.bin (for hex/output checking;
# discovery tool - not registered with ctest).
add_executable(assetdump_test
    ${CMAKE_CURRENT_SOURCE_DIR}/assetdump_test.cpp
    ${CODE_DIR}/RwStudioStream.cpp
    ${CODE_DIR}/rw_librw.cpp
)
target_include_directories(assetdump_test PRIVATE ${SDK_INCLUDE_DIRS})
target_link_libraries(assetdump_test PRIVATE librw::librw)

# .RWS objectRecord/manifest scanner - class names (e.g. CShipPlayer,
# CShipEnemy) and asset source paths (sourcePath .dff references)
# listed; to find the ships' dffs and spawn behaviours.
add_executable(shipscan_test
    ${CMAKE_CURRENT_SOURCE_DIR}/shipscan_test.cpp
    ${CODE_DIR}/RwStudioStream.cpp
    ${CODE_DIR}/rw_librw.cpp
)
target_include_directories(shipscan_test PRIVATE ${SDK_INCLUDE_DIRS})
target_link_libraries(shipscan_test PRIVATE librw::librw)
add_test(NAME shipscan_test COMMAND shipscan_test
    ${CANNONCRUISE_DATA_ROOT}/Streams/GLOBAL.RWS
    ${CANNONCRUISE_DATA_ROOT}/Streams/VIKING01.RWS)

# Phase 4.6s - integration stress: world load -> clump -> unload chain
# (A->B->C->A->B->C rounds; level transition + unload path must not crash).
add_executable(worldcycle_test
    ${CMAKE_CURRENT_SOURCE_DIR}/worldcycle_test.cpp
    ${CODE_DIR}/WorldGeometry.cpp
    ${CODE_DIR}/RwStudioStream.cpp
    ${CODE_DIR}/rw_librw.cpp
)
target_include_directories(worldcycle_test PRIVATE ${SDK_INCLUDE_DIRS})
target_link_libraries(worldcycle_test PRIVATE librw::librw)
add_test(NAME worldcycle_test COMMAND worldcycle_test
    ${CANNONCRUISE_DATA_ROOT}/Streams/VIKING01.RWS
    ${CANNONCRUISE_DATA_ROOT}/Streams/IDUNA03.RWS
    ${CANNONCRUISE_DATA_ROOT}/Streams/BEAVER01.RWS)

# Phase 4.6p - world spawn chain: 0x0704 records -> CClassFactory
# create + apply (known classes are created; unknown ones silently skipped).
# WHOLE_ARCHIVE: RWS_REGISTER_CLASS static registrations should be pulled from the archive.
add_executable(worldspawn_test
    ${CMAKE_CURRENT_SOURCE_DIR}/worldspawn_test.cpp
)
target_include_directories(worldspawn_test PRIVATE ${SDK_INCLUDE_DIRS})
target_link_libraries(worldspawn_test PRIVATE
    "$<LINK_LIBRARY:WHOLE_ARCHIVE,CannonCruiseLib>")
add_test(NAME worldspawn_test COMMAND worldspawn_test
    ${CANNONCRUISE_DATA_ROOT}/Streams/VIKING01.RWS
    ${CANNONCRUISE_DATA_ROOT}/Streams/MAINMENU.RWS)

# Phase 4.6q - ocean smoke: COcean grid setup + tile waves + Im3D
# render path running without crashing (NULL device no-op).
add_executable(ocean_test
    ${CMAKE_CURRENT_SOURCE_DIR}/ocean_test.cpp
)
target_include_directories(ocean_test PRIVATE ${SDK_INCLUDE_DIRS})
target_link_libraries(ocean_test PRIVATE
    "$<LINK_LIBRARY:WHOLE_ARCHIVE,CannonCruiseLib>")
add_test(NAME ocean_test COMMAND ocean_test)

# Phase 4.6r-1 - Rt2dMaestro .anm parser: Menu.anm (32 nodes) + CAG_Menu.anm
# (44 nodes) fixture - header/node/scene header one-to-one.
add_executable(rt2danim_test
    ${CMAKE_CURRENT_SOURCE_DIR}/rt2danim_test.cpp
)
target_include_directories(rt2danim_test PRIVATE ${SDK_INCLUDE_DIRS})
target_link_libraries(rt2danim_test PRIVATE
    "$<LINK_LIBRARY:WHOLE_ARCHIVE,CannonCruiseLib>")
add_test(NAME rt2danim_test COMMAND rt2danim_test
    ${CANNONCRUISE_DATA_ROOT}/Streams/MAINMENU.RWS
    ${CANNONCRUISE_DATA_ROOT}/Streams/MAPMENU.RWS)

# Phase 4.6r-4/infrastructure - menu+map state machine (headless, engineless).
add_executable(menustate_test
    ${CMAKE_CURRENT_SOURCE_DIR}/menustate_test.cpp
)
target_include_directories(menustate_test PRIVATE ${SDK_INCLUDE_DIRS})
target_link_libraries(menustate_test PRIVATE
    "$<LINK_LIBRARY:WHOLE_ARCHIVE,CannonCruiseLib>")
add_test(NAME menustate_test COMMAND menustate_test
    ${CANNONCRUISE_DATA_ROOT}/Streams/MAINMENU.RWS
    ${CANNONCRUISE_DATA_ROOT}/Streams/MAPMENU.RWS)

# .RWS asset path resolver - CANNONCRUISE_STREAMS env override + sentinel prefix
# scan (deep build layouts). The central resolver replaces the 5 hand-written candidate
# list (main.cpp / SceneManager.cpp).
add_executable(rwsassets_test
    ${CMAKE_CURRENT_SOURCE_DIR}/rwsassets_test.cpp
    ${CODE_DIR}/RwsAssets.cpp
)
target_include_directories(rwsassets_test PRIVATE ${SDK_INCLUDE_DIRS} ${CODE_DIR})
add_test(NAME rwsassets_test COMMAND rwsassets_test)

# FMV scene descriptor (CFMVBehavior) - 8-file transition graph (JINGLE/
# INTRO1-3/CREDITS/OUTRO1-3), video path ".\Video\Win32\<name>.bik" and TARGET
# scene link (INTRO1 -> NORDIC01; uppercase attempt required).
add_executable(fmvscene_test
    ${CMAKE_CURRENT_SOURCE_DIR}/fmvscene_test.cpp
    ${CODE_DIR}/FmvScene.cpp
    ${CODE_DIR}/RwsAssets.cpp
)
target_include_directories(fmvscene_test PRIVATE ${SDK_INCLUDE_DIRS} ${CODE_DIR})
add_test(NAME fmvscene_test COMMAND fmvscene_test)

# LocalizationManager test - Localiza/lang.def, langmap.def and lang_<lang>.txt
# parsing, BOM validation, string lookup and MENUTRIGGER event dispatch.
add_executable(localization_test
    ${CMAKE_CURRENT_SOURCE_DIR}/localization_test.cpp
)
target_include_directories(localization_test PRIVATE ${SDK_INCLUDE_DIRS} ${CODE_DIR})
target_link_libraries(localization_test PRIVATE "$<LINK_LIBRARY:WHOLE_ARCHIVE,CannonCruiseLib>")
add_test(NAME localization_test COMMAND localization_test)

# CShipPlayer test - reflection class factory registration, score, storm and respawn
add_executable(shipplayer_test
    ${CMAKE_CURRENT_SOURCE_DIR}/shipplayer_test.cpp
)
target_include_directories(shipplayer_test PRIVATE ${SDK_INCLUDE_DIRS} ${CODE_DIR})
target_link_libraries(shipplayer_test PRIVATE "$<LINK_LIBRARY:WHOLE_ARCHIVE,CannonCruiseLib>")
add_test(NAME shipplayer_test COMMAND shipplayer_test)

# Sprint 1 - player ship lives in the Bullet world: speed + progress asserts
add_executable(playground_test
    ${CMAKE_CURRENT_SOURCE_DIR}/playground_test.cpp
)
target_include_directories(playground_test PRIVATE ${SDK_INCLUDE_DIRS} ${CODE_DIR})
target_link_libraries(playground_test PRIVATE
    "$<LINK_LIBRARY:WHOLE_ARCHIVE,CannonCruiseLib>")
add_test(NAME playground_test COMMAND playground_test)

# InputManager test - Focus stack, button edge trigger detection and ShipPlayerInputProxy
add_executable(inputmanager_test
    ${CMAKE_CURRENT_SOURCE_DIR}/inputmanager_test.cpp
)
target_include_directories(inputmanager_test PRIVATE ${SDK_INCLUDE_DIRS} ${CODE_DIR})
target_link_libraries(inputmanager_test PRIVATE "$<LINK_LIBRARY:WHOLE_ARCHIVE,CannonCruiseLib>")
add_test(NAME inputmanager_test COMMAND inputmanager_test)

# ---------------------------------------------------------------------------
# Game-data wiring: every test resolves Streams/ through RwsAssets
# (CANNONCRUISE_STREAMS = folder that contains Streams/). Tests listed in
# CANNONCRUISE_DATA_TESTS read original game files and are disabled when the
# data is missing.
# ---------------------------------------------------------------------------
get_property(_cc_all_tests DIRECTORY PROPERTY TESTS)
set_tests_properties(${_cc_all_tests} PROPERTIES
    ENVIRONMENT "CANNONCRUISE_STREAMS=${CANNONCRUISE_DATA_ROOT}/")
set(CANNONCRUISE_DATA_TESTS
    rwlibrw_test
    rwstudio_test
    renderworld_test
    camerastate_test
    audiostream_test
    behaviourloader_test
    txddecode_test
    coltrimesh_test
    worldgeometry_test
    trailingchunk_test
    globalassets_test
    globaldump_test
    worldcycle_test
    worldspawn_test
    rt2danim_test
    menustate_test
    rwsassets_test
    fmvscene_test)
# The sentinel scan walks up from the working directory, so run it one level
# below the data root (exercises the "../" prefix).
if(CANNONCRUISE_HAVE_DATA)
    set_tests_properties(rwsassets_test PROPERTIES
        WORKING_DIRECTORY "${CANNONCRUISE_DATA_ROOT}/Streams")
else()
    set_tests_properties(${CANNONCRUISE_DATA_TESTS} PROPERTIES DISABLED TRUE)
endif()
