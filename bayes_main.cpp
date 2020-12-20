#include <stdio.h>
#include "EM.h"
#include <cstdlib>
#include <string>
#include <time.h>
#include <boost/random.hpp>

using namespace ai::em;
using namespace std;
int main(int argc, char** argv)
{
    Parameters param[6];
    // peff[j][i]: probability of effect #i = 0 given gender = j
	// j = 0 for male, j=1 for female
	// i = 0 for height, i = 1 for weight
	// so peff[0][0] is the probability of height > 55  given male gender
	//    peff[0][1] is the probability of weight > 130 given male gender
	//    peff[1][0] is the probability of height > 55  given female gender
	//    peff[1][1] is the probability of weight > 130 given female gender
    param[0].pg0 = 0.7;
    param[0].peff[0][0] = 0.7; // height 0 (>55) given male
    param[0].peff[0][1] = 0.8; // weight 0 (>130) given male
    param[0].peff[1][0] = 0.3; // height 0 (>55) given female
    param[0].peff[1][1] = 0.4; // weight 0 (>130) given female

    if(argc == 7)
    {
        param[0].pg0 = atof(argv[2]);
        param[0].peff[0][0] = atof(argv[3]); // height 0 (>55) given male
        param[0].peff[0][1] = atof(argv[4]); // weight 0 (>130) given male
        param[0].peff[1][0] = atof(argv[5]); // height 0 (>55) given female
        param[0].peff[1][1] = atof(argv[6]); // weight 0 (>130) given female
    }
//    //generating five random parameters set
    time_t timer;
    time(&timer);  /* get current time; same as: timer = time(NULL)  */
    boost::mt19937 seed( (int)timer );
    boost::uniform_real<> dist(0.0,1.0);
    boost::variate_generator<boost::mt19937&, boost::uniform_real<> > random(seed,dist);
    for (int i=1; i <6 ; ++i)
    {
        param[i].pg0 = random();
        param[i].peff[0][0] = random(); // height 0 (>55) given male
        param[i].peff[0][1] = random(); // weight 0 (>130) given male
        param[i].peff[1][0] = random();// height 0 (>55) given female
        param[i].peff[1][1] = random();// weight 0 (>130) given female
    }
    EM em;
    em.read_input(argv[1]);
    // five more random starting parameters
//    cout<<"random number 1 = "<<random()<<endl;
//    cout<<"random number 2 = "<<random()<<endl;
//    cout<<"random number 3= "<<random()<<endl;
    for (int i=0; i <6 ; ++i)
    {

        std::cout <<"Starting parameter set "<<i<<": " << endl;
        em.setParameters(param[i]);
        //em.read_input(argv[1]);
        Parameters final_param = em.optimize();
        printf ("%40s\t %20s\t %20s\n", "Probability", "Starting parameter", "final parameters");
        printf ("%40s\t %20.6f\t %20.6f\n", "P(gender=M)" ,  param[i].pg0, final_param.pg0);
        printf ("%40s\t %20.6f\t %20.6f\n", "P(weight=greater_than_130|gender=M)", param[i].peff[0][0], final_param.peff[0][0]);
        printf ("%40s\t %20.6f\t %20.6f\n", "P(weight=greater_than_130|gender=F)", param[i].peff[0][1], final_param.peff[0][1]);
        printf ("%40s\t %20.6f\t %20.6f\n", "P(height= greater_than_55|gender=M)", param[i].peff[1][0], final_param.peff[1][0]);
        printf ("%40s\t %20.6f\t %20.6f\n", "P(height= greater_than_55|gender=F)", param[i].peff[1][1], final_param.peff[1][1]);
    }




    return 0;

}
