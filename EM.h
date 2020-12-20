#ifndef EM_H_INCLUDED
#define EM_H_INCLUDED

#include <iostream>
#include <fstream>
#include <cmath>
#include <iomanip>

namespace ai
{
namespace em
{
// g for gender, 0 for male, 1 for female
// w for weight, 0 for weight > 130,  effect #1
// h for height, 0 for height > 5'5", effect #0
// pg0 for probability of male gender, unconditional
struct Parameters
{
	double pg0; // probability of male gender

	// peff[j][i]: probability of effect #i = 0 given gender = j
	// j = 0 for male, j=1 for female
	// i = 0 for height, i = 1 for weight
	// so peff[0][0] is the probability of height > 55  given male gender
	//    peff[0][1] is the probability of weight > 130 given male gender
	//    peff[1][0] is the probability of height > 55  given female gender
	//    peff[1][1] is the probability of weight > 130 given female gender
	double peff[2][2];


	//Constructors
	Parameters(){}

	// for each combination of weight and height
	// the probability of male given a weight-height combination
	// for binary index ijk, P(gender=0 | weight=j, height=k), gender=0 for Male
	double probMale(int wh)
	{
		switch(wh)
		{
			case 0: // case w=0,h=0
			{
				double prob_male = pg0*peff[0][1]*peff[0][0];
				double prob_female = (1.0-pg0)*peff[1][1]*peff[1][0];
				return prob_male/(prob_male + prob_female);
			}
			case 1: // case w=0,h=1
			{
				double prob_male = pg0*peff[0][1]*(1-peff[0][0]);
				double prob_female = (1.0-pg0)*peff[1][1]*(1.0-peff[1][0]);
				return prob_male/(prob_male + prob_female);
			}
			case 2: // case w=1,h=0
			{
				double prob_male = pg0*(1.0-peff[0][1])*peff[0][0];
				double prob_female = (1.0-pg0)*(1.0-peff[1][1])*peff[1][0];
				return prob_male/(prob_male + prob_female);
			}
			case 3: // case w=1,h=1
			default:
			{
				double prob_male = pg0*(1.0-peff[0][1])*(1.0-peff[0][0]);
				double prob_female = (1.0-pg0)*(1.0-peff[1][1])*(1.0-peff[1][0]);
				return prob_male/(prob_male + prob_female);
			}
		}
	}
	// calculate the likelihood given complete configuration, i.e., gender, weight, height known
	double completeLikelyhood(int combo)
	{
		switch(combo)
		{
		case 0: // 000, male, w=0, h=0
			return pg0*peff[0][1]*peff[0][0];
		case 1: // 001, male, w=0, h=1
			return pg0*peff[0][1]*(1.0 - peff[0][0]);
		case 2: // 010, male, w=1, h=0
			return pg0*(1.0 - peff[0][1])*peff[0][0];
		case 3: // 011, male, w=1, h=1
			return pg0*(1.0 - peff[0][1])*(1.0-peff[0][0]);
		case 4: // 100, female, w=0, h=0
			return (1.0-pg0)*peff[1][1]*peff[1][0];
		case 5: // 101, female, w=0, h=1
			return (1.0-pg0)*peff[1][1]*(1.0 - peff[1][0]);
		case 6: // 110, female, w=1, h=0
			return (1.0-pg0)*(1.0-peff[1][1])*peff[1][0];
        case 7: // 111, female, w=1, h=1
		default:
			return (1.0-pg0)*(1.0-peff[1][1])*(1.0-peff[1][0]);
		}
	}

	double incompleteLikelihood(int combo)
	{
		switch(combo)
		{
		case 0: // 00, w=0, h=0
			return pg0*peff[0][1]*peff[0][0] + (1.0-pg0)*peff[1][1]*peff[1][0];
		case 1: // 01, w=0, h=1
			return pg0*peff[0][1]*(1.0-peff[0][0]) + (1.0-pg0)*peff[1][1]*(1.0-peff[1][0]);
		case 2: // 10, w=1, h=0
			return pg0*(1.0-peff[0][1])*peff[0][0] + (1.0-pg0)*(1.0 - peff[1][1])*peff[1][0];
		default:// 11, w=1, h=1
			return pg0*(1.0-peff[0][1])*(1.0-peff[0][0]) + (1.0-pg0)*(1.0 - peff[1][1])*(1.0 - peff[1][0]);
		}
	}

	static const double convergence_criteria = 0.001;

