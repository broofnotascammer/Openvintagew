## @file
#  OpenVintage Platform Description File (.DSC)
#
#  OpenVintage is a modular compatibility and performance platform
#  targeting legacy Intel x86_64 hardware (2006-2015).
#
#  Copyright (c) 2026 OpenVintage Project. All rights reserved.
#  SPDX-License-Identifier: BSD-2-Clause-Patent
##

[Defines]
  PLATFORM_NAME                  = OpenVintage
  PLATFORM_GUID                  = 7F89D3A4-4A25-47D0-A0F1-0268EC3B7B39
  PLATFORM_VERSION               = 0.20
  DSC_SPECIFICATION              = 0x00010005
  OUTPUT_DIRECTORY               = Build/OpenVintageX64
  SUPPORTED_ARCHITECTURES        = X64
  BUILD_TARGETS                  = DEBUG|RELEASE|NOOPT
  SKUID_IDENTIFIER               = DEFAULT
  FLASH_DEFINITION               = OpenVintagePkg/OpenVintagePkg.fdf

################################################################################
# Library Classes Section
################################################################################
[LibraryClasses]
  # Basic
  BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
  BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
  PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
  IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
  PciLib|MdePkg/Library/BasePciLibCf8/BasePciLibCf8.inf
  PciCf8Lib|MdePkg/Library/BasePciCf8Lib/BasePciCf8Lib.inf
  PciExpressLib|MdePkg/Library/BasePciExpressLib/BasePciExpressLib.inf
  CpuLib|MdePkg/Library/BaseCpuLib/BaseCpuLib.inf
  SynchronizationLib|MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf
  PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
  DebugLib|MdePkg/Library/BaseDebugLibNull/BaseDebugLibNull.inf
  DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
  PeCoffLib|MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
  PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
  PeCoffExtraActionLib|MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
  RegisterFilterLib|MdePkg/Library/RegisterFilterLibNull/RegisterFilterLibNull.inf

  # UEFI & DXE
  UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
  UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
  UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
  UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
  UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
  DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
  DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf
  DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
  SortLib|MdeModulePkg/Library/BaseSortLib/BaseSortLib.inf

  # OpenVintage Custom Libraries
  OpenVintageCoreLib|OpenVintagePkg/Library/OpenVintageCoreLib/OpenVintageCoreLib.inf
  OpenVintageLogLib|OpenVintagePkg/Library/OpenVintageLogLib/OpenVintageLogLib.inf
  OvLoggerLib|OpenVintagePkg/Library/OvLoggerLib/OvLoggerLib.inf
  OvConfigLib|OpenVintagePkg/Core/OvConfigLib.inf
  OvMemoryLib|OpenVintagePkg/Memory/OvMemoryLib.inf
  OvHardwareLib|OpenVintagePkg/Hardware/OvHardwareLib.inf
  OvModuleLib|OpenVintagePkg/Core/OvModuleLib.inf
  OvResolverLib|OpenVintagePkg/Resolver/OvResolverLib.inf
  OvSchedulerLib|OpenVintagePkg/Scheduler/OvSchedulerLib.inf
  OvCoreLib|OpenVintagePkg/Core/OvCoreLib.inf

[LibraryClasses.common.UEFI_APPLICATION]
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf

[LibraryClasses.common.DXE_DRIVER]
  MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf

################################################################################
# PCD Section
################################################################################
[PcdsFixedAtBuild]
  gOpenVintagePkgTokenSpaceGuid.PcdOpenVintageBannerEnable|TRUE
  gOpenVintagePkgTokenSpaceGuid.PcdOpenVintageVersionString|L"OpenVintage Platform Firmware v0.2.0 (X64)"
  gOpenVintagePkgTokenSpaceGuid.PcdOpenVintageTargetArchitecture|L"Intel Core 2 / Nehalem / Sandy / Ivy / Haswell"
  gOpenVintagePkgTokenSpaceGuid.PcdOpenVintageLogLevel|0x00000003

################################################################################
# Components Section - Modules to Build
################################################################################
[Components]
  # OpenVintage Libraries
  OpenVintagePkg/Library/OpenVintageLogLib/OpenVintageLogLib.inf
  OpenVintagePkg/Library/OpenVintageCoreLib/OpenVintageCoreLib.inf
  OpenVintagePkg/Library/OvLoggerLib/OvLoggerLib.inf
  OpenVintagePkg/Core/OvConfigLib.inf
  OpenVintagePkg/Memory/OvMemoryLib.inf
  OpenVintagePkg/Hardware/OvHardwareLib.inf
  OpenVintagePkg/Core/OvModuleLib.inf
  OpenVintagePkg/Resolver/OvResolverLib.inf
  OpenVintagePkg/Scheduler/OvSchedulerLib.inf
  OpenVintagePkg/Core/OvCoreLib.inf

  # OpenVintage DXE Drivers
  OpenVintagePkg/Drivers/OpenVintageHalDxe/OpenVintageHalDxe.inf

  # OpenVintage Applications & Architectural Test Suite
  OpenVintagePkg/OpenVintageBootApp/OpenVintageBootApp.inf
  OpenVintagePkg/Tests/OvSelfTestApp.inf
