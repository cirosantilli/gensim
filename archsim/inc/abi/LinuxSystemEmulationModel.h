/*
 * File:   LinuxSystemEmulationModel.h
 * Author: s0457958
 *
 * Created on 04 September 2014, 12:49
 */

#ifndef LINUXSYSTEMEMULATIONMODEL_H
#define	LINUXSYSTEMEMULATIONMODEL_H

#include "abi/SystemEmulationModel.h"

namespace archsim
{
	namespace abi
	{
		struct LinuxEmulationComponent {
		public:
			bool valid;
			uint32_t address;
			uint32_t size;
		};
		class LinuxSystemEmulationModel : public SystemEmulationModel
		{
		public:
			LinuxSystemEmulationModel();
			virtual ~LinuxSystemEmulationModel();

			bool Initialise(System& system, archsim::uarch::uArch& uarch) override;
			void Destroy() override;

		protected:
			bool InstallPlatform(loader::BinaryLoader& loader) override;

			inline const LinuxEmulationComponent& GetRootFSComponent() const
			{
				return rootfs;
			}
			inline const LinuxEmulationComponent& GetDeviceTreeComponent() const
			{
				return devicetree;
			}
			inline const LinuxEmulationComponent& GetATAGSComponent() const
			{
				return atags;
			}

		private:
			LinuxEmulationComponent rootfs;
			LinuxEmulationComponent devicetree;
			LinuxEmulationComponent atags;

			bool InstallDeviceTree(std::string filename, uint32_t addr);
			bool InstallATAGS(std::string kernel_args, uint32_t addr);
			bool InstallRootFS(std::string filename, uint32_t addr);


		};
	}
}

#endif	/* LINUXSYSTEMEMULATIONMODEL_H */
