#include "obscura/DM_Particle.hpp"

#include <cmath>
#include <functional>
#include <iostream>

#include "libphysica/Integration.hpp"
#include "libphysica/Natural_Units.hpp"
#include "libphysica/Statistics.hpp"

namespace obscura
{
using namespace libphysica::natural_units;

//1. Base class for a DM particle with virtual functions for the cross sections
DM_Particle::DM_Particle()
: low_mass(false), using_cross_section(false), mass(10.0 * GeV), spin(1.0 / 2.0), fractional_density(1.0), DD_use_eta_function(false)
{
}

DM_Particle::DM_Particle(double m, double s)
: low_mass(false), using_cross_section(false), mass(m), spin(s), fractional_density(1.0), DD_use_eta_function(false)
{
}

void DM_Particle::Set_Mass(double mDM)
{
	// The cross sections might depend on the DM mass, and need to be re-computed to yield the same cross sections.
	double sigma_p = Sigma_Proton();
	double sigma_n = Sigma_Neutron();
	double sigma_e = Sigma_Electron();

	mass = mDM;

	Set_Sigma_Proton(sigma_p);
	Set_Sigma_Neutron(sigma_n);
	Set_Sigma_Electron(sigma_e);
}

void DM_Particle::Set_Spin(double s)
{
	spin = s;
}

void DM_Particle::Set_Low_Mass_Mode(bool ldm)
{
	low_mass = ldm;
}

void DM_Particle::Set_Fractional_Density(double f)
{
	fractional_density = f;
}

bool DM_Particle::Interaction_Parameter_Is_Cross_Section() const
{
	return using_cross_section;
}

double DM_Particle::Sigma_Total_Nucleus_Base(const Isotope& target, double vDM, double param)
{
	//Numerically integrate the differential cross section
	double q2min						= 0;
	double q2max						= 4.0 * pow(libphysica::Reduced_Mass(mass, target.mass) * vDM, 2.0);
	std::function<double(double)> dodq2 = [this, &target, vDM, param](double q2) {
		return dSigma_dq2_Nucleus(sqrt(q2), target, vDM, param);
	};
	double sigmatot = libphysica::Integrate(dodq2, q2min, q2max);
	return sigmatot;
}

double DM_Particle::Sigma_Total_Electron_Base(double vDM, double param)
{
	//Numerically integrate the differential cross section
	double q2min						= 0;
	double q2max						= 4.0 * pow(libphysica::Reduced_Mass(mass, mElectron) * vDM, 2.0);
	std::function<double(double)> dodq2 = [this, vDM, param](double q2) {
		return dSigma_dq2_Electron(sqrt(q2), vDM, param);
	};
	double sigmatot = libphysica::Integrate(dodq2, q2min, q2max);
	return sigmatot;
}

void DM_Particle::Print_Summary_Base(int MPI_rank) const
{
	if(MPI_rank == 0)
	{
		std::cout << std::endl
				  << "----------------------------------------" << std::endl
				  << "DM particle summary:" << std::endl;

		double massunit			= (mass < keV) ? eV : ((mass < MeV) ? keV : ((mass < GeV) ? MeV : GeV));
		std::string massunitstr = (mass < keV) ? "eV" : ((mass < MeV) ? "keV" : ((mass < GeV) ? "MeV" : "GeV"));
		std::cout << "\tMass:\t\t\t" << In_Units(mass, massunit) << " " << massunitstr << std::endl
				  << "\tSpin:\t\t\t" << spin << std::endl
				  << "\tLow mass:\t\t" << ((low_mass) ? "[x]" : "[ ]") << std::endl;
	}
}

double DM_Particle::dSigma_dER_Nucleus(double ER, const Isotope& target, double vDM, double param) const
{
	double q = sqrt(2.0 * target.mass * ER);
	return 2.0 * target.mass * dSigma_dq2_Nucleus(q, target, vDM, param);
}

double DM_Particle::d2Sigma_dER_dEe_Migdal(double ER, double Ee, double vDM, const Isotope& isotope, Atomic_Electron& shell) const
{
	double q  = sqrt(2.0 * isotope.mass * ER);
	double qe = mElectron / isotope.mass * q;
	return 1.0 / 4.0 / Ee * dSigma_dER_Nucleus(ER, isotope, vDM) * shell.Ionization_Form_Factor(qe, Ee);
}

double DM_Particle::Sigma_Total_Nucleus(const Isotope& target, double vDM, double param)
{
	return Sigma_Total_Nucleus_Base(target, vDM, param);
}
double DM_Particle::Sigma_Total_Electron(double vDM, double param)
{
	return Sigma_Total_Electron_Base(vDM, param);
}

void DM_Particle::Print_Summary(int MPI_rank) const
{
	Print_Summary_Base(MPI_rank);
}

// Scattering angle functions
double DM_Particle::PDF_Scattering_Angle_Nucleus_Base(double cos_alpha, const Isotope& target, double vDM, double param)
{
	double q		= libphysica::Reduced_Mass(target.mass, mass) * vDM * sqrt(2.0 * (1.0 - cos_alpha));
	double q2max	= 4.0 * libphysica::Reduced_Mass(target.mass, mass) * libphysica::Reduced_Mass(target.mass, mass) * vDM * vDM;
	double SigmaTot = Sigma_Total_Nucleus(target, vDM, param);
	if(SigmaTot != 0.0)
		return q2max / 2.0 / SigmaTot * dSigma_dq2_Nucleus(q, target, vDM, param);
	else
		return 0.0;
}

double DM_Particle::PDF_Scattering_Angle_Electron_Base(double cos_alpha, double vDM, double param)
{
	double q		= libphysica::Reduced_Mass(mElectron, mass) * vDM * sqrt(2.0 * (1.0 - cos_alpha));
	double q2max	= 4.0 * libphysica::Reduced_Mass(mElectron, mass) * libphysica::Reduced_Mass(mElectron, mass) * vDM * vDM;
	double SigmaTot = Sigma_Total_Electron(vDM, param);
	if(SigmaTot != 0.0)
		return q2max / 2.0 / SigmaTot * dSigma_dq2_Electron(q, vDM, param);
	else
		return 0.0;
}

double DM_Particle::CDF_Scattering_Angle_Nucleus_Base(double cos_alpha, const Isotope& target, double vDM, double param)
{
	if(cos_alpha <= -1.0)
		return 0.0;
	else if(cos_alpha >= 1.0)
		return 1.0;
	else
	{
		auto integrand = std::bind(&DM_Particle::PDF_Scattering_Angle_Nucleus_Base, this, std::placeholders::_1, target, vDM, param);
		double cdf	   = libphysica::Integrate(integrand, -1.0, cos_alpha);
		return cdf;
	}
}

double DM_Particle::CDF_Scattering_Angle_Electron_Base(double cos_alpha, double vDM, double param)
{
	if(cos_alpha <= -1.0)
		return 0.0;
	else if(cos_alpha >= 1.0)
		return 1.0;
	else
	{
		auto integrand = std::bind(&DM_Particle::PDF_Scattering_Angle_Electron_Base, this, std::placeholders::_1, vDM, param);
		double cdf	   = libphysica::Integrate(integrand, -1.0, cos_alpha);
		return cdf;
	}
}

double DM_Particle::Sample_Scattering_Angle_Nucleus_Base(std::mt19937& PRNG, const Isotope& target, double vDM, double param)
{
	double xi						  = libphysica::Sample_Uniform(PRNG, 0.0, 1.0);
	std::function<double(double)> cdf = [this, xi, &target, vDM, param](double cosa) {
		return xi - CDF_Scattering_Angle_Nucleus(cosa, target, vDM, param);
	};
	double cos_alpha = libphysica::Find_Root(cdf, -1.0, 1.0, 1e-6);
	return cos_alpha;
}

double DM_Particle::Sample_Scattering_Angle_Electron_Base(std::mt19937& PRNG, double vDM, double param)
{
	double xi						  = libphysica::Sample_Uniform(PRNG, 0.0, 1.0);
	std::function<double(double)> cdf = [this, xi, vDM, param](double cosa) {
		return xi - CDF_Scattering_Angle_Electron(cosa, vDM, param);
	};
	double cos_alpha = libphysica::Find_Root(cdf, -1.0, 1.0, 1e-6);
	return cos_alpha;
}

double DM_Particle::PDF_Scattering_Angle_Nucleus(double cos_alpha, const Isotope& target, double vDM, double param)
{
	return PDF_Scattering_Angle_Nucleus_Base(cos_alpha, target, vDM, param);
}

double DM_Particle::PDF_Scattering_Angle_Electron(double cos_alpha, double vDM, double param)
{
	return PDF_Scattering_Angle_Electron_Base(cos_alpha, vDM, param);
}

double DM_Particle::CDF_Scattering_Angle_Nucleus(double cos_alpha, const Isotope& target, double vDM, double param)
{
	return CDF_Scattering_Angle_Nucleus_Base(cos_alpha, target, vDM, param);
}

double DM_Particle::CDF_Scattering_Angle_Electron(double cos_alpha, double vDM, double param)
{
	return CDF_Scattering_Angle_Electron_Base(cos_alpha, vDM, param);
}

double DM_Particle::Sample_Scattering_Angle_Nucleus(std::mt19937& PRNG, const Isotope& target, double vDM, double param)
{
	return Sample_Scattering_Angle_Nucleus_Base(PRNG, target, vDM, param);
}

double DM_Particle::Sample_Scattering_Angle_Electron(std::mt19937& PRNG, double vDM, double param)
{
	return Sample_Scattering_Angle_Electron_Base(PRNG, vDM, param);
}

}	// namespace obscura