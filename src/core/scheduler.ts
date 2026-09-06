/**
 * OpenVintage - OVScheduler / System Manager
 * Dynamic core topology mapping, performance profile regulation, and thermal coordination.
 */

import { HardwareProfile, PerformanceProfile, SchedulerState, CoreAffinity } from '../types';

export class OVScheduler {
  /**
   * Generates initial scheduler state calibrated to the hardware profile.
   */
  public static initialize(hardware: HardwareProfile, profile: PerformanceProfile = 'balanced'): SchedulerState {
    const totalPhysical = hardware.cpu.physicalCores;
    const totalThreads = hardware.cpu.logicalThreads;

    const cores: CoreAffinity[] = [];
    for (let i = 0; i < totalThreads; i++) {
      const isHt = i >= totalPhysical;
      let role: CoreAffinity['assignedRole'] = 'idle';

      if (i === 0) {
        role = 'primary'; // Render loop / main thread
      } else if (i === 1) {
        role = 'io'; // IO and driver dispatch
      } else {
        role = 'worker'; // Translation / shader compile / compute pool
      }

      cores.push({
        coreId: i,
        isHyperthread: isHt,
        assignedRole: role,
        loadPercent: 10 + Math.floor(Math.random() * 25),
      });
    }

    const initialTemp = profile === 'battery' ? 48 : profile === 'max_performance' ? 76 : 58;

    return {
      currentProfile: profile,
      totalPhysicalCores: totalPhysical,
      totalThreads: totalThreads,
      cores,
      workerPoolSize: Math.max(1, totalPhysical - 1),
      cooperativeOsScheduling: profile !== 'max_performance',
      thermalThrottled: initialTemp > 85,
      thermalTempC: initialTemp,
      memoryUsageMB: Math.round(hardware.ramMB * 0.35),
      memoryCapacityMB: hardware.ramMB,
    };
  }

  /**
   * Adjusts scheduler configuration when the user or system switches profiles.
   */
  public static applyProfile(
    currentState: SchedulerState,
    newProfile: PerformanceProfile
  ): SchedulerState {
    const updatedCores = currentState.cores.map((core) => {
      let baseLoad = 15;
      if (newProfile === 'battery') {
        baseLoad = core.coreId === 0 ? 30 : 5;
      } else if (newProfile === 'balanced') {
        baseLoad = 25 + (core.coreId % 3) * 10;
      } else if (newProfile === 'performance') {
        baseLoad = 55 + (core.coreId % 2) * 15;
      } else if (newProfile === 'max_performance') {
        baseLoad = 80 + (core.coreId % 2) * 12;
      } else if (newProfile === 'developer') {
        baseLoad = 40;
      }

      return {
        ...core,
        loadPercent: Math.min(100, Math.max(0, baseLoad)),
      };
    });

    let tempC = 55;
    if (newProfile === 'battery') tempC = 46;
    else if (newProfile === 'performance') tempC = 72;
    else if (newProfile === 'max_performance') tempC = 86;
    else if (newProfile === 'developer') tempC = 58;

    return {
      ...currentState,
      currentProfile: newProfile,
      cores: updatedCores,
      cooperativeOsScheduling: newProfile !== 'max_performance',
      thermalThrottled: tempC >= 85,
      thermalTempC: tempC,
      workerPoolSize:
        newProfile === 'battery'
          ? 1
          : newProfile === 'max_performance'
          ? currentState.totalPhysicalCores
          : Math.max(1, currentState.totalPhysicalCores - 1),
    };
  }
}
