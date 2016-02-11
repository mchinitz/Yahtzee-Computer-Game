// This file is informational only, and is not compiled. But the prototypes are the same. The project is currently large enough

typedef storing_considered_rolls(*distribution_calculator)(bool[], int, part_three_predicted_scores[], \
	storing_considered_rolls_with_overall_value[], storing_considered_rolls_with_overall_value[], int);

storing_considered_rolls *find_redundency[8192] = { NULL };
part_three_predicted_scores part_three_arr[8192][13];

class Beynesian_Computations
{
public:
	//Parameters: function which calculates distribution, current options remaining, and upper score (the latter two are the two factors used to compute adjustments, which uses get_distribution)
	Beynesian_Computation(distribution_calculator get_distribution, \
		bool *available_options, int initial_upper_score);

	//Stores the adjustments in "part_three_arr". "Find_redundency" is modifed to store distributions.
	void compute_beynesian_expected_values();
};

//Attributes of any Yahtzee player, including human and CPU players.
class Player
{
	void update_score(int choice, int rolls[5]);

	//Returns a vector of the dice kept if turn_number < 2, or the category chosen (as a 1-element vector) if turn_number = 2
	virtual vector<int> make_decision(int turn_number, vector<int> rolls) = 0;

};

void play_game(Player *player, Player *opponent);

//Please see Compute_Probs.cpp for more information about how the adjustments were calculated, for this is the key part of the program