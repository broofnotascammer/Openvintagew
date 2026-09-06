/** @file
  OpenVintage Subsystem Module Abstraction Definition.
  Phase 2 Pluggable Module Architecture.

  Copyright (c) 2026 OpenVintage Project. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef OV_MODULE_LIB_H_
#define OV_MODULE_LIB_H_

#include <Uefi.h>

#define OV_MAX_MODULES  32

//
// Module Status Lifecycle
//
typedef enum {
  OvModStatusUnregistered = 0,
  OvModStatusRegistered,
  OvModStatusInitializing,
  OvModStatusActive,
  OvModStatusDegraded,
  OvModStatusError,
  OvModStatusShutdown
} OV_MODULE_STATUS;

//
// Module Classification
//
typedef enum {
  OvModTypeCore = 1,
  OvModTypeConfig,
  OvModTypeMemory,
  OvModTypeHardware,
  OvModTypeResolver,
  OvModTypeScheduler,
  OvModTypeGpuIr,      // Placeholder for future OVIR-GPU
  OvModTypeCpuIr,      // Placeholder for future OVIR-CPU
  OvModTypeCustom
} OV_MODULE_TYPE;

typedef struct _OV_MODULE_DESCRIPTOR OV_MODULE_DESCRIPTOR;

typedef
EFI_STATUS
(EFIAPI *OV_MODULE_INIT_FN)(
  IN OV_MODULE_DESCRIPTOR *Module
  );

typedef
EFI_STATUS
(EFIAPI *OV_MODULE_SHUTDOWN_FN)(
  IN OV_MODULE_DESCRIPTOR *Module
  );

typedef
OV_MODULE_STATUS
(EFIAPI *OV_MODULE_GET_STATUS_FN)(
  IN OV_MODULE_DESCRIPTOR *Module
  );

struct _OV_MODULE_DESCRIPTOR {
  UINT32                  ModuleId;
  CHAR16                  Name[32];
  OV_MODULE_TYPE          Type;
  UINT32                  Version;
  OV_MODULE_STATUS        Status;
  OV_MODULE_INIT_FN       Initialize;
  OV_MODULE_SHUTDOWN_FN   Shutdown;
  OV_MODULE_GET_STATUS_FN GetStatus;
  VOID                    *Context;
};

/**
  Initialize module management engine.

  @retval EFI_SUCCESS  Module subsystem ready.
**/
EFI_STATUS
EFIAPI
OvModuleInitializeEngine (
  VOID
  );

/**
  Register a module into the OpenVintage runtime.

  @param[in,out] Descriptor  Module descriptor to register.

  @retval EFI_SUCCESS        Module registered.
  @retval EFI_ALREADY_STARTED Module already registered.
  @retval EFI_OUT_OF_RESOURCES Exceeded maximum modules.
**/
EFI_STATUS
EFIAPI
OvModuleRegister (
  IN OUT OV_MODULE_DESCRIPTOR  *Descriptor
  );

/**
  Initialize all registered modules in order of registration.

  @retval EFI_SUCCESS  All modules initialized cleanly.
**/
EFI_STATUS
EFIAPI
OvModuleInitializeAll (
  VOID
  );

/**
  Gracefully shut down all active modules in reverse registration order.

  @retval EFI_SUCCESS  All modules shut down.
**/
EFI_STATUS
EFIAPI
OvModuleShutdownAll (
  VOID
  );

/**
  Get total count of registered modules.

  @return UINTN  Number of registered modules.
**/
UINTN
EFIAPI
OvModuleGetCount (
  VOID
  );

/**
  Retrieve descriptor pointer by module ID.

  @param[in]  ModuleId    Module ID.
  @param[out] Descriptor  Receives pointer to module descriptor.

  @retval EFI_SUCCESS     Found module.
  @retval EFI_NOT_FOUND   Module ID does not exist.
**/
EFI_STATUS
EFIAPI
OvModuleGetDescriptor (
  IN  UINT32                ModuleId,
  OUT OV_MODULE_DESCRIPTOR  **Descriptor
  );

/**
  Log status of all registered modules.
**/
VOID
EFIAPI
OvModuleDumpList (
  VOID
  );

#endif // OV_MODULE_LIB_H_
