import express from 'express';
import { createServer as createViteServer } from 'vite';
import path from 'path';
import { fileURLToPath } from 'url';
import dotenv from 'dotenv';

dotenv.config();

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

async function startServer() {
  const app = express();
  const PORT = process.env.PORT ? parseInt(process.env.PORT) : 3000;
  const isProd = process.env.NODE_ENV === 'production';

  app.use(express.json());

  // API endpoints for OpenVintage Pre-boot Simulator & Boot Management
  app.get('/api/health', (req, res) => {
    res.json({
      status: 'healthy',
      platform: 'OpenVintage Boot Picker & Pre-boot Simulator',
      phase: 'Phase 9.1',
      version: '9.1.0-RC1',
      espIntegrity: 'STRICT_PRESERVED',
      firmwareSafety: 'ENABLED'
    });
  });

  // Mock / Real simulated UEFI scan
  app.get('/api/uefi/targets', (req, res) => {
    res.json({
      timestamp: new Date().toISOString(),
      defaultTargetId: 'macos-apfs-1',
      timeoutSeconds: 5,
      targets: [
        {
          id: 'macos-apfs-1',
          name: 'macOS Monterey (APFS)',
          osType: 'macOS',
          devicePath: 'PciRoot(0x0)/Pci(0x1F,0x2)/Sata(0x0,0x0)/HD(2,GPT,C12A7328-F81F-11D2-BA4B-00A0C93EC93B)/System/Library/CoreServices/boot.efi',
          fsProtocol: 'EFI_SIMPLE_FILE_SYSTEM_PROTOCOL',
          isDefault: true,
          status: 'VERIFIED',
          icon: 'apple',
          halRequired: true,
          appleFirmwarePreserved: true
        },
        {
          id: 'opencore-esp',
          name: 'OpenCore 1.0.3 (Rescue Chainloader)',
          osType: 'OpenCore',
          devicePath: 'PciRoot(0x0)/Pci(0x1F,0x2)/Sata(0x0,0x0)/HD(1,GPT,7E54D748-0A1D-4C92-A23F-8C3729F45901)/EFI/OC/OpenCore.efi',
          fsProtocol: 'EFI_SIMPLE_FILE_SYSTEM_PROTOCOL',
          isDefault: false,
          status: 'VERIFIED',
          icon: 'shield',
          halRequired: false,
          appleFirmwarePreserved: true
        },
        {
          id: 'linux-grub',
          name: 'Ubuntu 24.04 LTS (GRUBx64)',
          osType: 'Linux',
          devicePath: 'PciRoot(0x0)/Pci(0x1F,0x2)/Sata(0x0,0x0)/HD(4,GPT,4952D4F2-BE7A-4560-A6E5-A9345BE7678B)/EFI/ubuntu/grubx64.efi',
          fsProtocol: 'EFI_SIMPLE_FILE_SYSTEM_PROTOCOL',
          isDefault: false,
          status: 'VERIFIED',
          icon: 'terminal',
          halRequired: false,
          appleFirmwarePreserved: true
        },
        {
          id: 'windows-bootmgr',
          name: 'Windows 11 Pro (bootmgfw)',
          osType: 'Windows',
          devicePath: 'PciRoot(0x0)/Pci(0x1F,0x2)/Sata(0x0,0x0)/HD(3,GPT,EBD0A0A2-B9E5-4433-87C0-68B6B72699C7)/EFI/Microsoft/Boot/bootmgfw.efi',
          fsProtocol: 'EFI_SIMPLE_FILE_SYSTEM_PROTOCOL',
          isDefault: false,
          status: 'VERIFIED',
          icon: 'monitor',
          halRequired: false,
          appleFirmwarePreserved: true
        },
        {
          id: 'refind-fallback',
          name: 'rEFInd Boot Manager (v0.14.2)',
          osType: 'rEFInd',
          devicePath: 'PciRoot(0x0)/Pci(0x1F,0x2)/Sata(0x0,0x0)/HD(1,GPT,7E54D748-0A1D-4C92-A23F-8C3729F45901)/EFI/refind/refind_x64.efi',
          fsProtocol: 'EFI_SIMPLE_FILE_SYSTEM_PROTOCOL',
          isDefault: false,
          status: 'VERIFIED',
          icon: 'compass',
          halRequired: false,
          appleFirmwarePreserved: true
        },
        {
          id: 'usb-external-boot',
          name: 'External USB Test (BOOTX64.EFI)',
          osType: 'OpenVintage Test',
          devicePath: 'PciRoot(0x0)/Pci(0x14,0x0)/USB(0x1,0x0)/HD(1,MBR,0x12345678)/EFI/BOOT/BOOTX64.EFI',
          fsProtocol: 'EFI_SIMPLE_FILE_SYSTEM_PROTOCOL',
          isDefault: false,
          status: 'ISOLATED_USB_SAFE',
          icon: 'usb',
          halRequired: true,
          appleFirmwarePreserved: true
        }
      ]
    });
  });

  // Simulated Boot Target Launch Test via gBS->LoadImage & StartImage
  app.post('/api/uefi/handoff', (req, res) => {
    const { targetId, simulateFailure } = req.body;
    
    if (simulateFailure) {
      return res.status(500).json({
        success: false,
        phase: 'gBS->LoadImage() / StartImage()',
        status: 'EFI_LOAD_ERROR',
        errorCode: '0x8000000000000001',
        message: 'Image verification failed or corrupted EFI binary. Falling back to OpenVintage Boot Picker interface.',
        fallbackTarget: 'macos-apfs-1',
        canRetry: true
      });
    }

    res.json({
      success: true,
      phase: 'SAFE_EFI_HANDOFF',
      status: 'EFI_SUCCESS',
      flow: [
        'Apple BootROM dispatched OpenVintageBootApp.efi',
        'OpenVintage hardware initialization & HAL verified',
        'OpenVintage boot picker displayed with 5s timeout',
        `Target selected: ${targetId}`,
        'gBS->LoadImage(FALSE, ImageHandle, DevicePath, NULL, 0, &TargetHandle) => EFI_SUCCESS',
        'OpenVintage pre-boot telemetry flushed to memory buffer',
        'gBS->StartImage(TargetHandle, NULL, NULL) => EFI_SUCCESS',
        'Handoff complete without modifying internal ESP or NVRAM'
      ]
    });
  });

  // Vite middleware in dev or static files in prod
  if (!isProd) {
    const vite = await createViteServer({
      server: { middlewareMode: true },
      appType: 'spa'
    });
    app.use(vite.middlewares);
  } else {
    app.use(express.static(path.join(__dirname, 'dist')));
    app.get('*', (req, res) => {
      res.sendFile(path.join(__dirname, 'dist', 'index.html'));
    });
  }

  app.listen(PORT, '0.0.0.0', () => {
    console.log(`[OpenVintage] Server running at http://0.0.0.0:${PORT}`);
  });
}

startServer().catch((err) => {
  console.error('[OpenVintage] Server start error:', err);
  process.exit(1);
});
