#include <iostream>
using namespace std;
#define NUM_CATEGORIES 13
#define NUM_OPTIONS 43

const int powers_of_six_array[6] = {1,6,36,216,1296, 7776};
const int factorial_arr[6] = {1, 1, 2, 6, 24, 120};
const int powers_of_two_array[13] = {1,2,4,8,16,32,64,128,256,512,1024,2048,4096};
const double global_lower_scores[14] = {0,0,0,0,0,0,0,0,0,25,30,40,50,0};

//For storing adjustments to the rest of the game, so that the computer player does not just try to maximize its score for the current turn.
//The score_independent_from_freq is the part of the adjustment that does not reflect the frequency of an upper category on the current turn.
//The freq_dependent_scores are the expected value of the 35-point bonus, given a frequency of 1-4 for a given upper category.
//Each structure represents a unique category and a unique distributions.
typedef struct part_three_predicted_scores
{
	double score_independent_from_freq = 0;
	double freq_dependent_scores[4];
	part_three_predicted_scores()
	{
		for (int i = 0; i < 4; i++)
			freq_dependent_scores[i] = 0;
	}
} part_three_predicted_scores;

//For storing distributions, given a set of available options and the current upper score.
typedef struct storing_considered_rolls
{
	double Probabilities[NUM_OPTIONS];
	double sums[3];
	int catgr_may_take_zero;
	storing_considered_rolls()
	{
		for (int i = 0; i < NUM_OPTIONS; i++)
			Probabilities[i] = 0;
		for (int i = 0; i < 3; i++)
			sums[i] = 0;
	}
} storing_considered_rolls;


typedef struct storing_considered_rolls_with_overall_value
{
	storing_considered_rolls info;
	double value = 0;
} storing_considered_rolls_with_overall_value;


enum {ones = 1, twos, threes, fours, fives, sixes, three_of_a_kind, \
four_of_a_kind, full_house, small_strait, large_strait, yahtzee, chance};

//Evaluates a roll on any turn in a round based on its score and optimal choice
typedef struct value_of_roll
{
	double value = 0;
	int option;
} value_of_roll;

//Class for evaluating rolls on the last turn of a round
class CategoryInfo
{
public:

	virtual bool is_valid_option(int category, int rolls[5], bool available_options[NUM_CATEGORIES+1]);

	//selects a category. The category selected is such that the (score for the roll + the adjustment to the rest of the game) is maximized
	value_of_roll make_choice(int roll [5], part_three_predicted_scores weights_part_3[13], bool available_options[14], \
			bool is_last_round, int curr_upper_score);

};

//Returns the distribution (if rolls_left = 3) or dice kept with the specified available options, upper score, and adjustments.
storing_considered_rolls get_distribution_or_choice(bool available_options[NUM_CATEGORIES + 1], int curr_upper_score, \
		part_three_predicted_scores weights_part_three[13], storing_considered_rolls_with_overall_value decision_input [9331],\
		storing_considered_rolls_with_overall_value decision_output[252], int rolls_left = 3);


