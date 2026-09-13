# Third-Party Software and License Policy

OpenVintage contains original OpenVintage code and may build against, package, or integrate with third-party projects. Third-party copyrights and licenses remain in force and are not replaced by the OpenVintage license.

## Current build dependency: TianoCore EDK II

`OpenVintagePkg` is an EDK II platform package and its DSC references EDK II libraries such as `MdePkg` and `MdeModulePkg`. EDK II is therefore a build dependency and portions of firmware binaries produced from those libraries may contain EDK II code.

EDK II's majority license is BSD-2-Clause-Patent, with additional components carrying separate licenses. OpenVintage must not claim EDK II code as OpenVintage-owned code. See the upstream EDK II license and licensing inventory before distributing binaries.

Upstream:
- https://github.com/tianocore/edk2
- https://github.com/tianocore/edk2/blob/master/License.txt

## Future integrations

OpenVintage is planned to integrate with established boot and macOS compatibility projects, including rEFInd and OpenCore Legacy Patcher (OCLP). These components are **not treated as OpenVintage-owned code**.

Before a release bundles either project, the exact upstream version/commit and every redistributed dependency must be recorded in a release-specific software bill of materials (SBOM), with:

- component name
- upstream repository
- exact version or commit
- copyright holders
- applicable license(s)
- modification status
- required source-code availability
- required notices and attribution
- redistribution conditions

No third-party component may be relicensed under the OpenVintage license.

## Commercial distribution policy

A commercial OpenVintage distribution may contain differently licensed third-party components only where their respective licenses permit the intended distribution and all license conditions are satisfied. Proprietary Apple software, firmware, macOS components, trademarks, and other Apple-controlled materials are separate from open-source licensing and require independent review before redistribution.

This document is an engineering compliance policy, not legal advice. Final commercial releases should undergo a professional software-license/IP review.