	friend std::ostream& operator<<(std::ostream & os, const Parameters & p)
	{
		os << "P(gender=0) = " << p.pg0;
		for(int i=0; i<2; ++i)
		{
		  os << ", P(eff[" << i << "]=0 | gender=0) = " << p.peff[0][i]
		     << ", P(eff[" << i << "]=0 | gender=1) = " << p.peff[1][i] << std::endl;
		}
		return os;
	}

}; // struct Parameters

class EM
{
public:
	// constructor
	EM()
	{
		// initialize counts
		for(int i=0; i<4; ++i)
		{
		    goodCounts[i] = 0;
		    goodCounts[i + 4] = 0;
		    missingCounts[i] = 0;
		}
		totalCount = 0;
	}
	// set parameters
	void setParameters(Parameters & param_)
	{
		parameters = param_;
	}
	// read input from file
	void read_input(const char* filename)
	{
		std::ifstream infile(filename);
		std::string line;
		getline(infile,line);
		// just count for each combination
		while(getline(infile,line))
		{
			++totalCount;
			// ignore all spaces
			int comb_index = 0;
			for(size_t i=0; i<line.size(); ++i)
			{
				if(line[i] == ' ' or line[i] == '\t' or line[i] == '\n')
					continue;
				comb_index = comb_index * 2 + (line[i] == '1');// 1 if char is 1, 0 otherwise (including '-')
			}
			if(line[0] == '-') // missing data
			{
				++missingCounts[comb_index];
//				std::cout << "line: " << line << ", updated missing count, comb_index "
//					 << comb_index << ", count " << missingCounts[comb_index] << std::endl;
			}
			else
			{
				++goodCounts[comb_index];
//				std::cout << "line: " << line << ", updated observed count, comb_index "
//					 << comb_index << ", count " << goodCounts[comb_index] << std::endl;
			}
		}
		infile.close();
	}
	// expectation
	void expectation()
	{
		//double probM = 0.0;
		for(int i=0; i<4;++i)
		{
			double probMale = parameters.probMale(i);// probability of male given weight and height comb i
			expectedCounts[i] = probMale * missingCounts[i]; // expected male count
			expectedCounts[i+4] = missingCounts[i] - expectedCounts[i]; //expected female count
		}
	}
	// maximization
	Parameters maximization()
	{
//		std::cout << "Printing parameters, " << parameters.pg0;
//		for(int g=0; g<2; g++)
//			for(int j=0; j<2;j++)
//		        std::cout << ", " << parameters.peff[g][j];

//		std::cout << "\nPrinting goodCounts";
//		for(int i=0; i < 8; ++i)
//			std::cout << ", " << goodCounts[i];
//		std::cout << std::endl << "Printing missingCounts";
//		for(int i=0; i < 4; ++i)
//			std::cout << ", " << missingCounts[i];
//		std::cout << std::endl << "Printing theta";
//		for(int i=0; i < 4; ++i)
//			std::cout << ", " << theta[i];
//		std::cout << std::endl;

		// double checking
		double sum_g0 = 0.0;
		double sum_g0w0 = 0.0;
		double sum_g0h0 = 0.0;
		double sum_g1w0 = 0.0;
		double sum_g1h0 = 0.0;
		for(int i=0;i<4;i++) // 000, 001, 010, 011
		{
			sum_g0 += expectedCounts[i] + goodCounts[i];
		}
		// 000 and 001, male, w=0, sum over h =0 to 1
		sum_g0w0 = expectedCounts[0] + expectedCounts[1] + goodCounts[0] + goodCounts[1];
		// 000 and 010
		sum_g0h0 = expectedCounts[0] + expectedCounts[2] + goodCounts[0] + goodCounts[2];
		// 100,101
		sum_g1w0 = expectedCounts[4] + expectedCounts[5] + goodCounts[4] + goodCounts[5];
		// 100 and 110
		sum_g1h0 = expectedCounts[4] + expectedCounts[6] + goodCounts[4] + goodCounts[6];
//		std::cout << "sums: " << sum_g0 << ", " << sum_g0w0 << ", " << sum_g0h0
//				  << ", " << sum_g1w0 << ", " << sum_g1h0 << ", " << totalCount << std::endl;
		Parameters param;
		param.pg0 = sum_g0 / totalCount;
		param.peff[0][0] = sum_g0h0/sum_g0;
		param.peff[0][1] = sum_g0w0/sum_g0;
		param.peff[1][0] = sum_g1h0/(totalCount - sum_g0);
		param.peff[1][1] = sum_g1w0/(totalCount - sum_g0);
//		std::cout << "Params: " << param.pg0 << ", " << param.peff[0][1] << ", " << param.peff[0][0]
//				  << ", " << param.peff[1][1] << ", " << param.peff[1][0] << std::endl;
		return param;
	}
	double compute_likelihood(Parameters & params)
	{
		double log_sum = 0.0;
		for(int i=0; i<4; ++i)
			log_sum += goodCounts[i] * std::log(params.completeLikelyhood(i))
		          + goodCounts[i + 4] * std::log(params.completeLikelyhood(i+4))
		          + missingCounts[i]*std::log(params.incompleteLikelihood(i));
		return log_sum;
	}
	// run the EM iterations until convergence
	Parameters optimize()
	{
		int iterations = 0;
		double new_log_likelihood = 0.0;
		double diff;
		log_likelihood = compute_likelihood(parameters);// impossible value to avoid coincidental spurious convergence
		std::cout << "Iteration    log_likelihood   delta\n";
		std::cout << "0            " << log_likelihood << "      0" << std::endl;
		do{
			++iterations;
			expectation();
			parameters = maximization();
			new_log_likelihood = compute_likelihood(parameters);
			diff = std::fabs(new_log_likelihood - log_likelihood);
			std::cout << iterations
			          << "         " << new_log_likelihood
			          << "         " << diff << std::endl;
			log_likelihood = new_log_likelihood;
		}while(diff > convergence_criteria);
		return parameters;
	}

private:
	Parameters parameters;
	int goodCounts[8]; // number of observed data for each combination (gender, weight, height)
	int missingCounts[4]; // number of missing data for each combination (gender, weight, height)
	// number of expected count for missing data, for each combination (gender, weight, height)
	// expectedCounts[i] = missingCounts[i] * theta[i]
	double expectedCounts[8] ;
	int totalCount; // observed count plus missing counts
	double log_likelihood;
	static const double convergence_criteria = 0.001;
};

}//namespace bayes
}//namespace ai


#endif // EM_H_INCLUDED
