#
# Build PhysX (PROJECT not SOLUTION)
#

SET(GW_DEPS_ROOT $ENV{GW_DEPS_ROOT})
FIND_PACKAGE(PxShared REQUIRED)

SET(PHYSX_SOURCE_DIR ${PROJECT_SOURCE_DIR}/../../../)

SET(PX_SOURCE_DIR ${PHYSX_SOURCE_DIR}/PhysX/src)
SET(MD_SOURCE_DIR ${PHYSX_SOURCE_DIR}/PhysXMetaData)

FIND_PACKAGE(nvToolsExt REQUIRED)

SET(PHYSX_PLATFORM_INCLUDES
	${NVTOOLSEXT_INCLUDE_DIRS}
)

SET(PHYSX_PLATFORM_SRC_FILES
	${PX_SOURCE_DIR}/device/windows/PhysXIndicatorWindows.cpp
	${PX_SOURCE_DIR}/gpu/NpPhysicsGpu.cpp
	${PX_SOURCE_DIR}/gpu/PxGpu.cpp
	${PX_SOURCE_DIR}/gpu/PxParticleDeviceExclusive.cpp
	${PX_SOURCE_DIR}/gpu/PxParticleGpu.cpp
	${PX_SOURCE_DIR}/gpu/PxPhysXGpuModuleLoader.cpp
	${PX_SOURCE_DIR}/gpu/PxPhysXIndicatorDeviceExclusive.cpp
	${PX_SOURCE_DIR}/windows/NpWindowsDelayLoadHook.cpp
	${PHYSX_SOURCE_DIR}/compiler/resource_${LIBPATH_SUFFIX}/PhysX3.rc
)


# Use generator expressions to set config specific preprocessor definitions
SET(PHYSX_COMPILE_DEFS
	# Common to all configurations
	${PHYSX_WINDOWS_COMPILE_DEFS};PX_PHYSX_CORE_EXPORTS

	$<$<CONFIG:debug>:${PHYSX_WINDOWS_DEBUG_COMPILE_DEFS};PX_PHYSX_DLL_NAME_POSTFIX=DEBUG;>
	$<$<CONFIG:checked>:${PHYSX_WINDOWS_CHECKED_COMPILE_DEFS};PX_PHYSX_DLL_NAME_POSTFIX=CHECKED;>
	$<$<CONFIG:profile>:${PHYSX_WINDOWS_PROFILE_COMPILE_DEFS};PX_PHYSX_DLL_NAME_POSTFIX=PROFILE;>
	$<$<CONFIG:release>:${PHYSX_WINDOWS_RELEASE_COMPILE_DEFS};>
)

SET(PHYSX_LIBTYPE SHARED)

# include common PhysX settings
INCLUDE(../common/PhysX.cmake)


# Add linked libraries
# TARGET_LINK_LIBRARIES(PhysX PUBLIC ${NVTOOLSEXT_LIBRARIES} LowLevel LowLevelAABB LowLevelCloth LowLevelDynamics LowLevelParticles PhysXCommon PhysXGpu PxFoundation PxPvdSDK PxTask SceneQuery SimulationController)

TARGET_LINK_LIBRARIES(PhysX PUBLIC ${NVTOOLSEXT_LIBRARIES} LowLevel LowLevelAABB LowLevelCloth LowLevelDynamics LowLevelParticles PhysXCommon PxFoundation PxPvdSDK PxTask SceneQuery SimulationController)

SET_TARGET_PROPERTIES(PhysX PROPERTIES 
	LINK_FLAGS_DEBUG "/MAP /DELAYLOAD:PxFoundationDEBUG_${LIBPATH_SUFFIX}.dll /DELAYLOAD:PhysX3CommonDEBUG_${LIBPATH_SUFFIX}.dll /DEBUG"
	LINK_FLAGS_CHECKED "/MAP /DELAYLOAD:PxFoundationCHECKED_${LIBPATH_SUFFIX}.dll /DELAYLOAD:PhysX3CommonCHECKED_${LIBPATH_SUFFIX}.dll"
	LINK_FLAGS_PROFILE "/MAP /DELAYLOAD:PxFoundationPROFILE_${LIBPATH_SUFFIX}.dll /DELAYLOAD:PhysX3CommonPROFILE_${LIBPATH_SUFFIX}.dll /INCREMENTAL:NO /DEBUG"
	LINK_FLAGS_RELEASE "/MAP /DELAYLOAD:PhysX3Common_${LIBPATH_SUFFIX}.dll /DELAYLOAD:PxFoundation_${LIBPATH_SUFFIX}.dll /INCREMENTAL:NO"
)
