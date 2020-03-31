#ifndef __Direct_Detection_hpp_
#define __Direct_Detection_hpp_

#include <string>

//Headers from libphys library
#include "Natural_Units.hpp"
#include "Numerics.hpp"

#include "DM_Particle.hpp"
#include "DM_Distribution.hpp"

//1. Detector base class
class Detector
{
	protected:
		std::string name, targets;
		double exposure, flat_efficiency;
		double energy_threshold, energy_max;

		virtual double Maximum_Energy_Deposit(const DM_Particle& DM, const DM_Distribution& DM_distr) const { return 0.0; };
		virtual double Minimum_DM_Mass(DM_Particle& DM, const DM_Distribution& DM_distr) const {return 0.0;};
		
		//Statistics
		std::string statistical_analysis;

		//a) Poisson statistics
		unsigned long int background_events;

		//b) Binned Poisson statistics
		unsigned int number_of_bins;
		std::vector<double> bins_energy;
		std::vector<unsigned long int> binned_background;

		//c) Maximum gap a'la Yellin
		std::vector<double> background_energy_data_sorted;
		double Likelihood_Maximum_Gap(const DM_Particle& DM, DM_Distribution& DM_distr);

		void Print_Summary_Base() const;
		
	public:
		Detector() : name("base name"), targets("base targets"), exposure(0.0), flat_efficiency(1.0), energy_threshold(0), energy_max(0), statistical_analysis("Poisson"), background_events(0), number_of_bins(0) {};
		Detector(std::string label, double expo,std::string target_type) : name(label), targets(target_type), exposure(expo) , flat_efficiency(1.0), energy_threshold(0), energy_max(0), statistical_analysis("Poisson"), background_events(0), number_of_bins(0) {};

		void Set_Flat_Efficiency(double eff);

		virtual double dRdE(double E, const DM_Particle& DM, DM_Distribution& DM_distr) { return 0.0;};
		virtual double Minimum_DM_Speed(const DM_Particle& DM) const {return 0.0;};

		//Statistics for upper bounds
		virtual double Likelihood(const DM_Particle& DM, DM_Distribution& DM_distr);
		virtual double Upper_Bound(DM_Particle& DM, DM_Distribution& DM_distr, double certainty = 0.95);
		std::vector<std::vector<double>> Limit_Curve(DM_Particle& DM, DM_Distribution& DM_distr, double mMin,double mMax, int points = 50,double certainty = 0.95);
		
		//a) Poisson
		void Set_Background(unsigned long int B);
		virtual double Total_Number_of_Signals(const DM_Particle& DM, DM_Distribution& DM_distr);
		//b) Binned Poisson
		void Define_Energy_Bins(double Emin, double Emax, int bins);
		void Set_Background(std::vector<unsigned long int> Bi);
		virtual std::vector<double> Binned_Number_of_Signals(const DM_Particle& DM, DM_Distribution& DM_distr);
		//c) Maximum gap
		void Use_Maximum_Gap(std::string filename_energy_data,double dim = keV);

		virtual void Print_Summary() const {Print_Summary_Base();};
};

//2. Functions for statistical analysis
	extern double CDF_Maximum_Gap(double x, double mu);

#endif